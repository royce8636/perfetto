// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import * as m from 'mithril';

import {assertExists, assertTrue} from '../base/logging';

import {globals} from './globals';
import {isPanelVNode, Panel, PanelSize} from './panel';
import {
  debugNow,
  perfDebug,
  perfDisplay,
  RunningStatistics,
  runningStatStr
} from './perf';

/**
 * If the panel container scrolls, the backing canvas height is
 * SCROLLING_CANVAS_OVERDRAW_FACTOR * parent container height.
 */
const SCROLLING_CANVAS_OVERDRAW_FACTOR = 2;

// We need any here so we can accept vnodes with arbitrary attrs.
// tslint:disable-next-line:no-any
export type AnyAttrsVnode = m.Vnode<any, {}>;

interface Attrs {
  panels: AnyAttrsVnode[];
  doesScroll: boolean;
}

export class PanelContainer implements m.ClassComponent<Attrs> {
  // These values are updated with proper values in oncreate.
  private parentWidth = 0;
  private parentHeight = 0;
  private scrollTop = 0;
  private panelHeights: number[] = [];
  private totalPanelHeight = 0;
  private canvasHeight = 0;

  private panelPerfStats = new WeakMap<Panel, RunningStatistics>();
  private perfStats = {
    totalPanels: 0,
    panelsOnCanvas: 0,
    renderStats: new RunningStatistics(10),
  };

  // attrs received in the most recent mithril redraw.
  private attrs?: Attrs;

  private canvasOverdrawFactor: number;
  private ctx?: CanvasRenderingContext2D;

  private onResize: () => void = () => {};
  private parentOnScroll: () => void = () => {};
  private canvasRedrawer: () => void;

  constructor(vnode: m.CVnode<Attrs>) {
    this.canvasOverdrawFactor =
        vnode.attrs.doesScroll ? SCROLLING_CANVAS_OVERDRAW_FACTOR : 1;
    this.canvasRedrawer = () => this.redrawCanvas();
    globals.rafScheduler.addRedrawCallback(this.canvasRedrawer);
    perfDisplay.addContainer(this);
  }

  oncreate(vnodeDom: m.CVnodeDOM<Attrs>) {
    const attrs = vnodeDom.attrs;
    // Save the canvas context in the state.
    const canvas =
        vnodeDom.dom.querySelector('.main-canvas') as HTMLCanvasElement;
    const ctx = canvas.getContext('2d');
    if (!ctx) {
      throw Error('Cannot create canvas context');
    }
    this.ctx = ctx;

    const clientRect =
        assertExists(vnodeDom.dom.parentElement).getBoundingClientRect();
    this.parentWidth = clientRect.width;
    this.parentHeight = clientRect.height;

    this.updatePanelHeightsFromDom(vnodeDom);
    (vnodeDom.dom as HTMLElement).style.height = `${this.totalPanelHeight}px`;

    this.canvasHeight = this.getCanvasHeight(attrs.doesScroll);
    this.updateCanvasDimensions(vnodeDom);

    // Save the resize handler in the state so we can remove it later.
    // TODO: Encapsulate resize handling better.
    this.onResize = () => {
      const clientRect =
          assertExists(vnodeDom.dom.parentElement).getBoundingClientRect();
      this.parentWidth = clientRect.width;
      this.parentHeight = clientRect.height;
      this.canvasHeight = this.getCanvasHeight(attrs.doesScroll);
      this.updateCanvasDimensions(vnodeDom);
      globals.rafScheduler.scheduleFullRedraw();
    };

    // Once ResizeObservers are out, we can stop accessing the window here.
    window.addEventListener('resize', this.onResize);

    // TODO(dproy): Handle change in doesScroll attribute.
    if (vnodeDom.attrs.doesScroll) {
      this.parentOnScroll = () => {
        this.scrollTop = vnodeDom.dom.parentElement!.scrollTop;
        this.repositionCanvas(vnodeDom);
        globals.rafScheduler.scheduleRedraw();
      };
      vnodeDom.dom.parentElement!.addEventListener(
          'scroll', this.parentOnScroll, {passive: true});
    }
  }

  onremove({attrs, dom}: m.CVnodeDOM<Attrs>) {
    window.removeEventListener('resize', this.onResize);
    globals.rafScheduler.removeRedrawCallback(this.canvasRedrawer);
    if (attrs.doesScroll) {
      dom.parentElement!.removeEventListener('scroll', this.parentOnScroll);
    }
    perfDisplay.removeContainer(this);
  }

  view({attrs}: m.CVnode<Attrs>) {
    // We receive a new vnode object with new attrs on every mithril redraw. We
    // store the latest attrs so redrawCanvas can use it.
    this.attrs = attrs;
    const renderPanel = (panel: m.Vnode) => perfDebug() ?
        m('.panel', panel, m('.debug-panel-border')) :
        m('.panel', panel);

    return m(
        '.scroll-limiter',
        m('canvas.main-canvas'),
        attrs.panels.map(renderPanel));
  }

  onupdate(vnodeDom: m.CVnodeDOM<Attrs>) {
    this.repositionCanvas(vnodeDom);

    if (this.updatePanelHeightsFromDom(vnodeDom)) {
      (vnodeDom.dom as HTMLElement).style.height = `${this.totalPanelHeight}px`;
    }

    // In non-scrolling case, canvas height can change if panel heights changed.
    const canvasHeight = this.getCanvasHeight(vnodeDom.attrs.doesScroll);
    if (this.canvasHeight !== canvasHeight) {
      this.canvasHeight = canvasHeight;
      this.updateCanvasDimensions(vnodeDom);
    }
  }

