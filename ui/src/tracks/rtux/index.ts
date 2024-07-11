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
// import {Actions} from '../../common/actions';
// import {search} from '../../base/binary_search';
// import { drawRtuxHoverScreen, drawTrackHoverTooltip } from '../../common/canvas_utils';
import { drawTrackHoverTooltip } from '../../common/canvas_utils';
// import {checkerboardExcept} from '../../frontend/checkerboard';


export interface Data extends TrackData {
  timestamps: BigInt64Array;
  // names: string[];
  events: Array<{ type: string, name: string, level: string }>;
}

const MARGIN = 2;
const RECT_HEIGHT = 35;
const TRACK_HEIGHT = (RECT_HEIGHT) + (2 * MARGIN);

// interface Grouped {
//   [key: string]: string[];
// }

class RTUXTrack implements Track {
  // private trackKey: string;

  // constructor(trackKey: string) {
    // this.trackKey = trackKey;
  // }

  private mousePos? = {x: 0, y: 0};
  private mouseX: number = 0;

  private fetcher = new TimelineFetcher(this.onBoundsChange.bind(this));

  async onCreate(){
    document.addEventListener('mousemove', (event: MouseEvent) => {
      // const element = document.getElementsByClassName('track-content')[0];
      let element;
      try{
        element = document.getElementsByClassName('track-content')[0];
      }
      catch(e){
        return;
      }
      const rect = element.getBoundingClientRect();
      this.mouseX = event.clientX - rect.left;
      // console.log("globalEvent", this.mouseX);
      const timeToFind = globals.timeline.visibleTimeScale.pxToHpTime(this.mouseX).toTime();
      const imageInfo = rtux_loader.findImageInfo(timeToFind);
      if (imageInfo){
          let image_path = `${globals.root}assets${imageInfo.image_path}`;
          rtux_loader.setImageToDisplay(image_path);
          rtux_loader.setImageDisplayedTime(imageInfo.time);
      }
      else{
        rtux_loader.setImageToDisplay("");
      }
      // this.mousePos = {x, y};
  });  }

  async onUpdate(): Promise<void> {
    await this.fetcher.requestDataForCurrentTime();
  }

  async onDestroy?(): Promise<void> {
    this.fetcher.dispose();
  }

  // async onBoundsChange(start: time, end: time, resolution: duration): Promise<Data> {
  //   // const excludeList = Array.from(globals.state.ftraceFilter.excludedNames);
  
  //   // Access the vector using the provided function
  //   const inputVector = rtux_loader.getStoredVector();
  
  //   // Filter the vector based on start, end, and excluded names
  //   const filteredVector = inputVector.filter(item =>
  //     item.key >= start &&
  //     item.key <= end
  //     // !excludeList.includes(item.value)
  //   );
  
  //   // Initialize grouped with the explicit type
  //   const grouped: Grouped = {};

  //   filteredVector.forEach(({ key, value }) => {
  //     // Convert the key to a string for consistent object indexing
  //     const tsQuantKey = ((key / resolution) * resolution).toString(); // Ensure arithmetic is valid for BigInt

  //     if (!grouped[tsQuantKey]) {
  //       grouped[tsQuantKey] = [];
  //     }
  //     grouped[tsQuantKey].push(value);
  //   });

  //   // Prepare the result object
  //   // Convert keys back to BigInt for timestamps array
  //   const timestamps = Object.keys(grouped).map(ts => BigInt(ts));
  //   const names = Object.values(grouped).flat(); // Flattening names if multiple per tsQuant

  //   const result: Data = {
  //     start: start,
  //     end: end,
  //     resolution: resolution,
  //     length: timestamps.length,
  //     timestamps: new BigInt64Array(timestamps),
  //     names,
  //   };

  //   return result;
  // }

