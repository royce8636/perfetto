// Copyright (C) 2023 The Android Open Source Project
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

import m from 'mithril';

import {searchSegment} from '../base/binary_search';
import {Disposable, NullDisposable} from '../base/disposable';
import {assertTrue, assertUnreachable} from '../base/logging';
import {duration, Time, time} from '../base/time';
import {drawTrackHoverTooltip} from '../common/canvas_utils';
import {raf} from '../core/raf_scheduler';
import {EngineProxy, LONG, NUM, Track} from '../public';
import {Button} from '../widgets/button';
import {MenuItem, MenuDivider, PopupMenu2} from '../widgets/menu';

import {checkerboardExcept} from './checkerboard';
import {globals} from './globals';
import {PanelSize} from './panel';
import {constraintsToQuerySuffix} from './sql_utils';
import {NewTrackArgs} from './track';
import {CacheKey, TimelineCache} from '../core/timeline_cache';
import {featureFlags} from '../core/feature_flags';

export const COUNTER_DEBUG_MENU_ITEMS = featureFlags.register({
  id: 'counterDebugMenuItems',
  name: 'Counter debug menu items',
  description: 'Extra counter menu items for debugging purposes.',
  defaultValue: false,
});

function roundAway(n: number): number {
  const exp = Math.ceil(Math.log10(Math.max(Math.abs(n), 1)));
  const pow10 = Math.pow(10, exp);
  return Math.sign(n) * (Math.ceil(Math.abs(n) / (pow10 / 20)) * (pow10 / 20));
}

function toLabel(n: number): string {
  if (n === 0) {
    return '0';
  }
  const units: [number, string][] = [
    [0.000000001, 'n'],
    [0.000001, 'u'],
    [0.001, 'm'],
    [1, ''],
    [1000, 'K'],
    [1000 * 1000, 'M'],
    [1000 * 1000 * 1000, 'G'],
    [1000 * 1000 * 1000 * 1000, 'T'],
  ];
  let largestMultiplier;
  let largestUnit;
  [largestMultiplier, largestUnit] = units[0];
  for (const [multiplier, unit] of units) {
    if (multiplier >= n) {
      break;
    }
    [largestMultiplier, largestUnit] = [multiplier, unit];
  }
  return `${Math.round(n / largestMultiplier)}${largestUnit}`;
}

class RangeSharer {
  static singleton?: RangeSharer;

  static get(): RangeSharer {
    if (RangeSharer.singleton === undefined) {
      RangeSharer.singleton = new RangeSharer();
    }
    return RangeSharer.singleton;
  }

  private tagToRange: Map<string, [number, number]>;
  private keyToEnabled: Map<string, boolean>;

  constructor() {
    this.tagToRange = new Map();
    this.keyToEnabled = new Map();
  }

  isEnabled(key: string): boolean {
    const value = this.keyToEnabled.get(key);
    if (value === undefined) {
      return true;
    }
    return value;
  }

  setEnabled(key: string, enabled: boolean): void {
    this.keyToEnabled.set(key, enabled);
  }

  share(
    options: CounterOptions,
    [min, max]: [number, number],
  ): [number, number] {
    const key = options.yRangeSharingKey;
    if (key === undefined || !this.isEnabled(key)) {
      return [min, max];
    }

    const tag = `${options.yRangeSharingKey}-${options.yMode}-${
      options.yDisplay
    }-${!!options.enlarge}`;
    const cachedRange = this.tagToRange.get(tag);
    if (cachedRange === undefined) {
      this.tagToRange.set(tag, [min, max]);
      return [min, max];
    }

    cachedRange[0] = Math.min(min, cachedRange[0]);
    cachedRange[1] = Math.max(max, cachedRange[1]);

    return [cachedRange[0], cachedRange[1]];
  }
}

interface CounterData {
  timestamps: BigInt64Array;
  counts: Uint32Array;
  avgValues: Float64Array;
  minDisplayValues: Float64Array;
  maxDisplayValues: Float64Array;
  lastDisplayValues: Float64Array;
  displayValueRange: [number, number];
}

