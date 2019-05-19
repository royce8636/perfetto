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

import {TrackState} from '../common/state';
import {LIMIT, TrackData} from '../common/track_data';

import {globals} from './globals';

/**
 * This interface forces track implementations to have some static properties.
 * Typescript does not have abstract static members, which is why this needs to
 * be in a seperate interface.
 */
export interface TrackCreator {
  // Store the kind explicitly as a string as opposed to using class.kind in
  // case we ever minify our code.
  readonly kind: string;

  // We need the |create| method because the stored value in the registry is an
  // abstract class, and we cannot call 'new' on an abstract class.
  create(TrackState: TrackState): Track;
}

/**
 * The abstract class that needs to be implemented by all tracks.
 */
export abstract class Track<Config = {}, Data extends TrackData = TrackData> {
  constructor(protected trackState: TrackState) {}
  abstract renderCanvas(ctx: CanvasRenderingContext2D): void;

  get config(): Config {
    return this.trackState.config as Config;
  }

  data(): Data|undefined {
    return globals.trackDataStore.get(this.trackState.id) as Data;
  }

  getHeight(): number {
    return 40;
  }

  onMouseMove(_position: {x: number, y: number}) {}

  /**
   * Returns whether the mouse click has selected something.
   * Used to prevent further propagation if necessary.
   */
  onMouseClick(_position: {x: number, y: number}): boolean {
    return false;
  }

  onMouseOut() {}

  shouldRequestData(
      data: Data|undefined, windowStart: number, windowEnd: number): boolean {
    if (data === undefined) return true;

    // If at the limit only request more data if the view has moved.
    const atLimit = data.length === LIMIT;
    if (atLimit) {
      // We request more data than the window, so add window duration to find
      // the previous window.
      const prevWindowStart = data.start + (windowEnd - windowStart);
      return windowStart !== prevWindowStart;
    }

    // Otherwise request more data only when out of range of current data or
    // resolution has changed.
    const inRange = windowStart >= data.start && windowEnd <= data.end;
    return !inRange || data.resolution !== globals.getCurResolution();
  }
}
