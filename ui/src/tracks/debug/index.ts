// Copyright (C) 2023 The Android Open Source Project
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

import {Plugin, PluginContext, PluginDescriptor} from '../../public';

import {DebugCounterTrack} from './counter_track';
import {DebugTrackV2} from './slice_track';

class DebugTrackPlugin implements Plugin {
  onActivate(ctx: PluginContext): void {
    ctx.LEGACY_registerTrack(DebugTrackV2);
    ctx.LEGACY_registerTrack(DebugCounterTrack);
  }
}

export const plugin: PluginDescriptor = {
  pluginId: 'perfetto.DebugSlices',
  plugin: DebugTrackPlugin,
};