// 0.5 Makes the horizontal lines sharp.
const MARGIN_TOP = 3.5;

interface CounterLimits {
  maxDisplayValue: number;
  minDisplayValue: number;
  maxDurNs: duration;
}

interface CounterTooltipState {
  lastDisplayValue: number;
  avgValue: number;
  count: number;
  ts: time;
  tsEnd?: time;
}

export interface CounterOptions {
  // Mode for computing the y value. Options are:
  // value = v[t] directly the value of the counter at time t
  // delta = v[t] - v[t-1] delta between value and previous value
  // rate = (v[t] - v[t-1]) / dt as delta but normalized for time
  yMode: 'value' | 'delta' | 'rate';

  // Whether Y scale should cover all of the possible values (and therefore, be
  // static) or whether it should be dynamic and cover only the visible values.
  yRange: 'all' | 'viewport';

  // Whether the Y scale should:
  // zero = y-axis scale should cover the origin (zero)
  // minmax = y-axis scale should cover just the range of yRange
  // log = as minmax but also use a log scale
  yDisplay: 'zero' | 'minmax' | 'log';

  // Whether the range boundaries should be strict and use the precise min/max
  // values or whether they should be rounded down/up to the nearest human
  // readable value.
  yRangeRounding: 'strict' | 'human_readable';

  // Allows *extending* the range of the y-axis counter increasing
  // the maximum (via yOverrideMaximum) or decreasing the minimum
  // (via yOverrideMinimum). This is useful for percentage counters
  // where the range (0-100) is known statically upfront and even if
  // the trace only includes smaller values.
  yOverrideMaximum?: number;
  yOverrideMinimum?: number;

  // If set all counters with the same key share a range.
  yRangeSharingKey?: string;

  // Show the chart as 4x the height.
  enlarge?: boolean;

  // unit for the counter. This is displayed in the tooltip and
  // legend.
  unit?: string;
}

export type BaseCounterTrackArgs = NewTrackArgs & {
  options?: Partial<CounterOptions>;
};

export abstract class BaseCounterTrack implements Track {
  protected engine: EngineProxy;
  protected trackKey: string;

  // This is the over-skirted cached bounds:
  private countersKey: CacheKey = CacheKey.zero();

  private counters: CounterData = {
    timestamps: new BigInt64Array(0),
    counts: new Uint32Array(0),
    avgValues: new Float64Array(0),
    minDisplayValues: new Float64Array(0),
    maxDisplayValues: new Float64Array(0),
    lastDisplayValues: new Float64Array(0),
    displayValueRange: [0, 0],
  };

  private cache: TimelineCache<CounterData> = new TimelineCache(5);

  // Cleanup hook for onInit.
  private initState?: Disposable;

  private limits?: CounterLimits;

  private mousePos = {x: 0, y: 0};
  private hover?: CounterTooltipState;
  private defaultOptions: Partial<CounterOptions>;
  private options?: CounterOptions;

  private getCounterOptions(): CounterOptions {
    if (this.options === undefined) {
      const options = this.getDefaultCounterOptions();
      for (const [key, value] of Object.entries(this.defaultOptions)) {
        if (value !== undefined) {
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          (options as any)[key] = value;
        }
      }
      this.options = options;
    }
    return this.options;
  }

  // Extension points.

  // onInit hook lets you do asynchronous set up e.g. creating a table
  // etc. We guarantee that this will be resolved before doing any
  // queries using the result of getSqlSource(). All persistent
  // state in trace_processor should be cleaned up when dispose is
  // called on the returned hook.
  async onInit(): Promise<Disposable> {
    return new NullDisposable();
  }

  // This should be an SQL expression returning the columns `ts` and `value`.
  abstract getSqlSource(): string;

  protected getDefaultCounterOptions(): CounterOptions {
    return {
      yRange: 'all',
      yRangeRounding: 'human_readable',
      yMode: 'value',
      yDisplay: 'zero',
    };
  }

  constructor(args: BaseCounterTrackArgs) {
    this.engine = args.engine;
    this.trackKey = args.trackKey;
    this.defaultOptions = args.options ?? {};
  }

