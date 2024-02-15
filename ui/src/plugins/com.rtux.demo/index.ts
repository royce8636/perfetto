import {
    Plugin,
    PluginContext,
    PluginContextTrace,
    PluginDescriptor,
    Track,
  } from '../../public';
import { PanelSize } from 'src/frontend/panel';
import { globals } from 'src/frontend/globals';
// import {time, Time} from '../../base/time';
import {RTUX_common} from '../../common/RTUX_common';

// import {searchSegment} from '../../base/binary_search';
// import {colorForCpu} from '../../common/colorizer';
// import {drawTrackHoverTooltip} from '../../common/canvas_utils';
// import {checkerboardExcept} from '../../frontend/checkerboard';

// 0.5 Makes the horizontal lines sharp.
// const MARGIN_TOP = 4.5;
// const RECT_HEIGHT = 20;

// class RTUXTrack implements Track {
//     private mousePos = {x: 0, y: 0};
//     private hoveredValue: number|undefined = undefined;
//     private hoveredTs: time|undefined = undefined;
//     private hoveredTsEnd: time|undefined = undefined;
//     private hoveredIdle: number|undefined = undefined;
//     getHeight() {
//         return MARGIN_TOP + RECT_HEIGHT - 1;
//       }
//     render(ctx: CanvasRenderingContext2D, size: PanelSize): void {
//         // TODO: fonts and colors should come from the CSS and not hardcoded here.
//         const {
//           visibleTimeScale,
//           visibleWindowTime,
//         } = globals.timeline;
//         const data = RTUX_common.getStoredVector();
//         const timestamps = RTUX_common.getTimestamps();
    
//         const endPx = size.width;
//         const zeroY = MARGIN_TOP + RECT_HEIGHT;
    
//         // Quantize the Y axis to quarters of powers of tens (7.5K, 10K, 12.5K).
//         // let yMax = data.maximumValue;
//         // const kUnits = ['', 'K', 'M', 'G', 'T', 'E'];
//         // const exp = Math.ceil(Math.log10(Math.max(yMax, 1)));
//         // const pow10 = Math.pow(10, exp);
//         // yMax = Math.ceil(yMax / (pow10 / 4)) * (pow10 / 4);
//         // const unitGroup = Math.floor(exp / 3);
//         // const num = yMax / Math.pow(10, unitGroup * 3);
//         // // The values we have for cpufreq are in kHz so +1 to unitGroup.
//         // const yLabel = `${num} ${kUnits[unitGroup + 1]}Hz`;
    
//         // Draw the CPU frequency graph.
//         const color = colorForCpu(0);
//         let saturation = 45;
//         if (globals.state.hoveredUtid !== -1) {
//           saturation = 0;
//         }
    
//         ctx.fillStyle = color.setHSL({s: saturation, l: 70}).cssString;
//         ctx.strokeStyle = color.setHSL({s: saturation, l: 55}).cssString;
    
//         const calculateX = (timestamp: time) => {
//           return Math.floor(visibleTimeScale.timeToPx(timestamp));
//         };
//         // const calculateY = (value: number) => {
//         // //   return zeroY - Math.round((value / yMax) * RECT_HEIGHT);
//         // };
//         // const calculateY {
//         //     return zeroY;
//         // };
    
//         const start = visibleWindowTime.start;
//         const end = visibleWindowTime.end;
//         const [rawStartIdx] = searchSegment(timestamps, start.toTime());
//         const startIdx = rawStartIdx === -1 ? 0 : rawStartIdx;
    
//         const [, rawEndIdx] = searchSegment(timestamps, end.toTime());
//         const endIdx = rawEndIdx === -1 ? timestamps.length : rawEndIdx;
    
//         ctx.beginPath();
//         const timestamp = Time.fromRaw(timestamps[startIdx]);
//         ctx.moveTo(Math.max(calculateX(timestamp), 0), zeroY);
    
//         let lastDrawnY = zeroY;
//         for (let i = startIdx; i < endIdx; i++) {
//           const timestamp = Time.fromRaw(data.timestamps[i]);
//           const x = calculateX(timestamp);
    
//         //   const minY = calculateY(data.minFreqKHz[i]);
//         //   const maxY = calculateY(data.maxFreqKHz[i]);
//         //   const lastY = calculateY(data.lastFreqKHz[i]);
//         const minY = zeroY;
//         const maxY = 10;
//         const lastY = 5;
    
//           ctx.lineTo(x, lastDrawnY);
//           if (minY === maxY) {
//             ctx.lineTo(x, lastY);
//           } else {
//             ctx.lineTo(x, minY);
//             ctx.lineTo(x, maxY);
//             ctx.lineTo(x, lastY);
//           }
//           lastDrawnY = lastY;
//         }
//         // Find the end time for the last frequency event and then draw
//         // down to zero to show that we do not have data after that point.
//         const finalX = Math.min(calculateX(data.maxTsEnd), endPx);
//         ctx.lineTo(finalX, lastDrawnY);
//         ctx.lineTo(finalX, zeroY);
//         ctx.lineTo(endPx, zeroY);
//         ctx.closePath();
//         ctx.fill();
//         ctx.stroke();
    
