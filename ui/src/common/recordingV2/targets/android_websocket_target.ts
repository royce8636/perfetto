// Copyright (C) 2022 The Android Open Source Project
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

import {AdbConnectionOverWebsocket} from '../adb_connection_over_websocket';
import {DEFAULT_TRACED_CONSUMER_SOCKET_PATH} from '../adb_targets_utils';
import {
  RecordingTargetV2,
  TargetInfo,
  TracingSession,
  TracingSessionListener,
} from '../recording_interfaces_v2';
import {TracedTracingSession} from '../traced_tracing_session';

export class AndroidWebsocketTarget implements RecordingTargetV2 {
  private adbConnection: AdbConnectionOverWebsocket;

  constructor(private serialNumber: string, websocketUrl: string) {
    this.adbConnection =
        new AdbConnectionOverWebsocket(serialNumber, websocketUrl);
  }

  getInfo(): TargetInfo {
    return {
      targetType: 'ANDROID',
      // TODO(octaviant): fetch the OS from the adb connection
      // once aosp/2127460 is in
      androidApiLevel: undefined,
      dataSources: [],
      name: this.serialNumber + ' WebSocket',
    };
  }

  // This is called when the websocket url changes and we need to disconnect
  // the targets connected to the old URL.
  disconnect(): void {
    this.adbConnection.disconnect();
  }


  // Starts a tracing session in order to fetch information such as apiLevel
  // and dataSources from the device. Then, it cancels the session.
  async fetchTargetInfo(tracingSessionListener: TracingSessionListener):
      Promise<void> {
    const tracingSession =
        await this.createTracingSession(tracingSessionListener);
    tracingSession.cancel();
  }

  async createTracingSession(tracingSessionListener: TracingSessionListener):
      Promise<TracingSession> {
    this.adbConnection.onDisconnect = tracingSessionListener.onDisconnect;
    const adbStream = await this.adbConnection.connectSocket(
        DEFAULT_TRACED_CONSUMER_SOCKET_PATH);

    const tracingSession =
        new TracedTracingSession(adbStream, tracingSessionListener);
    await tracingSession.initConnection();
    return tracingSession;
  }

  canConnectWithoutContention(): Promise<boolean> {
    return this.adbConnection.canConnectWithoutContention();
  }
}
