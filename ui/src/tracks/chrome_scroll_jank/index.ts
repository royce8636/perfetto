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

import {AddTrackArgs} from '../../common/actions';
import {Engine} from '../../common/engine';
import {featureFlags} from '../../common/feature_flags';
import {
  PluginContext,
} from '../../public';
import {ObjectById} from '../../common/state';
import {CustomSqlDetailsPanelConfig} from '../custom_sql_table_slices';

import {ChromeTasksScrollJankTrack} from './chrome_tasks_scroll_jank_track';
import {addLatencyTracks, EventLatencyTrack} from './event_latency_track';
import {TopLevelScrollTrack} from './scroll_track';
import {addTopLevelJankTrack, TopLevelJankTrack} from './top_level_jank_track';
import {
  addJankyLatenciesTrack,
  TopLevelEventLatencyTrack,
} from './top_level_janky_event_latencies_track';

export {Data} from '../chrome_slices';

export const ENABLE_CHROME_SCROLL_JANK_PLUGIN = featureFlags.register({
  id: 'enableChromeScrollJankPlugin',
  name: 'Enable Chrome Scroll Jank plugin',
  description: 'Adds new tracks for scroll jank in Chrome',
  defaultValue: false,
});

export const INPUT_LATENCY_TRACK = 'InputLatency::';

export const ENABLE_SCROLL_JANK_PLUGIN_V2 = featureFlags.register({
  id: 'enableScrollJankPluginV2',
  name: 'Enable Scroll Jank plugin V2',
  description: 'Adds new tracks and visualizations for scroll jank.',
  defaultValue: false,
});

export type ScrollJankTracks = {
  tracksToAdd: AddTrackArgs[],
};

export interface ScrollJankTrackSpec {
  id: string;
  sqlTableName: string;
  detailsPanelConfig: CustomSqlDetailsPanelConfig;
}

// Global state for the scroll jank plugin.
export class ScrollJankPluginState {
  private static instance: ScrollJankPluginState;
  private tracks: ObjectById<ScrollJankTrackSpec>;

  private constructor() {
    this.tracks = {};
  }

  public static getInstance(): ScrollJankPluginState {
    if (!ScrollJankPluginState.instance) {
      ScrollJankPluginState.instance = new ScrollJankPluginState();
    }

    return ScrollJankPluginState.instance;
  }

  public registerTrack(args: {
    kind: string,
    trackId: string,
    tableName: string,
    detailsPanelConfig: CustomSqlDetailsPanelConfig,
  }): void {
    this.tracks[args.kind] = {
      id: args.trackId,
      sqlTableName: args.tableName,
      detailsPanelConfig: args.detailsPanelConfig,
    };
  }

  public unregisterTrack(kind: string): void {
    delete this.tracks[kind];
  }

  public getTrack(kind: string): ScrollJankTrackSpec|undefined {
    return this.tracks[kind];
  }
}

export async function getScrollJankTracks(engine: Engine):
    Promise<ScrollJankTracks> {
  const result: ScrollJankTracks = {
    tracksToAdd: [],
  };

  const topLevelJanks = addTopLevelJankTrack(engine);
  const topLevelJanksResult = await topLevelJanks;
  let originalLength = result.tracksToAdd.length;
  result.tracksToAdd.length += topLevelJanksResult.tracksToAdd.length;
  for (let i = 0; i < topLevelJanksResult.tracksToAdd.length; ++i) {
    result.tracksToAdd[i + originalLength] = topLevelJanksResult.tracksToAdd[i];
  }

  // TODO(b/278844325): Top Level event latency summary is already rendered in
  // the TopLevelJankTrack; this track should be rendered at a more
  // intuitive location when the descendant slices are rendered.
  originalLength = result.tracksToAdd.length;
  const jankyEventLatencies = addJankyLatenciesTrack(engine);
  const jankyEventLatencyResult = await jankyEventLatencies;
  result.tracksToAdd.length += jankyEventLatencyResult.tracksToAdd.length;
  for (let i = 0; i < jankyEventLatencyResult.tracksToAdd.length; ++i) {
    result.tracksToAdd[i + originalLength] =
        jankyEventLatencyResult.tracksToAdd[i];
  }

  originalLength = result.tracksToAdd.length;
  const eventLatencies = addLatencyTracks(engine);
  const eventLatencyResult = await eventLatencies;
  result.tracksToAdd.length += eventLatencyResult.tracksToAdd.length;
  for (let i = 0; i < eventLatencyResult.tracksToAdd.length; ++i) {
    result.tracksToAdd[i + originalLength] = eventLatencyResult.tracksToAdd[i];
  }

  return result;
}

function activate(ctx: PluginContext) {
  ctx.registerTrack(ChromeTasksScrollJankTrack);
  ctx.registerTrack(TopLevelJankTrack);
  ctx.registerTrack(TopLevelScrollTrack);
  ctx.registerTrack(TopLevelEventLatencyTrack);
  ctx.registerTrack(EventLatencyTrack);
}

export const plugin = {
  pluginId: 'perfetto.ChromeScrollJank',
  activate,
};
