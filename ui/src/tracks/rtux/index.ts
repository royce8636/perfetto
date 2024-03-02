import {
    // EngineProxy,
    Plugin,
    PluginContext,
    PluginContextTrace,
    PluginDescriptor,
    Track,
  } from '../../public';
import { PanelSize } from 'src/frontend/panel';
// import {PrimaryTrackSortKey} from '../../public';
import { rtux_loader } from '../../frontend/rtux_loader';
import { TimelineFetcher } from '../../common/track_helper';
import {globals} from '../../frontend/globals';
import {time, duration, Time} from '../../base/time';
import {TrackData} from '../../common/track_data';
import {checkerboardExcept_debug} from '../../frontend/checkerboard';
import {RTUXPanel} from '../../frontend/rtux_panel';
import { RTUXDetailsTab} from '../../frontend/rtux_detail_panel';
import {Actions} from '../../common/actions';
// import {search} from '../../base/binary_search';
// import { drawRtuxHoverScreen, drawTrackHoverTooltip } from '../../common/canvas_utils';
import { drawTrackHoverTooltip } from '../../common/canvas_utils';
// import {checkerboardExcept} from '../../frontend/checkerboard';


export interface Data extends TrackData {
  timestamps: BigInt64Array;
  names: string[];
}

const MARGIN = 2;
const RECT_HEIGHT = 35;
const TRACK_HEIGHT = (RECT_HEIGHT) + (2 * MARGIN);

interface Grouped {
  [key: string]: string[];
}

class RTUXTrack implements Track {
  // private trackKey: string;

  // constructor(trackKey: string) {
    // this.trackKey = trackKey;
  // }

  private mousePos? = {x: 0, y: 0};

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

        // rtux_loader.getPhotoInfo().then((photo_array) => {
        //   if (this.mousePos !== undefined){
        //     drawRtuxHoverScreen(ctx, this.mousePos, this.getHeight(), photo_array);
        //   }
        // });
        // const photoInfo = rtux_loader.getPhotoInfo();
        if (this.mousePos !== undefined){
          const timeToFind = visibleTimeScale.pxToHpTime(this.mousePos.x).toTime();
          const imageInfo = rtux_loader.findImageInfo(timeToFind);
          if (imageInfo){
            let image_path = `${globals.root}assets${imageInfo.image_path}`;
            rtux_loader.setImageToDisplay(image_path);
          }
        }

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

          if (this.mousePos !== undefined && xPos - diamondSideLen / 2 < this.mousePos.x && this.mousePos.x < xPos + diamondSideLen / 2){
            drawTrackHoverTooltip(ctx, this.mousePos, this.getHeight(), data.names[i]);
          }
          // if (this.mousePos !== undefined){
          //   drawRtuxHoverScreen(ctx, this.mousePos, this.getHeight(), photo_array);
          // }

        }

    }

      
    onMouseMove(pos: { x: number; y: number; }): void {
      this.mousePos = pos;

      const {
        visibleTimeScale,
      } = globals.timeline;
      if (this.mousePos !== undefined){
        const timeToFind = visibleTimeScale.pxToHpTime(this.mousePos.x).toTime();
        const imageInfo = rtux_loader.findImageInfo(timeToFind);
        if (imageInfo){
          let image_path = `${globals.root}assets${imageInfo.image_path}`;
          rtux_loader.setImageToDisplay(image_path);
        }
      }
    }

    onMouseOut(){
      this.mousePos = undefined;
    }


    // onMouseClick({x}: { x: number;}) {
    onMouseClick({x}: { x: number; y: number; }): boolean {
      const data = this.fetcher.data;
      if (data === undefined) return false;

      // const time = visibleTimeScale.pxToHpTime(x);
      // const index = search(data.timestamps, time.toTime());
      // const id = index === -1 ? undefined : 0;
      // if (!id) return false;
      // globals.makeSelection(Actions.showTab({uri: 'dev.rtux.track#RTUXDetailTab'}))
      // globals.makeSelection(Actions.selectRTUX({id, trackKey: this.trackKey}));
      const diamondSideLen = RECT_HEIGHT / Math.sqrt(2);
      const {visibleTimeScale} = globals.timeline;

      for (let i = 0; i < data.timestamps.length; i++) {
        const timestamp = Time.fromRaw(data.timestamps[i]);
        const xPos = Math.floor(visibleTimeScale.timeToPx(timestamp));
        if (x !== undefined && xPos - diamondSideLen / 2 < x && x < xPos + diamondSideLen / 2){
          Actions.showTab({uri: 'dev.rtux.track#RTUXDetailTab'});
          // globals.makeSelection(Actions.selectRTUX({id: 0}));
          return true;
        }
      }
      return false;
    }
}
  
  
class RTUX implements Plugin {
  onActivate(_ctx: PluginContext): void {}
  
  async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
    ctx.registerTrack({
      uri: 'dev.rtux.track#RTUXTrack',
      displayName: 'RTUX Events',
      kind: 'TRACK_KIND',
      // trackFactory: ({trackKey}) => {
      //   return new RTUXTrack(trackKey);
      // },
      trackFactory: () => new RTUXTrack(),
    });

    // ctx.registerDetailsPanel({
    //   render: (sel) => {
    //     if (sel.kind === 'RTUX') {
    //       return m(RtuxDetailpanel);
    //     }
    //   },
    // });

    ctx.registerCommand({
      id: 'dev.rtux.track.AddRTUXTrackCommand',
      name: 'Add RTUX track',
      callback: () => {
        ctx.timeline.addTrack(
          'dev.rtux.track#RTUXTrack',
          'RTUX Events',
        );
      },
    });

    const rtuxTabUri = 'dev.rtux.track#RTUXTab';
    ctx.registerTab({
      uri: rtuxTabUri,
      isEphemeral: true,
      content: {
        render: () => m(RTUXPanel),
        getTitle: () => 'RTUX Events',
      },
    });
    ctx.addDefaultTab(rtuxTabUri);

    const rtuxDetailTabUri = 'dev.rtux.track#RTUXDetailTab';
    ctx.registerTab({
      uri: rtuxDetailTabUri,
      isEphemeral: true,
      content: {
        render: () => m(RTUXDetailsTab),
        getTitle: () => 'RTUX Details Tab',
      },
    });
    ctx.addDefaultTab(rtuxDetailTabUri);
  }
}
  
export const plugin: PluginDescriptor = {
  pluginId: 'dev.rtux.track',
  plugin: RTUX,
};