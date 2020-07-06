// Copyright (C) 2020 The Android Open Source Project
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

import {cropText} from './canvas_utils';

test('cropHelper regular text', () => {
  expect(cropText(
             'com.android.camera [4096]',
             /*charWidth=*/ 5,
             /*rectWidth=*/ 1 * 5 + 2))
      .toBe('c');
  expect(cropText('com.android.camera [4096]', 5, 4 * 5 + 2)).toBe('c');
  expect(cropText('com.android.camera [4096]', 5, 5 * 5 + 2)).toBe('co...');
  expect(cropText('com.android.camera [4096]', 5, 13 * 5 + 2))
      .toBe('com.androi...');
  expect(cropText('com.android.camera [4096]', 5, 25 * 5 + 2))
      .toBe('com.android.camera [4096]');
});
