import { Time, time } from '../base/time';
import { publishRtuxCounters, publishRtuxPanelData } from './publish';
import {
    FtraceStat,
    RtuxEvent,
    globals,
} from './globals';
import {Actions} from '../common/actions';
// import { getContainingTrackId } from 'src/common/state';
// import { Num } from 'src/common/event_set';
// Define the types for clarity
type PhotoInfo = Array<[time, Array<{ image_path: string; time: any }> ]>;
type Vector = Array<{ key: time, value: string }>;

class RtuxLoader {
    private imageToDisplay: string = "";
    private imageDisplayedTime: time = Time.INVALID;
    // private subscribers: ((imagePath: string) => void)[] = [];
    public photoInfo: PhotoInfo = [];
    private globalVector: Vector = [];
    private oldImage: string = "";
    private minKey: number = -1;
    private minStep: number = -1;
    // private mouseX: number = 0;
    // private trackGroupId = getContainingTrackId(globals.state, );
    // private tracksContainer = document.querySelectorAll('#track_');


    getStoredVector(): Vector {
        return this.globalVector;
    }

    getPhotoInfo(): PhotoInfo {
        return this.photoInfo;
    }

    roundToSignificantFigures(num: time, n: number): time {
        const raw_num = Number(num);
        const exponent = Math.floor(Math.log10(Math.abs(raw_num)));
        const scaled = raw_num / Math.pow(10, exponent - n + 1);
        const rounded = Math.round(scaled) * Math.pow(10, exponent - n + 1);
        return Time.fromRaw(BigInt(rounded));
    }

    findImageInfo(timestamp: time): any {
        if (this.photoInfo === undefined || this.photoInfo.length === 0) {
            console.log("photo_info is undefined or empty");
            return undefined;
        }
        const rounded_time = this.roundToSignificantFigures(timestamp, 6);
        let matchingEntry;
        if (this.minKey === -1 || this.minStep === -1){  // if hash is not available
            // console.log("hash is not available")
            matchingEntry = this.photoInfo.find(([key, _]) => key === rounded_time);
            if (!matchingEntry) {
                console.log("No hash: matchingEntry is undefined");
                return undefined;
            }
        }else{  // if hash is available
            const index = (Number(rounded_time) - this.minKey) / this.minStep;
            // console.log("Hash:", index)
            matchingEntry = this.photoInfo[index];
            if (!matchingEntry) {
                console.log("Hash: matchingEntry is undefined", rounded_time, index);
                return undefined;
            }
            // if (matchingEntry.length === 0){
                // return undefined;
            // }
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

    setImageToDisplay(imagePath: string): void {
        if (this.oldImage !== imagePath) {
            globals.dispatch(Actions.updateRtuxImage({image: imagePath}));
            this.imageToDisplay = imagePath;
            this.oldImage = imagePath;
            // console.log("imageToDisplay", this.imageToDisplay);
        }
        // this.notifySubscribers();
    }

    getImageToDisplay(): string {
        return this.imageToDisplay;
    }

    setImageDisplayedTime(time: time): void {
        this.imageDisplayedTime = time;
    }

    getImageDisplayedTime(): time {
        return this.imageDisplayedTime;
    }

    // subscribe(callback: (imagePath: string) => void): void {
    //     this.subscribers.push(callback);
    // }

    // private notifySubscribers(): void {
    //     this.subscribers.forEach(callback => callback(this.imageToDisplay));
    // }

    async readJsonFile(file: File): Promise<{ vector: Vector, photoVector: PhotoInfo }> {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => {
                const str = reader.result as string;
                const json = JSON.parse(str);

                let vector: Vector = [];
                let photoVector: PhotoInfo = [];

                // Process summary and rounded_photo_info sections...
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
                try{
                    if (json.hash.min_key && json.hash.min_step){
                        this.minKey = json.hash.min_key;
                        this.minStep = json.hash.min_step;
                        console.log("minKey", this.minKey);
                        console.log("minStep", this.minStep);
                    }
                }catch(e){
                    console.log("minKey and minStep not found");
                }

                // Assign processed data to class properties
                this.globalVector = vector;
                this.photoInfo = photoVector;

                // window.addEventListener('mousemove', (event: MouseEvent) => {
                //     this.mouseX = event.clientX;
                //     console.log("rtuxLoader", this.mouseX);
                //     const timeToFind = globals.timeline.visibleTimeScale.pxToHpTime(this.mouseX).toTime();
                //     const imageInfo = this.findImageInfo(timeToFind);
                //     if (imageInfo){
                //         let image_path = `${globals.root}assets${imageInfo.image_path}`;
                //         this.setImageToDisplay(image_path);
                //     }
                //     else{
                //         this.setImageToDisplay("");
                //     }
                //     // this.mousePos = {x, y};
                // });

                // Resolve with the processed data
                resolve({ vector, photoVector });
            };
            reader.onerror = () => {
                reject(reader.error);
            };
            reader.readAsText(file);
        });

    }

    // Add other methods here...
    
}

// Export an instance of the class
export const rtux_loader = new RtuxLoader();
