// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use size file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import {timeToCode, toNs} from '../common/time';

import {globals, SliceDetails} from './globals';
import {Panel} from './panel';

export abstract class SlicePanel extends Panel {
  protected computeDuration(ts: number, dur: number): string {
    return toNs(dur) === -1 ?
        `${globals.state.traceTime.endSec - ts} (Did not end)` :
        timeToCode(dur);
  }

  protected getProcessThreadDetails(sliceInfo: SliceDetails) {
    return new Map<string, string|undefined>([
      ['Thread', `${sliceInfo.threadName} ${sliceInfo.tid}`],
      ['Process', `${sliceInfo.processName} ${sliceInfo.pid}`],
      ['User ID', sliceInfo.uid ? String(sliceInfo.uid) : undefined],
      ['Package name', sliceInfo.packageName],
      [
        'Version code',
        sliceInfo.versionCode ? String(sliceInfo.versionCode) : undefined
      ]
    ]);
  }
}