  private updateCanvasDimensions(vnodeDom: m.CVnodeDOM<Attrs>) {
    const canvas =
        assertExists(vnodeDom.dom.querySelector('canvas.main-canvas')) as
        HTMLCanvasElement;
    const ctx = assertExists(this.ctx);
    canvas.style.height = `${this.canvasHeight}px`;
    const dpr = window.devicePixelRatio;
    ctx.canvas.width = this.parentWidth * dpr;
    ctx.canvas.height = this.canvasHeight * dpr;
    ctx.scale(dpr, dpr);
  }

  private updatePanelHeightsFromDom(vnodeDom: m.CVnodeDOM<Attrs>): boolean {
    const prevHeight = this.totalPanelHeight;
    this.panelHeights = [];
    this.totalPanelHeight = 0;

    const panels = vnodeDom.dom.querySelectorAll('.panel');
    assertTrue(panels.length === vnodeDom.attrs.panels.length);
    for (let i = 0; i < panels.length; i++) {
      const height = panels[i].getBoundingClientRect().height;
      this.panelHeights[i] = height;
      this.totalPanelHeight += height;
    }

    return this.totalPanelHeight !== prevHeight;
  }

  private getCanvasHeight(doesScroll: boolean) {
    return doesScroll ? this.parentHeight * this.canvasOverdrawFactor :
                        this.totalPanelHeight;
  }

  private repositionCanvas(vnodeDom: m.CVnodeDOM<Attrs>) {
    const canvas =
        assertExists(vnodeDom.dom.querySelector('canvas.main-canvas')) as
        HTMLCanvasElement;
    const canvasYStart = this.scrollTop - this.getCanvasOverdrawHeightPerSide();
    canvas.style.transform = `translateY(${canvasYStart}px)`;
  }

  private overlapsCanvas(yStart: number, yEnd: number) {
    return yEnd > 0 && yStart < this.canvasHeight;
  }

  private redrawCanvas() {
    const redrawStart = debugNow();
    if (!this.ctx) return;
    this.ctx.clearRect(0, 0, this.parentWidth, this.canvasHeight);
    const canvasYStart = this.scrollTop - this.getCanvasOverdrawHeightPerSide();

    let panelYStart = 0;
    const panels = assertExists(this.attrs).panels;
    assertTrue(panels.length === this.panelHeights.length);
    let totalOnCanvas = 0;
    for (let i = 0; i < panels.length; i++) {
      const panel = panels[i];
      const panelHeight = this.panelHeights[i];
      const yStartOnCanvas = panelYStart - canvasYStart;

      if (!this.overlapsCanvas(yStartOnCanvas, yStartOnCanvas + panelHeight)) {
        panelYStart += panelHeight;
        continue;
      }

      totalOnCanvas++;

      if (!isPanelVNode(panel)) {
        throw Error('Vnode passed to panel container is not a panel');
      }

      this.ctx.save();
      this.ctx.translate(0, yStartOnCanvas);
      const clipRect = new Path2D();
      const size = {width: this.parentWidth, height: panelHeight};
      clipRect.rect(0, 0, size.width, size.height);
      this.ctx.clip(clipRect);
      const beforeRender = debugNow();
      panel.state.renderCanvas(this.ctx, size, panel);
      this.updatePanelStats(
          i, panel.state, debugNow() - beforeRender, this.ctx, size);
      this.ctx.restore();
      panelYStart += panelHeight;
    }
    const redrawDur = debugNow() - redrawStart;
    this.updatePerfStats(redrawDur, panels.length, totalOnCanvas);
  }

  private updatePanelStats(
      panelIndex: number, panel: Panel, renderTime: number,
      ctx: CanvasRenderingContext2D, size: PanelSize) {
    if (!perfDebug()) return;
    let renderStats = this.panelPerfStats.get(panel);
    if (renderStats === undefined) {
      renderStats = new RunningStatistics();
      this.panelPerfStats.set(panel, renderStats);
    }
    renderStats.addValue(renderTime);

    const statW = 300;
    ctx.fillStyle = 'hsl(97, 100%, 96%)';
    ctx.fillRect(size.width - statW, size.height - 20, statW, 20);
    ctx.fillStyle = 'hsla(122, 77%, 22%)';
    const statStr = `Panel ${panelIndex + 1} | ` + runningStatStr(renderStats);
    ctx.fillText(statStr, size.width - statW, size.height - 10);
  }

  private updatePerfStats(
      renderTime: number, totalPanels: number, panelsOnCanvas: number) {
    if (!perfDebug()) return;
    this.perfStats.renderStats.addValue(renderTime);
    this.perfStats.totalPanels = totalPanels;
    this.perfStats.panelsOnCanvas = panelsOnCanvas;
  }

  renderPerfStats(index: number) {
    assertTrue(perfDebug());
    return [m(
        'section',
        m('div', `Panel Container ${index + 1}`),
        m('div',
          `${this.perfStats.totalPanels} panels, ` +
              `${this.perfStats.panelsOnCanvas} on canvas.`),
        m('div', runningStatStr(this.perfStats.renderStats)), )];
  }

  private getCanvasOverdrawHeightPerSide() {
    const overdrawHeight = (this.canvasOverdrawFactor - 1) * this.parentHeight;
    return overdrawHeight / 2;
  }
}
