import m from 'mithril';

import {
    // EngineProxy,
    // Tab,
    Plugin,
    PluginContext,
    PluginContextTrace,
    PluginDescriptor,
    Track,
  } from '../../public';
import { PanelSize } from 'src/frontend/panel';
import {PrimaryTrackSortKey} from '../../public';
import { rtux_loader } from '../../frontend/rtux_loader';
import {LogPanel} from '../../frontend/logs_panel';

  const BAR_HEIGHT = 3;
  const MARGIN_TOP = 4.5;
  const RECT_HEIGHT = 30.5;
  
//   class RTUXTab implements Tab {
//     render(): m.Children {
//       return m('div', 'Hello from my tab');
//     }
  
//     getTitle(): string {
//       return 'RTUX Tab';
//     }
//   }
  
// async function loadRtuxCommon() {
//     const module = await import('../../common/rtux_common');
//     return module.rtux_common;
//   }

class RTUXTrack implements Track {
    // private centerY = this.getHeight() / 2 + BAR_HEIGHT;
    private markerWidth = (this.getHeight() - MARGIN_TOP - BAR_HEIGHT) / 2;

    getHeight() {
        return MARGIN_TOP + RECT_HEIGHT - 1;
    }
    render(ctx: CanvasRenderingContext2D, _size: PanelSize): void {
        // const {
        // visibleTimeScale: timeScale,
        // } = globals.timeline;
        const data = rtux_loader.getStoredVector();
        if (data === undefined) return;

        // const timestamps = rtux_common.getTimestamps();
        // const tsStart = Math.min(...Array.from(timestamps));
        // const selection = globals.state.currentSelection;
        // const isHovered = false;
        // const isSelected = selection !== null
        // const strokeWidth = isSelected ? 3 : 0;
        // this.drawMarker(
        //     ctx,
        //     tsStart,
        //     this.centerY,
        //     isHovered,
        //     strokeWidth,
        //     0);

        // const callsiteId = 0;
        // ctx.fillStyle = colorForSample(callsiteId, false);
        ctx.fillRect(2, MARGIN_TOP, 2, BAR_HEIGHT);
    }
    drawMarker(
        ctx: CanvasRenderingContext2D, x: number, y: number): void {
        ctx.beginPath();
        ctx.moveTo(x - this.markerWidth, y - this.markerWidth);
        ctx.lineTo(x, y + this.markerWidth);
        ctx.lineTo(x + this.markerWidth, y - this.markerWidth);
        ctx.lineTo(x - this.markerWidth, y - this.markerWidth);
        ctx.closePath();
        // ctx.fillStyle = colorForSample(callsiteId, isHovered);
        ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
        ctx.fill();
        // if (strokeWidth > 0) {
        //   ctx.strokeStyle = 'rgba(0, 0, 0, 0.3)';
        //   ctx.lineWidth = strokeWidth;
        //   ctx.stroke();
        }
}
  
  
  class RTUX implements Plugin {
      onActivate(_ctx: PluginContext): void {}
    
      // async onActivate(ctx: PluginContextTrace): Promise<void> {
      async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
        ctx.registerTrack({
            uri: 'dev.rtux.track',
            displayName: 'RTUX Events',
            trackFactory: () => new RTUXTrack(),
        });
        ctx.addDefaultTrack({
            uri: 'dev.rtux.track#RTUXTrack',
            displayName: 'RTUX Events',
            sortKey: PrimaryTrackSortKey.ORDINARY_TRACK,
            // kind: CPU_PROFILE_TRACK_KIND,
            // utid,
            // trackFactory: () => new RTUXTrack(),
        });
    //     ctx.registerTab({
    //       isEphemeral: false,
    //       uri: 'com.rtux.track#RTUXTab',
    //       content: new RTUXTab(),
    //     });
    //     ctx.registerCommand({
    //         id:'dev.rtux.track#ShowRTUXTab',
    //         name: 'Show RTUX Tab',
    //         callback: () => {
    //             ctx.tabs.showTab('dev.rtux.track#RTUXTab');
    //         },
    // });
        ctx.registerTab({
            isEphemeral: false,
            uri: 'dev.rtux.track#RTUXTab',
            content: {
                render: () => m(LogPanel),
                getTitle: () => 'RTUX Events',
            },
        });
        ctx.registerCommand({
            id: 'dev.rtux.track#ShowRTUXTab',
            name: 'Show RTUX Tab',
            callback: () => {
                ctx.tabs.showTab('dev.rtux.track#RTUXTab');
            },
        });
    }
}
  
    export const plugin: PluginDescriptor = {
      pluginId: 'dev.rtux.track',
      plugin: RTUX,
    };