  getHeight() {
    const height = 40;
    return this.getCounterOptions().enlarge ? height * 4 : height;
  }

  // A method to render menu items for switching the defualt
  // rendering options.  Useful if a subclass wants to incorporate it
  // as a submenu.
  protected getCounterContextMenuItems(): m.Children {
    const options = this.getCounterOptions();

    return [
      m(
        MenuItem,
        {
          label: `Display (currently: ${options.yDisplay})`,
        },

        m(MenuItem, {
          label: 'Zero-based',
          icon:
            options.yDisplay === 'zero'
              ? 'radio_button_checked'
              : 'radio_button_unchecked',
          onclick: () => {
            options.yDisplay = 'zero';
            this.invalidate();
          },
        }),

        m(MenuItem, {
          label: 'Min/Max',
          icon:
            options.yDisplay === 'minmax'
              ? 'radio_button_checked'
              : 'radio_button_unchecked',
          onclick: () => {
            options.yDisplay = 'minmax';
            this.invalidate();
          },
        }),

        m(MenuItem, {
          label: 'Log',
          icon:
            options.yDisplay === 'log'
              ? 'radio_button_checked'
              : 'radio_button_unchecked',
          onclick: () => {
            options.yDisplay = 'log';
            this.invalidate();
          },
        }),
      ),

      m(MenuItem, {
        label: 'Zoom on scroll',
        icon:
          options.yRange === 'viewport'
            ? 'check_box'
            : 'check_box_outline_blank',
        onclick: () => {
          options.yRange = options.yRange === 'viewport' ? 'all' : 'viewport';
          this.invalidate();
        },
      }),

      m(MenuItem, {
        label: `Enlarge`,
        icon: options.enlarge ? 'check_box' : 'check_box_outline_blank',
        onclick: () => {
          options.enlarge = !options.enlarge;
          this.invalidate();
        },
      }),

      options.yRangeSharingKey &&
        m(MenuItem, {
          label: `Share y-axis scale (group: ${options.yRangeSharingKey})`,
          icon: RangeSharer.get().isEnabled(options.yRangeSharingKey)
            ? 'check_box'
            : 'check_box_outline_blank',
          onclick: () => {
            const key = options.yRangeSharingKey;
            if (key === undefined) {
              return;
            }
            const sharer = RangeSharer.get();
            sharer.setEnabled(key, !sharer.isEnabled(key));
            this.invalidate();
          },
        }),

      COUNTER_DEBUG_MENU_ITEMS.get() && [
        m(MenuDivider),
        m(
          MenuItem,
          {
            label: `Mode (currently: ${options.yMode})`,
          },

          m(MenuItem, {
            label: 'Value',
            icon:
              options.yMode === 'value'
                ? 'radio_button_checked'
                : 'radio_button_unchecked',
            onclick: () => {
              options.yMode = 'value';
              this.invalidate();
            },
          }),

          m(MenuItem, {
            label: 'Delta',
            icon:
              options.yMode === 'delta'
                ? 'radio_button_checked'
                : 'radio_button_unchecked',
            onclick: () => {
              options.yMode = 'delta';
              this.invalidate();
            },
          }),

          m(MenuItem, {
            label: 'Rate',
            icon:
              options.yMode === 'rate'
                ? 'radio_button_checked'
                : 'radio_button_unchecked',
            onclick: () => {
              options.yMode = 'rate';
              this.invalidate();
            },
          }),
        ),
        m(MenuItem, {
          label: 'Round y-axis scale',
          icon:
            options.yRangeRounding === 'human_readable'
              ? 'check_box'
              : 'check_box_outline_blank',
          onclick: () => {
            options.yRangeRounding =
              options.yRangeRounding === 'human_readable'
                ? 'strict'
                : 'human_readable';
            this.invalidate();
          },
        }),
      ],
    ];
  }

