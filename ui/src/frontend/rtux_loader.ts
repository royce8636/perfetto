import {Time, time} from '../base/time';
// const fs = require('fs').promises;
// const path = require('path');
// const os = require('os');
// import fs from 'fs/promises';
// import * as fs from 'fs/promises';
import {promises as fs} from 'fs';
import path from 'path';
// import os from 'os';

// import { Logger } from 'sass';
// Define a module-level variable to store the vector
import {
    // globals,
    FtraceStat,
    RtuxEvent,
} from './globals';
import { publishRtuxCounters, publishRtuxPanelData } from './publish';
let globalVector: Array<{ key: time, value: string }> = [];
//define global log_directory
let log_directory: string = "";

function readRtuxFile(file: File): Promise<Array<{ key: time, value: string }>> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            // Convert the result to a string
            const str = reader.result as string;
            // const configProtoBase64 = base64Encode(configProto);
            // const encode = (str: string):string => Buffer.from(str, 'binary').toString('base64');
            // Parse the string into a vector
            const lines = str.split('\n');

            log_directory = lines.shift() || ""; // Removes and returns the first element of the array, which is the log directory

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

// A new function to access the stored vector
function getStoredVector(): Array<{ key: time, value: string }> {
    return globalVector;
}

// function resolveHome(filepath: string) {
//     if (filepath[0] === '~') {
//         return path.join(os.homedir(), filepath.slice(1));
//     }
//     return filepath;
// }

// function resolveHome(filepath: string) {
//     if (filepath[0] === '~') {
//         return process.env.HOME + filepath.slice(1);
//     }
//     return filepath;
// }


type DetectedFile = {
    path: string;
    number: number;
};
async function getSortedFilePaths(): Promise<string[]> {
    const directory = log_directory;
    try {
        const img_files = await fs.readdir(directory);
        const detectedFiles: string[] = img_files
            .filter((img_file: string) => img_file.startsWith('detected_') && img_file.endsWith('.png'))
            .map((img_file: string): DetectedFile => {
                const match = img_file.match(/detected_(\d+)\.png/);
                const numberPart = match ? match[1] : '0'; // Fallback to '0' if match is null
                return {
                    path: path.join(directory, img_file),
                    number: parseInt(numberPart, 10),
                };
            })
            .sort((a: DetectedFile, b: DetectedFile) => a.number - b.number)
            .map((file: DetectedFile) => file.path);

        return detectedFiles;
    } catch (error) {
        console.error('Error reading directory:', error);
        return [];
    }
}

// async function loadRtux(): Promise<void> {
//     let     
// }

// Optionally, adjust the structure to encapsulate the operations
export const rtux_loader = {
    readRtuxFile,
    openRtuxFromFile: async (file: File) => {
        await readRtuxFile(file);
        // await pluginManager.onRtuxLoad();
        // You might want to return something or process the vector further here
    },
    getStoredVector, // Allow access to the stored vector
    getSortedFilePaths,
};

// import {Controller} from '../controller/controller';
// import {Engine} from '../trace_processor/engine';
// import {Actions} from '../common/actions';
// import {globals} from './globals';
// import {assertExists, assertTrue} from '../base/logging';

// type States = 'init' | 'loading_trace' | 'ready';

// export class RtuxController extends Controller<States>{
//     private readonly engineId: string;
//     private engine?: Engine;
  
//     constructor(engineId: string) {
//       super('init');
//       this.engineId = engineId;
//     }
//     run() {
//         const engineCfg = assertExists(globals.state.engine);
//         switch (this.state) {
//         case 'init':
//           this.loadRtux()
//             .then((mode) => {
//               globals.dispatch(Actions.setEngineReady({
//                 engineId: this.engineId,
//                 ready: true,
//                 mode,
//               }));
//             })
//             .catch((err) => {
//               this.updateStatus(`${err}`);
//               throw err;
//             });
//           this.updateStatus('Opening trace');
//           this.setState('loading_trace');
//           break;
//     }
//     }
//     private updateStatus(msg: string): void {
//         globals.dispatch(Actions.updateStatus({
//         msg,
//         timestamp: Date.now() / 1000,
//         }));
//     }

//     private async loadRtux(): Promise<EngineMode>{
//         this.updateStatus('Creating Rtux Processor');
//         let engineMode: EngineMode;
//         let useRpc = false;
//         if (globals.state.newEngineMode === 'USE_HTTP_RPC_IF_AVAILABLE') {
//           useRpc = (await HttpRpcEngine.checkConnection()).connected;
//         }
//         let engine;
//         if (useRpc) {
//           console.log('Opening trace using native accelerator over HTTP+RPC');
//           engineMode = 'HTTP_RPC';
//           engine = new HttpRpcEngine(this.engineId, LoadingManager.getInstance);
//           engine.errorHandler = (err) => {
//             globals.dispatch(
//               Actions.setEngineFailed({mode: 'HTTP_RPC', failure: `${err}`}));
//             throw err;
//           };
//         } else {
//           console.log('Opening trace using built-in WASM engine');
//           engineMode = 'WASM';
//           const enginePort = resetEngineWorker();
//           engine = new WasmEngineProxy(
//             this.engineId, enginePort, LoadingManager.getInstance);
//           engine.resetTraceProcessor({
//             cropTrackEvents: CROP_TRACK_EVENTS_FLAG.get(),
//             ingestFtraceInRawTable: INGEST_FTRACE_IN_RAW_TABLE_FLAG.get(),
//             analyzeTraceProtoContent: ANALYZE_TRACE_PROTO_CONTENT_FLAG.get(),
//           });
//         }
//     }
// }