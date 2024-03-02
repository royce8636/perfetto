import {Time, time} from '../base/time';
// import m from 'mithril';

import {
    FtraceStat,
    RtuxEvent,
} from './globals';
import { publishRtuxCounters, publishRtuxPanelData } from './publish';

let globalVector: Array<{ key: time, value: string }> = [];
// let photoInfo: Array<{image_path: string, time: time}> = [];
let photoInfo: Array < [time, Array<{ image_path: string; time: any }> ]> = [];
// let photo_directory: string = "";
let imageToDiplay: string = "";

function getStoredVector(): Array<{ key: time, value: string }> {
    return globalVector;
}

function getPhotoInfo(): Array<[time, Array<{ image_path: string; time: any }> ]> {
    return photoInfo;
}

function getImageToDisplay(): string {
    return imageToDiplay;
}

function setImageToDisplay(images: string): void {
    imageToDiplay = images;
    // m.redraw();
    console.log("imageToDiplay", imageToDiplay);
}

function roundToSignificantFigures(num: time, n: number): time {
    const raw_num = Number(num);
    const exponent = Math.floor(Math.log10(Math.abs(raw_num)));
    const scaled = raw_num / Math.pow(10, exponent - n + 1);
    const rounded = Math.round(scaled) * Math.pow(10, exponent - n + 1);
    return Time.fromRaw(BigInt(rounded));
  }

function findImageInfo(timestamp: time): any{
    if (photoInfo === undefined || photoInfo.length === 0) {
        console.log("photo_info is undefined or empty")
        return undefined;
      }
      const rounted_time = roundToSignificantFigures(timestamp, 6);
      const matchingEntry = photoInfo.find(([key, _]) => key === rounted_time);
      if (!matchingEntry) {
        console.log("matchingEntry is undefined", rounted_time)
        return undefined;
      }
      const [, image_info] = matchingEntry;
      let closest = image_info[0];
      let closest_time = Math.abs(Number(closest.time) - Number(timestamp));
      image_info.forEach((info) => {
        const diff = Math.abs(Number(info.time) - Number(timestamp));
        if (diff < closest_time) {
          closest = info;
          closest_time = diff;
        }
      });
      return closest;
}

function readJsonFile(file: File): Promise<{
    vector: Array<{ key: time, value: string }>,
    photoVector: Array<[time, Array<{ image_path: string; time: any }> ]>
    }> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            const str = reader.result as string;
            const json = JSON.parse(str);

            let vector: Array<{ key: time, value: string }> = [];
            let photoVector: Array<[time, Array<{ image_path: string; time: any }>]> = [];

            // Process "summary" section if it exists
            if (json.summary) {
                let realtime;
                try {
                    vector = Object.entries(json.summary).map(([event, data]: [string, any]) => {
                        realtime = BigInt(Math.floor(data.CLOCK_REALTIME * 1e9));
                        return { key: Time.fromRaw(realtime), value: event };
                    });
                } catch(e) {
                    vector = Object.entries(json.summary).map(([event, ]: [string, any]) => {
                        return { key: Time.INVALID, value: event };
                    });
                }
                
                // Process counters for Rtux
                const counters: FtraceStat[] = [];
                let cnt = 0;
                for (const event in json.summary) {
                    counters.push({name: event, count: cnt});
                    cnt++;
                }
                publishRtuxCounters(counters);
                
                const events: RtuxEvent[] = [];
                vector.forEach(({key, value}) => {
                    events.push({ts: key, event: value});
                });
                publishRtuxPanelData({events, offset: 0, numEvents: cnt});
            }

            // Process "rounded_photo_info" section if it exists
            if (json.rounded_photo_info) {
                Object.entries(json.rounded_photo_info).forEach(([timestamp, value]: [any, any]) => {
                    timestamp = Time.fromRaw(BigInt(Math.floor(parseFloat(timestamp) * 1e9)));
                    const imagesForTimestamp: Array<{ image_path: string; time: any }> = [];
                    
                    Object.entries(value).forEach(([imagePath, imageTime]: [string, any]) => {
                        imagesForTimestamp.push({
                            image_path: imagePath,
                            time: Time.fromRaw(BigInt(Math.floor(parseFloat(imageTime) * 1e9))),
                        });
                    });
                    
                    photoVector.push([timestamp, imagesForTimestamp]);
                });
            }

            // Resolve the promise with both vectors and photoInfo
            globalVector = vector;
            photoInfo = photoVector;
            resolve({ vector, photoVector });
        };
        reader.onerror = () => {
            reject(reader.error);
        };
        reader.readAsText(file);
    });
}


// Optionally, adjust the structure to encapsulate the operations
export const rtux_loader = {
    getStoredVector, // Allow access to the stored vector
    getPhotoInfo,
    getImageToDisplay,
    setImageToDisplay,
    findImageInfo,
    openJsonRtuxFromFile: async (file: File) => {
        await readJsonFile(file);
    },
};