  protected invalidate() {
    this.limits = undefined;
    this.cache.invalidate();
    this.countersKey = CacheKey.zero();
    this.counters = {
      timestamps: new BigInt64Array(0),
      counts: new Uint32Array(0),
      avgValues: new Float64Array(0),
      minDisplayValues: new Float64Array(0),
      maxDisplayValues: new Float64Array(0),
      lastDisplayValues: new Float64Array(0),
      displayValueRange: [0, 0],
    };
    this.hover = undefined;

    raf.scheduleFullRedraw();
  }

  // A method to render a context menu corresponding to switching the rendering
  // modes. By default, getTrackShellButtons renders it, but a subclass can call
  // it manually, if they want to customise rendering track buttons.
  protected getCounterContextMenu(): m.Child {
    return m(
      PopupMenu2,
      {
        trigger: m(Button, {icon: 'show_chart', compact: true}),
      },
      this.getCounterContextMenuItems(),
    );
  }

  getTrackShellButtons(): m.Children {
    return this.getCounterContextMenu();
  }

  async onCreate(): Promise<void> {
    this.initState = await this.onInit();
  }

  async onUpdate(): Promise<void> {
    const {visibleTimeScale: timeScale, visibleWindowTime: vizTime} =
      globals.timeline;

    const windowSizePx = Math.max(1, timeScale.pxSpan.delta);
    const rawStartNs = vizTime.start.toTime();
    const rawEndNs = vizTime.end.toTime();
    const rawCountersKey = CacheKey.create(rawStartNs, rawEndNs, windowSizePx);

    // If the visible time range is outside the cached area, requests
    // asynchronously new data from the SQL engine.
    await this.maybeRequestData(rawCountersKey);
  }