//         // Draw CPU idle rectangles that overlay the CPU freq graph.
//         ctx.fillStyle = `rgba(240, 240, 240, 1)`;
    
//         for (let i = startIdx; i < endIdx; i++) {
//           if (data.lastIdleValues[i] < 0) {
//             continue;
//           }
    
//           // We intentionally don't use the floor function here when computing x
//           // coordinates. Instead we use floating point which prevents flickering as
//           // we pan and zoom; this relies on the browser anti-aliasing pixels
//           // correctly.
//           const timestamp = Time.fromRaw(data.timestamps[i]);
//           const x = visibleTimeScale.timeToPx(timestamp);
//           const xEnd = i === data.lastIdleValues.length - 1 ?
//             finalX :
//             visibleTimeScale.timeToPx(Time.fromRaw(data.timestamps[i + 1]));
    
//           const width = xEnd - x;
//           const height = calculateY(data.lastFreqKHz[i]) - zeroY;
    
//           ctx.fillRect(x, zeroY, width, height);
//         }
    
//         ctx.font = '10px Roboto Condensed';
    
//         if (this.hoveredValue !== undefined && this.hoveredTs !== undefined) {
//           let text = `${this.hoveredValue.toLocaleString()}kHz`;
    
//           ctx.fillStyle = color.setHSL({s: 45, l: 75}).cssString;
//           ctx.strokeStyle = color.setHSL({s: 45, l: 45}).cssString;
    
//           const xStart = Math.floor(visibleTimeScale.timeToPx(this.hoveredTs));
//           const xEnd = this.hoveredTsEnd === undefined ?
//             endPx :
//             Math.floor(visibleTimeScale.timeToPx(this.hoveredTsEnd));
//           const y = zeroY - Math.round((this.hoveredValue / yMax) * RECT_HEIGHT);
    
//           // Highlight line.
//           ctx.beginPath();
//           ctx.moveTo(xStart, y);
//           ctx.lineTo(xEnd, y);
//           ctx.lineWidth = 3;
//           ctx.stroke();
//           ctx.lineWidth = 1;
    
//           // Draw change marker.
//           ctx.beginPath();
//           ctx.arc(
//             xStart, y, 3 /* r*/, 0 /* start angle*/, 2 * Math.PI /* end angle*/);
//           ctx.fill();
//           ctx.stroke();
    
//           // Display idle value if current hover is idle.
//           if (this.hoveredIdle !== undefined && this.hoveredIdle !== -1) {
//             // Display the idle value +1 to be consistent with catapult.
//             text += ` (Idle: ${(this.hoveredIdle + 1).toLocaleString()})`;
//           }
    
//           // Draw the tooltip.
//           drawTrackHoverTooltip(ctx, this.mousePos, this.getHeight(), text);
//         }
    
//         // Write the Y scale on the top left corner.
//         ctx.textBaseline = 'alphabetic';
//         ctx.fillStyle = 'rgba(255, 255, 255, 0.6)';
//         ctx.fillRect(0, 0, 42, 18);
//         ctx.fillStyle = '#666';
//         ctx.textAlign = 'left';
//         ctx.fillText(`${yLabel}`, 4, 14);
    
//         // If the cached trace slices don't fully cover the visible time range,
//         // show a gray rectangle with a "Loading..." label.
//         checkerboardExcept(
//           ctx,
//           this.getHeight(),
//           0,
//           size.width,
//           visibleTimeScale.timeToPx(data.start),
//           visibleTimeScale.timeToPx(data.end));
//       }
// }

import {colorForSample} from '../../common/colorizer';
// import {TrackData} from '../../common/track_data';

const BAR_HEIGHT = 3;
const MARGIN_TOP = 4.5;
const RECT_HEIGHT = 30.5;

// interface Data extends TrackData {
//     ids: Float64Array;
//     tsStarts: BigInt64Array;
//     callsiteId: Uint32Array;
//   }

class RTUXTrack implements Track {
    private centerY = this.getHeight() / 2 + BAR_HEIGHT;
    private markerWidth = (this.getHeight() - MARGIN_TOP - BAR_HEIGHT) / 2;
    // private hoveredTs: time|undefined = undefined;

    // async onBoundsChange(start: time, end: time, resolution: duration):
    //         Promise<Data> {
    //     // const query = `select
    //     //     id,
    //     //     ts,
    //     //     callsite_id as callsiteId
    //     //     from cpu_profile_stack_sample
    //     //     where utid = ${this.utid}
    //     //     order by ts`;

