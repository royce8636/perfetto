import {
    // EngineProxy,
    Plugin,
    PluginContext,
    PluginContextTrace,
    PluginDescriptor,
    Track,
  } from '../../public';
import { PanelSize } from 'src/frontend/panel';
import {PrimaryTrackSortKey} from '../../public';
import { rtux_loader } from '../../frontend/rtux_loader';
import { TimelineFetcher } from '../../common/track_helper';
import {globals} from '../../frontend/globals';
import {time, duration, Time} from '../../base/time';
import {TrackData} from '../../common/track_data';
import {checkerboardExcept_debug} from '../../frontend/checkerboard';
// import {checkerboardExcept} from '../../frontend/checkerboard';

export interface Data extends TrackData {
  timestamps: BigInt64Array;
  names: string[];
}

const MARGIN = 2;
const RECT_HEIGHT = 18;
const TRACK_HEIGHT = (RECT_HEIGHT) + (2 * MARGIN);

interface Grouped {
  [key: string]: string[];
}

class RTUXTrack implements Track {

  private fetcher = new TimelineFetcher(this.onBoundsChange.bind(this));

  async onUpdate(): Promise<void> {
    await this.fetcher.requestDataForCurrentTime();
  }

  async onDestroy?(): Promise<void> {
    this.fetcher.dispose();
  }

  async onBoundsChange(start: time, end: time, resolution: duration): Promise<Data> {
    // const excludeList = Array.from(globals.state.ftraceFilter.excludedNames);
  
    // Access the vector using the provided function
    const inputVector = rtux_loader.getStoredVector();
  
    // Filter the vector based on start, end, and excluded names
    const filteredVector = inputVector.filter(item =>
      item.key >= start &&
      item.key <= end
      // !excludeList.includes(item.value)
    );
  
    // Initialize grouped with the explicit type
    const grouped: Grouped = {};

    filteredVector.forEach(({ key, value }) => {
      // Convert the key to a string for consistent object indexing
      const tsQuantKey = ((key / resolution) * resolution).toString(); // Ensure arithmetic is valid for BigInt

      if (!grouped[tsQuantKey]) {
        grouped[tsQuantKey] = [];
      }
      grouped[tsQuantKey].push(value);
    });

    // Prepare the result object
    // Convert keys back to BigInt for timestamps array
    const timestamps = Object.keys(grouped).map(ts => BigInt(ts));
    const names = Object.values(grouped).flat(); // Flattening names if multiple per tsQuant

    const result: Data = {
      start: start,
      end: end,
      resolution: resolution,
      length: timestamps.length,
      timestamps: new BigInt64Array(timestamps),
      names,
    };

    return result;
  }
  

    getHeight() {
        return TRACK_HEIGHT;
    }
    render(ctx: CanvasRenderingContext2D, size: PanelSize): void {
        const {
          visibleTimeScale,
        } = globals.timeline;

        const data = this.fetcher.data;
        
        if (data === undefined) return;

        const dataStartPx = visibleTimeScale.timeToPx(data.start);
        const dataEndPx = visibleTimeScale.timeToPx(data.end);

        // checkerboardExcept(
        //   ctx, this.getHeight(), 0, size.width, dataStartPx, dataEndPx);
        checkerboardExcept_debug(
            ctx, this.getHeight(), 0, size.width, dataStartPx, dataEndPx);
    
        const diamondSideLen = RECT_HEIGHT / Math.sqrt(2);

        for (let i = 0; i < data.timestamps.length; i++) {
          // const name = data.names[i];
          ctx.fillStyle = 'rgb(255, 0, 0)';
          const timestamp = Time.fromRaw(data.timestamps[i]);
          const xPos = Math.floor(visibleTimeScale.timeToPx(timestamp));
    
          // Draw a diamond over the event
          ctx.save();
          ctx.translate(xPos, MARGIN);
          ctx.rotate(Math.PI / 4);
          ctx.fillRect(0, 0, diamondSideLen, diamondSideLen);
          ctx.restore();
        }
    }
}
  
  
  class RTUX implements Plugin {
      onActivate(_ctx: PluginContext): void {}
    
      // async onActivate(ctx: PluginContextTrace): Promise<void> {
    //   async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
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
        // ctx.registerTab({
        //   uri: 'com.rtux.track#RTUXTab',
        //   content: new RTUXTab(),
        // });
      }
    }
  
    export const plugin: PluginDescriptor = {
      pluginId: 'dev.rtux.track',
      plugin: RTUX,
    };