  render(ctx: CanvasRenderingContext2D, size: PanelSize) {
    const {
      visibleTimeScale: timeScale,
      // visibleWindowTime: vizTime,
    } = globals.timeline;

    // In any case, draw whatever we have (which might be stale/incomplete).

    const limits = this.limits;
    const data = this.counters;

    if (data.timestamps.length === 0 || limits === undefined) {
      return;
    }

    assertTrue(data.timestamps.length === data.counts.length);
    assertTrue(data.timestamps.length === data.avgValues.length);
    assertTrue(data.timestamps.length === data.minDisplayValues.length);
    assertTrue(data.timestamps.length === data.maxDisplayValues.length);
    assertTrue(data.timestamps.length === data.lastDisplayValues.length);

    const options = this.getCounterOptions();

    const timestamps = data.timestamps;
    const minValues = data.minDisplayValues;
    const maxValues = data.maxDisplayValues;
    const lastValues = data.lastDisplayValues;

    // Choose a range for the y-axis
    const {yRange, yMin, yMax, yLabel} = this.computeYRange(
      limits,
      data.displayValueRange,
    );

    const effectiveHeight = this.getHeight() - MARGIN_TOP;
    const endPx = size.width;
    const hasZero = yMin < 0 && yMax > 0;
    let zeroY = effectiveHeight + MARGIN_TOP;
    if (hasZero) {
      zeroY = effectiveHeight * (yMax / (yMax - yMin)) + MARGIN_TOP;
    }

    // Use hue to differentiate the scale of the counter value
    const exp = Math.ceil(Math.log10(Math.max(yMax, 1)));
    const expCapped = Math.min(exp - 3, 9);
    const hue = (180 - Math.floor(expCapped * (180 / 6)) + 360) % 360;

    ctx.fillStyle = `hsl(${hue}, 45%, 75%)`;
    ctx.strokeStyle = `hsl(${hue}, 45%, 45%)`;

    const calculateX = (ts: time) => {
      return Math.floor(timeScale.timeToPx(ts));
    };
    const calculateY = (value: number) => {
      return (
        MARGIN_TOP +
        effectiveHeight -
        Math.round(((value - yMin) / yRange) * effectiveHeight)
      );
    };

    ctx.beginPath();
    const timestamp = Time.fromRaw(timestamps[0]);
    ctx.moveTo(calculateX(timestamp), zeroY);
    let lastDrawnY = zeroY;
    for (let i = 0; i < timestamps.length; i++) {
      const timestamp = Time.fromRaw(timestamps[i]);
      const x = calculateX(timestamp);
      const minY = calculateY(minValues[i]);
      const maxY = calculateY(maxValues[i]);
      const lastY = calculateY(lastValues[i]);

      ctx.lineTo(x, lastDrawnY);
      if (minY === maxY) {
        assertTrue(lastY === minY);
        ctx.lineTo(x, lastY);
      } else {
        ctx.lineTo(x, minY);
        ctx.lineTo(x, maxY);
        ctx.lineTo(x, lastY);
      }
      lastDrawnY = lastY;
    }
    ctx.lineTo(endPx, lastDrawnY);
    ctx.lineTo(endPx, zeroY);
    ctx.closePath();
    ctx.fill();
    ctx.stroke();

    if (hasZero) {
      // Draw the Y=0 dashed line.
      ctx.strokeStyle = `hsl(${hue}, 10%, 71%)`;
      ctx.beginPath();
      ctx.setLineDash([2, 4]);
      ctx.moveTo(0, zeroY);
      ctx.lineTo(endPx, zeroY);
      ctx.closePath();
      ctx.stroke();
      ctx.setLineDash([]);
    }
    ctx.font = '10px Roboto Condensed';

    const hover = this.hover;
    if (hover !== undefined) {
      let text = `${hover.avgValue.toLocaleString()}`;

      const unit = this.unit;
      switch (options.yMode) {
        case 'value':
          text = `${text} ${unit}`;
          break;
        case 'delta':
          text = `${text} \u0394${unit}`;
          break;
        case 'rate':
          text = `${text} \u0394${unit}/s`;
          break;
        default:
          assertUnreachable(options.yMode);
          break;
      }

      if (hover.count > 1) {
        text += ` (avg of ${hover.count})`;
      }

      ctx.fillStyle = `hsl(${hue}, 45%, 75%)`;
      ctx.strokeStyle = `hsl(${hue}, 45%, 45%)`;

      const xStart = Math.floor(timeScale.timeToPx(hover.ts));
      const xEnd =
        hover.tsEnd === undefined
          ? endPx
          : Math.floor(timeScale.timeToPx(hover.tsEnd));
      const y =
        MARGIN_TOP +
        effectiveHeight -
        Math.round(
          ((hover.lastDisplayValue - yMin) / yRange) * effectiveHeight,
        );

      // Highlight line.
      ctx.beginPath();
      ctx.moveTo(xStart, y);
      ctx.lineTo(xEnd, y);
      ctx.lineWidth = 3;
      ctx.stroke();
      ctx.lineWidth = 1;

      // Draw change marker.
      ctx.beginPath();
      ctx.arc(
        xStart,
        y,
        3 /* r*/,
        0 /* start angle*/,
        2 * Math.PI /* end angle*/,
      );
      ctx.fill();
      ctx.stroke();

      // Draw the tooltip.
      drawTrackHoverTooltip(ctx, this.mousePos, this.getHeight(), text);
    }

    // Write the Y scale on the top left corner.
    ctx.fillStyle = 'rgba(255, 255, 255, 0.6)';
    ctx.fillRect(0, 0, 42, 16);
    ctx.fillStyle = '#666';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText(`${yLabel}`, 5, 14);

    // TODO(hjd): Refactor this into checkerboardExcept
    {
      const counterEndPx = Infinity;
      // Grey out RHS.
      if (counterEndPx < endPx) {
        ctx.fillStyle = '#0000001f';
        ctx.fillRect(counterEndPx, 0, endPx - counterEndPx, this.getHeight());
      }
    }

    // If the cached trace slices don't fully cover the visible time range,
    // show a gray rectangle with a "Loading..." label.
    checkerboardExcept(
      ctx,
      this.getHeight(),
      0,
      size.width,
      timeScale.timeToPx(this.countersKey.start),
      timeScale.timeToPx(this.countersKey.end),
    );
  }

