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
        const selection = globals.state.currentSelection;
        const isHovered = false;
        const isSelected = selection !== null
        const strokeWidth = isSelected ? 3 : 0;
        this.drawMarker(
            ctx,
            tsStart,
            this.centerY,
            isHovered,
            strokeWidth,
            0);

        const callsiteId = 0;
        ctx.fillStyle = colorForSample(callsiteId, false);
        ctx.fillRect(2, MARGIN_TOP, 2, BAR_HEIGHT);
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
  
    async onRtuxLoad(ctx: PluginContextTrace): Promise<void> {
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