  async onBoundsChange(start: time, end: time, resolution: duration): Promise<Data> {
    // Access the vector using the provided function
    const inputVector = rtux_loader.getStoredVector();
  
    // Filter the vector based on start and end
    const filteredVector = inputVector.filter(item =>
      item.key >= start &&
      item.key <= end
    );
  
    // Initialize grouped with the explicit type
    const grouped: { [key: string]: Array<{ type: string, name: string, level: string }> } = {};
  
    filteredVector.forEach(({ key, value }) => {
      // Convert the key to a string for consistent object indexing
      const tsQuantKey = ((key / resolution) * resolution).toString();
  
      if (!grouped[tsQuantKey]) {
        grouped[tsQuantKey] = [];
      }
  
      // Parse the value string
      const [eventType, eventName, level] = value.split(':');
  
      // Create a structured object for each event
      const eventObject = {
        type: eventType,
        name: eventName,
        level: level
      };
  
      grouped[tsQuantKey].push(eventObject);
    });
  
    // Prepare the result object
    // Convert keys back to BigInt for timestamps array
    const timestamps = Object.keys(grouped).map(ts => BigInt(ts));
    const events = Object.values(grouped).flat(); // Flattening events if multiple per tsQuant
  
    const result: Data = {
      start: start,
      end: end,
      resolution: resolution,
      length: timestamps.length,
      timestamps: new BigInt64Array(timestamps),
      events,
    };
  
    return result;
  }

    getHeight() {
        return TRACK_HEIGHT;
    }
    render(ctx: CanvasRenderingContext2D, size: PanelSize): void {
      const { visibleTimeScale } = globals.timeline;
      const data = this.fetcher.data;
      
      if (data === undefined) return;
  
      const dataStartPx = visibleTimeScale.timeToPx(data.start);
      const dataEndPx = visibleTimeScale.timeToPx(data.end);
  
      checkerboardExcept_debug(
        ctx, this.getHeight(), 0, size.width, dataStartPx, dataEndPx);
  
      const diamondSideLen = RECT_HEIGHT / Math.sqrt(2);
  
      if (this.mousePos !== undefined) {
        const timeToFind = visibleTimeScale.pxToHpTime(this.mousePos.x).toTime();
        const imageInfo = rtux_loader.findImageInfo(timeToFind);
        if (imageInfo) {
          let image_path = `${globals.root}assets${imageInfo.image_path}`;
          rtux_loader.setImageToDisplay(image_path);
        }
      }
  
      for (let i = 0; i < data.timestamps.length; i++) {
        const event = data.events[i];
        ctx.fillStyle = event.type === 'detection' ? 'rgb(255, 0, 0)' : 'rgb(0, 0, 255)';
        const timestamp = Time.fromRaw(data.timestamps[i]);
        const xPos = Math.floor(visibleTimeScale.timeToPx(timestamp));
  
        // Draw a diamond over the event
        ctx.save();
        ctx.translate(xPos, MARGIN);
        ctx.rotate(Math.PI / 4);
        ctx.fillRect(0, 0, diamondSideLen, diamondSideLen);
        ctx.restore();
  
        if (this.mousePos !== undefined && xPos - diamondSideLen / 2 < this.mousePos.x && this.mousePos.x < xPos + diamondSideLen / 2) {
          drawTrackHoverTooltip(ctx, this.mousePos, this.getHeight(), `${event.type}: ${event.name} (Level: ${event.level})`);
        }
      }
    }

      
    // onMouseMove(pos: { x: number; y: number; }): void {
      // this.mousePos = pos;
      // console.log("rtuxTrack", this.mousePos);
    //   const {
    //     visibleTimeScale,
    //   } = globals.timeline;
    //   if (this.mousePos !== undefined){
    //     const timeToFind = visibleTimeScale.pxToHpTime(this.mousePos.x).toTime();
    //     const imageInfo = rtux_loader.findImageInfo(timeToFind);
    //     if (imageInfo){
    //       let image_path = `${globals.root}assets${imageInfo.image_path}`;
    //       rtux_loader.setImageToDisplay(image_path);
    //     }
    //     else{
    //       rtux_loader.setImageToDisplay("");
    //     }
    //   }
    // }

    onMouseOut(){
      this.mousePos = undefined;
    }
}
  
  
class RTUX implements Plugin {
  onActivate(_ctx: PluginContext): void {}
  
  async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
    ctx.registerTrack({
      uri: 'dev.rtux.track#RTUXTrack',
      displayName: 'RTUX Events',
      kind: 'TRACK_KIND',
      trackFactory: () => new RTUXTrack(),
    });

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