  onMouseMove(pos: {x: number; y: number}) {
    const data = this.counters;
    if (data === undefined) return;
    this.mousePos = pos;
    const {visibleTimeScale} = globals.timeline;
    const time = visibleTimeScale.pxToHpTime(pos.x);

    const [left, right] = searchSegment(data.timestamps, time.toTime());

    if (left === -1) {
      this.hover = undefined;
      return;
    }

    const ts = Time.fromRaw(data.timestamps[left]);
    const tsEnd =
      right === -1 ? undefined : Time.fromRaw(data.timestamps[right]);
    const lastDisplayValue = data.lastDisplayValues[left];
    const count = data.counts[left];
    const avgValue = data.avgValues[left];
    this.hover = {
      ts,
      tsEnd,
      lastDisplayValue,
      count,
      avgValue,
    };
  }

  onMouseOut() {
    this.hover = undefined;
  }

  onDestroy(): void {
    if (this.initState) {
      this.initState.dispose();
      this.initState = undefined;
    }
  }

  // Compute the range of values to display and range label.
  private computeYRange(
    limits: CounterLimits,
    dataLimits: [number, number],
  ): {
    yMin: number;
    yMax: number;
    yRange: number;
    yLabel: string;
  } {
    const options = this.getCounterOptions();

    let yMin = limits.minDisplayValue;
    let yMax = limits.maxDisplayValue;

    if (options.yRange === 'viewport') {
      [yMin, yMax] = dataLimits;
    }

    if (options.yDisplay === 'zero') {
      yMin = Math.min(0, yMin);
    }

    if (options.yOverrideMaximum !== undefined) {
      yMax = Math.max(options.yOverrideMaximum, yMax);
    }

    if (options.yOverrideMinimum !== undefined) {
      yMin = Math.min(options.yOverrideMinimum, yMin);
    }

    if (options.yRangeRounding === 'human_readable') {
      if (options.yDisplay === 'log') {
        yMax = Math.log(roundAway(Math.exp(yMax)));
        yMin = Math.log(roundAway(Math.exp(yMin)));
      } else {
        yMax = roundAway(yMax);
        yMin = roundAway(yMin);
      }
    }

    const sharer = RangeSharer.get();
    [yMin, yMax] = sharer.share(options, [yMin, yMax]);

    let yLabel: string;

    if (options.yDisplay === 'minmax') {
      yLabel = 'min - max';
    } else {
      let max = yMax;
      let min = yMin;
      if (options.yDisplay === 'log') {
        max = Math.exp(max);
        min = Math.exp(min);
      }
      const n = Math.abs(max - min);
      yLabel = toLabel(n);
    }

    const unit = this.unit;
    switch (options.yMode) {
      case 'value':
        yLabel += ` ${unit}`;
        break;
      case 'delta':
        yLabel += `\u0394${unit}`;
        break;
      case 'rate':
        yLabel += `\u0394${unit}/s`;
        break;
      default:
        assertUnreachable(options.yMode);
        break;
    }

    if (options.yDisplay === 'log') {
      yLabel = `log(${yLabel})`;
    }

    return {
      yMin,
      yMax,
      yLabel,
      yRange: yMax - yMin,
    };
  }

  // The underlying table has `ts` and `value` columns.
  private getSqlPreamble(): string {
    const options = this.getCounterOptions();

    let valueExpr;

    switch (options.yMode) {
      case 'value':
        valueExpr = 'value';
        break;
      case 'delta':
        valueExpr = 'lead(value, 1, value) over (order by ts) - value';
        break;
      case 'rate':
        valueExpr =
          '(lead(value, 1, value) over (order by ts) - value) / ((lead(ts, 1, 100) over (order by ts) - ts) / 1e9)';
        break;
      default:
        assertUnreachable(options.yMode);
        break;
    }

    let displayValueExpr = valueExpr;
    if (options.yDisplay === 'log') {
      displayValueExpr = `ifnull(ln(${displayValueExpr}), 0)`;
    }

    return `
      WITH data AS (
        SELECT
          ts,
          ${valueExpr} as value,
          ${displayValueExpr} as displayValue
        FROM (${this.getSqlSource()})
      )
    `;
  }

