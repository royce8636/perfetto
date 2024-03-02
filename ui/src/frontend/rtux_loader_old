import {Time, time} from '../base/time';

import {
    FtraceStat,
    RtuxEvent,
} from './globals';
import { publishRtuxCounters, publishRtuxPanelData } from './publish';
// import {HighPrecisionTime } from '../common/high_precision_time';

let globalVector: Array<{ key: time, value: string }> = [];
// let photoInfo: Array<{image_path: string, time: HighPrecisionTime}> = [];
// let photoInfo: Array<{image_path: string, time: time}> = [];
let photoInfo: Array < [time, Array<{ image_path: string; time: any }> ]> = [];
// let globalVector: Array<{ key: string, value: time }> = [];
//define global log_directory
// let log_directory: string = "";
// let photo_directory: string = "";
let imageToDiplay: string = "";

function readRtuxFile(file: File): Promise<Array<{ key: time, value: string }>> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            // Convert the result to a string
            const str = reader.result as string;
            // Parse the string into a vector
            const lines = str.split('\n');

            // log_directory = lines.shift() || ""; // Removes and returns the first element of the array, which is the log directory

            const vector = lines.map(line => {
                const [key, value] = line.split(': ');
                try{
                    const rawkey = BigInt(Math.floor(parseFloat(key) * 1e9));
                    return { key: Time.fromRaw(rawkey), value };
                }
                catch(e){
                    console.log(e);
                    return { key: Time.INVALID, value };

                }
            }).filter(item => {
                return !(item.key === Time.INVALID)
            })
            const counters: FtraceStat[] = [];
            let cnt = 0;
            for (const line of lines) {
                const [name, _] = line.split(': ');
                counters.push({name, count: cnt});
                cnt++;
            }
            publishRtuxCounters(counters);

            const events: RtuxEvent[] = [];
            vector.forEach(({key, value}) => {
                events.push({ts:key, event:value});
            });
            publishRtuxPanelData({events, offset:0,numEvents:cnt});

            globalVector = vector;

            resolve(vector);
        };
        reader.onerror = () => {
            reject(reader.error);
        };
        reader.readAsText(file);
    });
}

function readJsonRtuxFile(file: File): Promise<Array<{ key: time, value: string }>> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            const str = reader.result as string;
            const json = JSON.parse(str);
            let vector;
            let realtime
            try{
                vector = Object.entries(json).map(([event, data]: [string, any]) => {
                    realtime = BigInt(Math.floor(data.CLOCK_REALTIME * 1e9));
                    return { key: Time.fromRaw(realtime), value: event };
                });
            }
            catch(e){
                vector = Object.entries(json).map(([event, data]: [string, any]) => {
                    realtime = BigInt(Math.floor(data.CLOCK_REALTIME * 1e9));
                    return { key: Time.INVALID, value: event };
                });
            }
            const counters: FtraceStat[] = [];
            let cnt = 0;
            for (const event in json) {
                counters.push({name: event, count: cnt});
                cnt++;
            }
            publishRtuxCounters(counters);
            
            const events: RtuxEvent[] = [];
            vector.forEach(({key, value}) => {
                events.push({ts:key, event:value});
            });
            publishRtuxPanelData({events, offset:0,numEvents:cnt});

            globalVector = vector;
            resolve(vector);
        };
        reader.onerror = () => {
            reject(reader.error);
        };
        reader.readAsText(file);
    });
}

function getStoredVector(): Array<{ key: time, value: string }> {
    return globalVector;
}


async function readPhotoInfo(file: File): Promise<Array<[time, Array<{ image_path: string; time: any }> ]>> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            const str = reader.result as string;
            const json = JSON.parse(str);
            
            // Initialize an empty array to store the result
            const result: Array<[time, Array<{ image_path: string; time: any }>]> = [];
            
            // Iterate over each entry in the JSON object
            Object.entries(json).forEach(([timestamp, value]: [any, any]) => {
                timestamp = Time.fromRaw(BigInt(Math.floor(parseFloat(timestamp) * 1e9)));
                // Initialize an array to hold this timestamp's images
                const imagesForTimestamp: Array<{ image_path: string; time: any }> = [];
                
                // Iterate over each path in the value object
                Object.entries(value).forEach(([imagePath, imageTime]: [string, any]) => {
                    // Add each image with its path and time to the array
                    imagesForTimestamp.push({
                        image_path: imagePath,
                        time: Time.fromRaw(BigInt(Math.floor(parseFloat(imageTime) * 1e9))),
                    });
                });
                
                // Add the timestamp and its images to the result
                result.push([timestamp, imagesForTimestamp]);
            });
            
            // Resolve the promise with the filled result array
            photoInfo = result;
            resolve(result);
        };
        reader.onerror = () => {
            reject(reader.error);
        };
        reader.readAsText(file);
    });
}

// function getPhotoInfo(): Array<{image_path: string, time: time}> {
function getPhotoInfo(): Array<[time, Array<{ image_path: string; time: any }> ]> {
    return photoInfo;
}

function getImageToDisplay(): string {
    return imageToDiplay;
}

function setImageToDisplay(images: string): void {
    imageToDiplay = images;
}

// Optionally, adjust the structure to encapsulate the operations
export const rtux_loader = {
    readRtuxFile,
    openRtuxFromFile: async (file: File) => {
        await readRtuxFile(file);
        // await pluginManager.onRtuxLoad();
        // You might want to return something or process the vector further here
    },
    getStoredVector, // Allow access to the stored vector
    readJsonRtuxFile,
    getPhotoInfo,
    getImageToDisplay,
    setImageToDisplay,
    openJsonRtuxFromFile: async (file: File) => {
        await readJsonRtuxFile(file);
    },
    openPhotoInfoFromFile: async (file: File) => {
        await readPhotoInfo(file);
    },
};
