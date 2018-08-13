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

import {TimeSpan, timeToString} from '../common/time';

import {DragGestureHandler} from './drag_gesture_handler';
import {globals} from './globals';
import {Panel} from './panel';
import {TimeScale} from './time_scale';
import {OVERVIEW_QUERY_ID} from './viewer_page';

export class OverviewTimelinePanel implements Panel {
  private width?: number;
  private dragStartPx = 0;
  private gesture?: DragGestureHandler;
  private timeScale?: TimeScale;
  private totTime = new TimeSpan(0, 0);

  constructor() {}

  getHeight(): number {
    return 100;
  }

  updateDom(dom: HTMLElement) {
    this.width = dom.getBoundingClientRect().width;
    this.totTime = new TimeSpan(
        0, globals.state.traceTime.endSec - globals.state.traceTime.startSec);
    this.timeScale = new TimeScale(this.totTime, [0, this.width]);

    if (this.gesture === undefined) {
      this.gesture = new DragGestureHandler(
          dom,
          this.onDrag.bind(this),
          this.onDragStart.bind(this),
          this.onDragEnd.bind(this));
    }
  }

  renderCanvas(ctx: CanvasRenderingContext2D) {
    if (this.width === undefined) return;
    if (this.timeScale === undefined) return;
    const headerHeight = 25;
    const tracksHeight = this.getHeight() - headerHeight;

    // Draw time labels on the top header.
    ctx.font = '10px Google Sans';
    ctx.fillStyle = '#999';
    for (let i = 0; i < 100; i++) {
      const xPos = i * this.width / 100;
      const t = this.timeScale.pxToTime(xPos);
      if (xPos < 0) continue;
      if (xPos > this.width) break;
      if (i % 10 === 0) {
        ctx.fillRect(xPos, 0, 1, headerHeight - 5);
        ctx.fillText(timeToString(t), xPos + 5, 18);
      } else {
        ctx.fillRect(xPos, 0, 1, 5);
      }
    }

    // Draw mini-tracks with quanitzed density for each process.
    if (globals.queryResults.has(OVERVIEW_QUERY_ID)) {
      const res: {[key: string]: {name: string, load: Uint8Array}} =
          globals.queryResults.get(OVERVIEW_QUERY_ID)!;
      const numProcs = Object.keys(res).length;
      const hueStep = Math.floor(255 / numProcs);
      let y = 0;
      const trackHeight = (tracksHeight - 2) / numProcs;
      for (const upid of Object.keys(res)) {
        const loads = res[upid].load;
        const px = this.width / loads.length;
        for (let i = 0; i < loads.length; i++) {
          const lightness = Math.ceil((1 - loads[i] / 0xff * 0.7) * 100);
          ctx.fillStyle = `hsl(${255 - y * hueStep}, 50%, ${lightness}%)`;
          ctx.fillRect(i * px, headerHeight + y * trackHeight, px, trackHeight);
        }
        y++;
      }
    }

    // Draw bottom border.
    ctx.fillStyle = 'hsl(219, 40%, 50%)';
    ctx.fillRect(0, this.getHeight() - 2, this.width, 2);

    // Draw semi-opaque rects that occlude the non-visible time range.
    const vizTime = globals.frontendLocalState.visibleWindowTime;
    const vizStartPx = this.timeScale.timeToPx(vizTime.start);
    const vizEndPx = this.timeScale.timeToPx(vizTime.end);

    ctx.fillStyle = 'rgba(240, 240, 240, 0.7)';
    ctx.fillRect(0, headerHeight, vizStartPx, tracksHeight);
    ctx.fillRect(vizEndPx, headerHeight, this.width - vizEndPx, tracksHeight);

    // Draw brushes.
    ctx.fillStyle = '#999';
    ctx.fillRect(vizStartPx, headerHeight, 1, tracksHeight);
    ctx.fillRect(vizEndPx, headerHeight, 1, tracksHeight);
    const handleWidth = 3;
    const handleHeight = 25;
    const y = headerHeight + (tracksHeight - handleHeight) / 2;
    ctx.fillRect(vizStartPx - handleWidth, y, handleWidth, handleHeight);
    ctx.fillRect(vizEndPx + 1, y, handleWidth, handleHeight);
  }

  onDrag(x: number) {
    // Set visible time limits from selection.
    if (this.timeScale === undefined) return;
    let tStart = this.timeScale.pxToTime(this.dragStartPx);
    let tEnd = this.timeScale.pxToTime(x);
    if (tStart > tEnd) [tStart, tEnd] = [tEnd, tStart];
    const vizTime = new TimeSpan(tStart, tEnd);
    globals.frontendLocalState.updateVisibleTime(vizTime);
    globals.rafScheduler.scheduleOneRedraw();
  }

  onDragStart(x: number) {
    this.dragStartPx = x;
  }

  onDragEnd() {
    this.dragStartPx = 0;
  }
}