  private async maybeRequestData(rawCountersKey: CacheKey) {
    let limits = this.limits;
    if (limits === undefined) {
      const maxDurQuery = await this.engine.query(`
        ${this.getSqlPreamble()}
        SELECT
          max(dur) as maxDur
        FROM (
          SELECT
            lead(ts, 1, ts) over (order by ts) - ts as dur
          FROM data
        )
      `);
      const maxDurRow = maxDurQuery.firstRow({
        maxDur: LONG,
      });
      const maxDurNs = maxDurRow.maxDur;

      const displayValueQuery = await this.engine.query(`
        ${this.getSqlPreamble()}
        SELECT
          max(displayValue) as maxDisplayValue,
          min(displayValue) as minDisplayValue
        FROM data
      `);
      const displayValueRow = displayValueQuery.firstRow({
        minDisplayValue: NUM,
        maxDisplayValue: NUM,
      });

      const minDisplayValue = displayValueRow.minDisplayValue;
      const maxDisplayValue = displayValueRow.maxDisplayValue;
      limits = this.limits = {
        minDisplayValue,
        maxDisplayValue,
        maxDurNs,
      };
    }

    if (rawCountersKey.isCoveredBy(this.countersKey)) {
      return; // We have the data already, no need to re-query.
    }

    const countersKey = rawCountersKey.normalize();
    if (!rawCountersKey.isCoveredBy(countersKey)) {
      throw new Error(
        `Normalization error ${countersKey.toString()} ${rawCountersKey.toString()}`,
      );
    }

    const maybeCachedCounters = this.cache.lookup(countersKey);
    if (maybeCachedCounters) {
      this.countersKey = countersKey;
      this.counters = maybeCachedCounters;
      return;
    }

    const bucketNs = countersKey.bucketSize;

    const constraint = constraintsToQuerySuffix({
      filters: [
        `ts >= ${countersKey.start} - ${limits.maxDurNs}`,
        `ts <= ${countersKey.end}`,
        `value is not null`,
      ],
      groupBy: ['tsq'],
      orderBy: ['tsq'],
    });

    const queryRes = await this.engine.query(`
      ${this.getSqlPreamble()}
      SELECT
        (ts + ${bucketNs / 2n}) / ${bucketNs} * ${bucketNs} as tsq,
        count(value) as count,
        avg(value) as avgValue,
        min(displayValue) as minDisplayValue,
        max(displayValue) as maxDisplayValue,
        value_at_max_ts(ts, displayValue) as lastDisplayValue
      FROM data
      ${constraint}
    `);

    const it = queryRes.iter({
      tsq: LONG,
      count: NUM,
      avgValue: NUM,
      minDisplayValue: NUM,
      maxDisplayValue: NUM,
      lastDisplayValue: NUM,
    });

    const numRows = queryRes.numRows();
    const data: CounterData = {
      timestamps: new BigInt64Array(numRows),
      counts: new Uint32Array(numRows),
      avgValues: new Float64Array(numRows),
      minDisplayValues: new Float64Array(numRows),
      maxDisplayValues: new Float64Array(numRows),
      lastDisplayValues: new Float64Array(numRows),
      displayValueRange: [0, 0],
    };

    let min = 0;
    let max = 0;
    for (let row = 0; it.valid(); it.next(), row++) {
      const ts = Time.fromRaw(it.tsq);
      data.timestamps[row] = ts;
      data.counts[row] = it.count;
      data.avgValues[row] = it.avgValue;
      data.minDisplayValues[row] = it.minDisplayValue;
      data.maxDisplayValues[row] = it.maxDisplayValue;
      data.lastDisplayValues[row] = it.lastDisplayValue;
      min = Math.min(min, it.minDisplayValue);
      max = Math.max(max, it.maxDisplayValue);
    }

    data.displayValueRange = [min, max];

    this.cache.insert(countersKey, data);
    this.countersKey = countersKey;
    this.counters = data;

    raf.scheduleRedraw();
  }

  get unit(): string {
    return this.getCounterOptions().unit ?? '';
  }
}