    //     // const result = await this.engine.query(query);
    //     // const numRows = result.numRows();
    //     const data: Data = {
    //         start,
    //         end,
    //         resolution,
    //         length: 1,
    //         ids: new Float64Array(1),
    //         tsStarts: new BigInt64Array(1),
    //         callsiteId: new Uint32Array(1),
    //     };

    //     for (let row = 0; it.valid(); it.next(), ++row) {
    //         data.ids[row] = it.id;
    //         data.tsStarts[row] = it.ts;
    //         data.callsiteId[row] = it.callsiteId;
    //     }

    //     return data;
    // }

    getHeight() {
        return MARGIN_TOP + RECT_HEIGHT - 1;
      }
    render(ctx: CanvasRenderingContext2D, _size: PanelSize): void {
        // const {
        // visibleTimeScale: timeScale,
        // } = globals.timeline;
        const data = RTUX_common.getStoredVector();

        if (data === undefined) return;

        const timestamps = RTUX_common.getTimestamps();
        const tsStart = Math.min(...Array.from(timestamps));
        // const centerX = tsStart;
        const selection = globals.state.currentSelection;
        const isHovered = false;
        // const isSelected = selection !== null &&
            // selection.kind === 'CPU_PROFILE_SAMPLE' && selection.ts === centerX;
        const isSelected = selection !== null
            const strokeWidth = isSelected ? 3 : 0;
        this.drawMarker(
            ctx,
            tsStart,
            this.centerY,
            isHovered,
            strokeWidth,
            0);

        // Group together identical identical CPU profile samples by connecting them
        // with an horizontal bar.
        let clusterStartIndex = 0;
        const callsiteId = 0;

        // Find the end of the cluster by searching for the next different CPU
        // sample. The resulting range [clusterStartIndex, clusterEndIndex] is
        // inclusive and within array bounds.
        let clusterEndIndex = clusterStartIndex;

        // If there are multiple CPU samples in the cluster, draw a line.
        if (clusterStartIndex !== clusterEndIndex) {
            // const startX = Time.fromRaw(data.tsStarts[clusterStartIndex]);
            // const endX = Time.fromRaw(data.tsStarts[clusterEndIndex]);
            // const leftPx = timeScale.timeToPx(startX) - this.markerWidth;
            // const rightPx = timeScale.timeToPx(endX) + this.markerWidth;
            // const width = rightPx - leftPx;
            // ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
            ctx.fillStyle = colorForSample(callsiteId, false);
            ctx.fillRect(2, MARGIN_TOP, 2, BAR_HEIGHT);
        }

        // Move to the next cluster.
        clusterStartIndex = clusterEndIndex + 1;
    }
    drawMarker(
        ctx: CanvasRenderingContext2D, x: number, y: number, isHovered: boolean,
        strokeWidth: number, callsiteId: number): void {
        ctx.beginPath();
        ctx.moveTo(x - this.markerWidth, y - this.markerWidth);
        ctx.lineTo(x, y + this.markerWidth);
        ctx.lineTo(x + this.markerWidth, y - this.markerWidth);
        ctx.lineTo(x - this.markerWidth, y - this.markerWidth);
        ctx.closePath();
        ctx.fillStyle = colorForSample(callsiteId, isHovered);
        // ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
        ctx.fill();
        if (strokeWidth > 0) {
          ctx.strokeStyle = 'rgba(0, 0, 0, 0.3)';
          ctx.lineWidth = strokeWidth;
          ctx.stroke();
        }
      }

}

class RTUX implements Plugin {
    onActivate(_ctx: PluginContext): void {}
  
    async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
    //   const result = await ctx.engine.query(`
    //     select
    //       utid,
    //       tid,
    //       upid,
    //       thread.name as threadName
    //     from
    //       thread
    //       join (select utid
    //           from cpu_profile_stack_sample group by utid
    //       ) using(utid)
    //       left join process using(upid)
    //     where utid != 0
    //     group by utid`);
  
    //   const it = result.iter({
    //     utid: NUM,
    //     upid: NUM_NULL,
    //     tid: NUM_NULL,
    //     threadName: STR_NULL,
    //   });
    ctx.registerTrack({
        uri: 'com.rtux.demo.RTUXTrack',
        displayName: 'RTUX Events',
        // kind: CPU_PROFILE_TRACK_KIND,
        // utid,
        trackFactory: () => new RTUXTrack(),
    });
  
    //   ctx.registerDetailsPanel({
    //     render: (sel) => {
    //       if (sel.kind === 'CPU_PROFILE_SAMPLE') {
    //         return m(CpuProfileDetailsPanel);
    //       } else {
    //         return undefined;
    //       }
    //     },
    //   });
    }
  }

  export const plugin: PluginDescriptor = {
    pluginId: 'com.rtux.demo',
    plugin: RTUX,
  };