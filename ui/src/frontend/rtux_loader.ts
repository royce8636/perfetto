import { Time, time } from '../base/time';
import { publishRtuxCounters, publishRtuxPanelData } from './publish';
import {
    FtraceStat,
    RtuxEvent,
    globals,
} from './globals';
import {Actions} from '../common/actions';
type PhotoInfo = Array<[time, Array<{ image_path: string; time: any }> ]>;
type Vector = Array<{ key: time, value: string }>;

class RtuxLoader {
    private imageToDisplay: string = "";
    private imageDisplayedTime: time = Time.INVALID;
    public photoInfo: PhotoInfo = [];
    private globalVector: Vector = [];
    private oldImage: string = "";
    private minKey: number = -1;
    private minStep: number = -1;


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

    // findImageInfo(timestamp: time): any {
    //     if (this.photoInfo === undefined || this.photoInfo.length === 0) {
    //         console.log("photo_info is undefined or empty", timestamp);
    //         return undefined;
    //     }
    //     const rounded_time = this.roundToSignificantFigures(timestamp, 6);
    //     let matchingEntry;
    //     if (this.minKey === -1 || this.minStep === -1){  // if hash is not available
    //         // console.log("hash is not available")
    //         matchingEntry = this.photoInfo.find(([key, _]) => key === rounded_time);
    //         if (!matchingEntry) {
    //             console.log("No hash: matchingEntry is undefined");
    //             return undefined;
    //         }
    //     }else{  // if hash is available
    //         const index = Math.floor((Number(rounded_time) - this.minKey) / this.minStep);
    //         // console.log("Hash:", index)
    //         matchingEntry = this.photoInfo[index];
    //         if (!matchingEntry) {
    //             console.log("Hash: matchingEntry is undefined", rounded_time, index);
    //             return undefined;
    //         }
    //     }
    //     const [, image_info] = matchingEntry;
    //     let closest = image_info[0];
    //     let closest_time = Math.abs(Number(closest.time) - Number(timestamp));
    //     image_info.forEach((info) => {
    //         const diff = Math.abs(Number(info.time) - Number(timestamp));
    //         if (diff < closest_time) {
    //             closest = info;
    //             closest_time = diff;
    //         }
    //     });
    //     // log the given time, the closest time, and the image path
    //     console.log("Given time:", timestamp, "Closest time:", closest.time, "Image path:", closest.image_path);
    //     return closest;
    // }

    findImageInfo(timestamp: time): any {
        if (this.photoInfo === undefined || this.photoInfo.length === 0) {
            console.log("photo_info is undefined or empty", timestamp);
            return undefined;
        }
    
        const timestampNs = Number(timestamp);
    
        if (this.minKey !== -1 && this.minStep !== -1) {
            // Use hash-based method
            const index = Math.floor((timestampNs - this.minKey) / this.minStep);
            
            let closestIndex = index;
            let closestDiff = Infinity;
            
            for (let i = Math.max(0, index - 5); i < Math.min(this.photoInfo.length, index + 6); i++) {
                if (this.photoInfo[i]) {
                    const diff = Math.abs(Number(this.photoInfo[i][0]) - timestampNs);
                    if (diff < closestDiff) {
                        closestIndex = i;
                        closestDiff = diff;
                    }
                }
            }
    
            if (!this.photoInfo[closestIndex]) {
                console.log("Hash: matchingEntry is undefined", timestampNs, closestIndex);
                return undefined;
            }
    
            const [, image_info] = this.photoInfo[closestIndex];
    
            let closest = image_info[0];
            let closest_time = Math.abs(Number(closest.time) - timestampNs);
            
            for (const info of image_info) {
                const diff = Math.abs(Number(info.time) - timestampNs);
                if (diff < closest_time) {
                    closest = info;
                    closest_time = diff;
                }
            }
    
            // console.log("Given time:", timestamp, "Closest time:", closest.time, "Image path:", closest.image_path);
            return closest;
        } 
        // else {
        //     let matchingEntry;
        //     matchingEntry = this.photoInfo.find(([key, _]) => key === timestampNs);
        //     if (!matchingEntry) {
        //         console.log("No hash: matchingEntry is undefined");
        //         return undefined;
        //     }
        // }
    }

    setImageToDisplay(imagePath: string): void {
        if (this.oldImage !== imagePath) {
            globals.dispatch(Actions.updateRtuxImage({image: imagePath}));
            this.imageToDisplay = imagePath;
            this.oldImage = imagePath;
        }
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

    async readJsonFile(file: File): Promise<{ vector: Vector, photoVector: PhotoInfo }> {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => {
                const str = reader.result as string;
                const json = JSON.parse(str);

                let vector: Vector = [];
                let photoVector: PhotoInfo = [];

                // Process summary section
                if (json.summary) {
                    let realtime;
                    try {
                        vector = Object.entries(json.summary).map(([event, data]: [string, any]) => {
                            realtime = BigInt(Math.floor(data.CLOCK_REALTIME * 1e9));
                            const eventType = data.Level ? "detection" : "cmd";
                            const eventName = data.Name || event;
                            const level = data.Level || "INFO";
                            return { 
                                key: Time.fromRaw(realtime), 
                                value: `${eventType}:${eventName}:${level}` 
                            };
                        });
                    } catch(e) {
                        vector = Object.entries(json.summary).map(([event, data]: [string, any]) => {
                            const eventType = data.Level ? "detection" : "cmd";
                            const eventName = data.Name || event;
                            const level = data.Level || "INFO";
                            return { 
                                key: Time.INVALID, 
                                value: `${eventType}:${eventName}:${level}` 
                            };
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

                this.globalVector = vector;
                this.photoInfo = photoVector;

                resolve({ vector, photoVector });
            };
            reader.onerror = () => {
                reject(reader.error);
            };
            reader.readAsText(file);
        });
    }

}

export const rtux_loader = new RtuxLoader();
