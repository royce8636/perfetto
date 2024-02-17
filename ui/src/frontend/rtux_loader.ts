// import {Time, time} from '../base/time';
// // Define a module-level variable to store the vector
// let globalVector: Array<{ key: time, value: string }> = [];

// function readRtuxFile(file: File): Promise<Array<{ key: time, value: string }>> {
//     return new Promise((resolve, reject) => {
//         const reader = new FileReader();
//         reader.onload = () => {
//             // Convert the result to a string
//             const str = reader.result as string;
//             // const configProtoBase64 = base64Encode(configProto);
//             // const encode = (str: string):string => Buffer.from(str, 'binary').toString('base64');
//             // Parse the string into a vector
//             const lines = str.split('\n');
//             const vector = lines.map(line => {
//                 const [key, value] = line.split(': ');
//                 // return { key: base64Encode(key), value };
//                 // return { key: Time.fromSeconds(parseFloat(key)), value };
//                 const rawkey = BigInt(Math.floor(parseFloat(key) * 1e9));
//                 return { key: Time.fromRaw(rawkey), value };
//             });

//             // Store the vector in the global variable
//             globalVector = vector;

//             resolve(vector);
//         };
//         reader.onerror = () => {
//             reject(reader.error);
//         };
//         reader.readAsText(file);
//     });
// }

// // A new function to access the stored vector
// function getStoredVector(): Array<{ key: time, value: string }> {
//     return globalVector;
// }

// // async function loadRtux(): Promise<void> {
// //     let     
// // }

// // Optionally, adjust the structure to encapsulate the operations
// export const rtux_loader = {
//     readRtuxFile,
//     openRtuxFromFile: async (file: File) => {
//         await readRtuxFile(file);
//         // await pluginManager.onRtuxLoad();
//         // You might want to return something or process the vector further here
//     },
//     getStoredVector, // Allow access to the stored vector
// };
import {Controller} from '../controller/controller';
import {Engine} from '../trace_processor/engine';
import {Actions} from '../common/actions';
import {globals} from './globals';
import {assertExists, assertTrue} from '../base/logging';

type States = 'init' | 'loading_trace' | 'ready';

export class RtuxController extends Controller<States>{
    private readonly engineId: string;
    private engine?: Engine;
  
    constructor(engineId: string) {
      super('init');
      this.engineId = engineId;
    }
    run() {
        const engineCfg = assertExists(globals.state.engine);
        switch (this.state) {
        case 'init':
          this.loadRtux()
            .then((mode) => {
              globals.dispatch(Actions.setEngineReady({
                engineId: this.engineId,
                ready: true,
                mode,
              }));
            })
            .catch((err) => {
              this.updateStatus(`${err}`);
              throw err;
            });
          this.updateStatus('Opening trace');
          this.setState('loading_trace');
          break;
    }
    }
    private updateStatus(msg: string): void {
        globals.dispatch(Actions.updateStatus({
        msg,
        timestamp: Date.now() / 1000,
        }));
    }

    private async loadRtux(): Promise<EngineMode>{
        this.updateStatus('Creating Rtux Processor');
        let engineMode: EngineMode;
        let useRpc = false;
        if (globals.state.newEngineMode === 'USE_HTTP_RPC_IF_AVAILABLE') {
          useRpc = (await HttpRpcEngine.checkConnection()).connected;
        }
        let engine;
        if (useRpc) {
          console.log('Opening trace using native accelerator over HTTP+RPC');
          engineMode = 'HTTP_RPC';
          engine = new HttpRpcEngine(this.engineId, LoadingManager.getInstance);
          engine.errorHandler = (err) => {
            globals.dispatch(
              Actions.setEngineFailed({mode: 'HTTP_RPC', failure: `${err}`}));
            throw err;
          };
        } else {
          console.log('Opening trace using built-in WASM engine');
          engineMode = 'WASM';
          const enginePort = resetEngineWorker();
          engine = new WasmEngineProxy(
            this.engineId, enginePort, LoadingManager.getInstance);
          engine.resetTraceProcessor({
            cropTrackEvents: CROP_TRACK_EVENTS_FLAG.get(),
            ingestFtraceInRawTable: INGEST_FTRACE_IN_RAW_TABLE_FLAG.get(),
            analyzeTraceProtoContent: ANALYZE_TRACE_PROTO_CONTENT_FLAG.get(),
          });
        }
    }
}