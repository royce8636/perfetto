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

import {EngineProxy} from '../../trace_processor/engine';
import {
  Plugin,
  PluginContext,
  PluginContextTrace,
  PluginDescriptor,
} from '../../public';
import {addDebugSliceTrack} from '../../tracks/debug/slice_track';


class AndroidNetwork implements Plugin {
  onActivate(_ctx: PluginContext): void {}

  // Adds a debug track using the provided query and given columns. The columns
  // must be start with ts, dur, and a name column. The name column and all
  // following columns are shown as arguments in slice details.
  async addSimpleTrack(
    engine: EngineProxy, trackName: string, tableOrQuery: string,
    columns: string[]): Promise<void> {
    await addDebugSliceTrack(
      engine,
      {
        sqlSource: `SELECT ${columns.join(',')} FROM ${tableOrQuery}`,
        columns: columns,
      },
      trackName,
      {ts: columns[0], dur: columns[1], name: columns[2]},
      columns.slice(2),
    );
  }

  async onTraceLoad(ctx: PluginContextTrace): Promise<void> {
    ctx.registerCommand({
      id: 'dev.perfetto.AndroidNetwork#batteryEvents',
      name: 'Run query: Pin battery events',
      callback: async (track) => {
        if (track === undefined) {
          track = prompt('Battery Track', '');
          if (track === null) return;
        }

        await ctx.engine.query(`SELECT IMPORT('android.battery_stats');`);
        await this.addSimpleTrack(
          ctx.engine,
          track,
          `(SELECT *
            FROM android_battery_stats_event_slices
            WHERE track_name = "${track}")`,
          ['ts', 'dur', 'str_value', 'int_value']);
      },
    });

    ctx.registerCommand({
      id: 'dev.perfetto.AndroidNetwork#activityTrack',
      name: 'Run query: Visualize Network Activity',
      callback: async (groupby, filter, trackName) => {
        if (groupby === undefined) {
          groupby = prompt('Group by', 'package_name');
          if (groupby === null) return;
        }

        if (filter === undefined) {
          filter = prompt('Filter', 'TRUE');
          if (filter === null) return;
        }

        const suffix = new Date().getTime();
        await ctx.engine.query(`
          SELECT RUN_METRIC(
            'android/network_activity_template.sql',
            'view_name', 'android_network_activity_${suffix}',
            'group_by',  '${groupby}',
            'filter',    '${filter}',
            'idle_ns',   '10e9',
            'quant_ns',  '3e9'
          );
        `);

        // The first group column is used for the slice name.
        const groupCols = groupby.replaceAll(' ', '').split(',');
        await this.addSimpleTrack(
          ctx.engine,
          trackName || 'Network Activity',
          `android_network_activity_${suffix}`,
          ['ts', 'dur', ...groupCols, 'packet_length', 'packet_count']);
      },
    });
  }
}

export const plugin: PluginDescriptor = {
  pluginId: 'dev.perfetto.AndroidNetwork',
  plugin: AndroidNetwork,
};
