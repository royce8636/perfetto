// Copyright (C) 2018 The Android Open Source Project
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

import {TrackData} from '../../common/track_data';

export const COUNTER_TRACK_KIND = 'CounterTrack';

export type CounterScaleOptions = 'ZERO_BASED'|'MIN_MAX'|'DELTA_FROM_PREVIOUS';

export interface Data extends TrackData {
  maximumValue: number;
  minimumValue: number;
  maximumDelta: number;
  minimumDelta: number;
  timestamps: Float64Array;
  lastIds: Float64Array;
  minValues: Float64Array;
  maxValues: Float64Array;
  lastValues: Float64Array;
  totalDeltas: Float64Array;
}

export interface Config {
  name: string;
  maximumValue?: number;
  minimumValue?: number;
  startTs?: number;
  endTs?: number;
  namespace: string;
  trackId: number;
  scale?: CounterScaleOptions;
}
