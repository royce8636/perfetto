// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import {uint8ArrayToBase64} from '../base/string_utils';
import {Adb, AdbStream} from './adb_interfaces';
import {ReadBuffersResponse} from './consumer_port_types';
import {globals} from './globals';
import {extractTraceConfig} from './record_controller';
import {Consumer, RpcConsumerPort} from './record_controller_interfaces';

enum AdbState {
  READY,
  RECORDING,
  FETCHING
}
const DEFAULT_DESTINATION_FILE = '/data/misc/perfetto-traces/trace';

export class AdbConsumerPort extends RpcConsumerPort {
  // public for testing
  traceDestFile = DEFAULT_DESTINATION_FILE;
  private state = AdbState.READY;
  private adb: Adb;
  private device: USBDevice|undefined = undefined;

  constructor(adb: Adb, consumerPortListener: Consumer) {
    super(consumerPortListener);
    this.adb = adb;
  }


  handleCommand(method: string, params: Uint8Array) {
    switch (method) {
      case 'EnableTracing':
        this.enableTracing(params);
        break;
      case 'ReadBuffers':
        this.readBuffers();
        break;
      case 'DisableTracing':
        // TODO(nicomazz): Implement disable tracing. The adb shell stream
        // should be stopped somehow.
        break;
      case 'FreeBuffers':  // no-op
      case 'GetTraceStats':
        break;
      default:
        this.sendErrorMessage(`Method not recognized: ${method}`);
        break;
    }
  }

  async enableTracing(enableTracingProto: Uint8Array) {
    try {
      console.assert(this.state === AdbState.READY);
      this.device = await this.findDevice();

      if (this.device === undefined) {
        this.sendErrorMessage('No device found');
        return;
      }
      this.sendStatus(
          'Check the screen of your device and allow USB debugging.');
      await this.adb.connect(this.device);
      const traceConfigProto = extractTraceConfig(enableTracingProto);

      if (!traceConfigProto) {
        this.sendErrorMessage('Invalid config.');
        return;
      }

      await this.startRecording(traceConfigProto);
      this.sendStatus('Recording in progress...');

    } catch (e) {
      this.sendErrorMessage(e.message);
    }
  }

  async startRecording(configProto: Uint8Array) {
    this.state = AdbState.RECORDING;
    const recordCommand = this.generateStartTracingCommand(configProto);
    const recordShell: AdbStream = await this.adb.shell(recordCommand);
    let response = '';
    recordShell.onData = (str, _) => response += str;
    recordShell.onClose = () => {
      if (!this.tracingEndedSuccessfully(response)) {
        this.sendErrorMessage(response);
        this.state = AdbState.READY;
        return;
      }
      this.sendStatus('Recording ended successfully. Fetching the trace..');
      this.sendMessage({type: 'EnableTracingResponse'});
    };
  }

  tracingEndedSuccessfully(response: string): boolean {
    return !response.includes(' 0 ms') && response.includes('Wrote ');
  }

  async findDevice() {
    const deviceConnected = globals.state.androidDeviceConnected;
    if (!deviceConnected) return undefined;
    const devices = await navigator.usb.getDevices();
    return devices.find(d => d.serialNumber === deviceConnected.serial);
  }

  async readBuffers() {
    console.assert(this.state === AdbState.RECORDING);
    this.state = AdbState.FETCHING;

    const readTraceShell =
        await this.adb.shell(this.generateReadTraceCommand());
    let trace = '';
    readTraceShell.onData = (str, _) => {
      // TODO(nicomazz): Since we are using base64, we can't decode the chunks
      // as they are received (without further base64 stream decoding
      // implementations). After the investigation about why without base64
      // things are not working, the chunks should be sent as they are received,
      // like in the following line.
      // this.sendMessage(this.generateChunkReadResponse(str));
      // EDIT: we should send back a response as if it was a real
      // ReadBufferResponse, with trace packets. Here we are only sending the
      // trace split in several pieces.
      trace += str;
    };
    readTraceShell.onClose = () => {
      const decoded = atob(trace.replace(/\n/g, ''));

      this.sendMessage(
          this.generateChunkReadResponse(decoded, /* last */ true));
      this.state = AdbState.READY;
    };
  }

  generateChunkReadResponse(data: string, last = false): ReadBuffersResponse {
    return {
      type: 'ReadBuffersResponse',
      slices: [{data: data as unknown as Uint8Array, lastSliceForPacket: last}]
    };
  }

  generateReadTraceCommand(): string {
    // TODO(nicomazz): Investigate why without base64 things break.
    return `cat ${this.traceDestFile} | gzip | base64`;
  }

  generateStartTracingCommand(tracingConfig: Uint8Array) {
    const configBase64 = uint8ArrayToBase64(tracingConfig);
    const perfettoCmd = `perfetto -c - -o ${this.traceDestFile}`;
    return `echo '${configBase64}' | base64 -d | ${perfettoCmd}`;
  }
}
