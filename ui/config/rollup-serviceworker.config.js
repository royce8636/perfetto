// Copyright (C) 2021 The Android Open Source Project
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

import commonjs from '@rollup/plugin-commonjs';
import nodeResolve from '@rollup/plugin-node-resolve';
import sourcemaps from 'rollup-plugin-sourcemaps';

const path = require('path');
const ROOT_DIR = path.dirname(path.dirname(__dirname));  // The repo root.
const OUT_SYMLINK = path.join(ROOT_DIR, 'ui/out');

export default [{
  input: `${OUT_SYMLINK}/tsc/service_worker/service_worker.js`,
  output: {
    name: 'service_worker',
    format: 'iife',
    esModule: false,
    file: `${OUT_SYMLINK}/dist/service_worker.js`,
    sourcemap: true,
  },
  plugins: [
    nodeResolve({
      mainFields: ['browser'],
      browser: true,
      preferBuiltins: false,
    }),
    commonjs(),
    sourcemaps(),
  ],
}]
