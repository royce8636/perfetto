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

import {NamedRecordConfig, RecordConfig, RecordMode} from '../common/state';

type Json = JsonObject|Json[]|null|number|boolean|string;

export interface JsonObject {
  [key: string]: Json;
}

class ObjectValidator {
  raw: JsonObject;
  invalidKeys: string[];
  prefix: string;

  constructor(raw: JsonObject, prefix?: string, invalidKeys?: string[]) {
    this.raw = raw;
    this.prefix = prefix || '';
    this.invalidKeys = invalidKeys || [];
  }

  private reportInvalidKey(key: string) {
    this.invalidKeys.push(this.prefix + key);
  }

  number(key: string, def = 0): number {
    if (!(key in this.raw)) {
      return def;
    }

    const val = this.raw[key];
    delete this.raw[key];
    if (typeof val === 'number') {
      return val;
    } else {
      this.reportInvalidKey(key);
      return def;
    }
  }

  string(key: string, def = ''): string {
    if (!(key in this.raw)) {
      return def;
    }

    const val = this.raw[key];
    delete this.raw[key];
    if (typeof val === 'string') {
      return val;
    } else {
      this.reportInvalidKey(key);
      return def;
    }
  }

  requiredString(key: string): string {
    if (!(key in this.raw)) {
      throw new Error(`key ${this.prefix + key} not found`);
    }

    const val = this.raw[key];
    delete this.raw[key];
    if (typeof val === 'string') {
      return val;
    } else {
      throw new Error(`key ${this.prefix + key} not found`);
    }
  }

  stringArray(key: string, def: string[] = []): string[] {
    if (!(key in this.raw)) {
      return def;
    }

    const val = this.raw[key];
    delete this.raw[key];
    if (Array.isArray(val)) {
      for (let i = 0; i < val.length; i++) {
        if (typeof val[i] !== 'string') {
          this.reportInvalidKey(key);
          return def;
        }
      }
      return val as string[];
    } else {
      this.reportInvalidKey(key);
      return def;
    }
  }

  boolean(key: string, def = false): boolean {
    if (!(key in this.raw)) {
      return def;
    }

    const val = this.raw[key];
    delete this.raw[key];
    if (typeof val === 'boolean') {
      return val;
    } else {
      this.reportInvalidKey(key);
      return def;
    }
  }

  recordMode(key: string, def: RecordMode): RecordMode {
    if (!(key in this.raw)) {
      return def;
    }

    const mode = this.raw[key];
    delete this.raw[key];
    if (typeof mode !== 'string') {
      this.reportInvalidKey(key);
      return def;
    }

    if (mode === 'STOP_WHEN_FULL') {
      return mode;
    } else if (mode === 'RING_BUFFER') {
      return mode;
    } else if (mode === 'LONG_TRACE') {
      return mode;
    } else {
      this.reportInvalidKey(key);
      return def;
    }
  }

  private childObject(key: string): JsonObject {
    if (!(key in this.raw)) {
      return {};
    }

    const result = this.raw[key];
    delete this.raw[key];
    if (typeof result === 'object' && !Array.isArray(result) &&
        result !== null) {
      return result;
    } else {
      this.reportInvalidKey(key);
      return {};
    }
  }

  object(key: string): ObjectValidator {
    return new ObjectValidator(
        this.childObject(key), key + '.', this.invalidKeys);
  }
}

export interface ValidationResult<T> {
  result: T;
  invalidKeys: string[];
  extraKeys: string[];
}

// Run the parser that takes raw JSON and outputs (potentially reconstructed
// with default values) typed object, together with additional information,
// such as fields with invalid values and extraneous keys.
//
// Parsers modify input JSON objects destructively.
export function runParser<T>(
    parser: (validator: ObjectValidator) => T,
    input: JsonObject): ValidationResult<T> {
  const validator = new ObjectValidator(input);
  const valid = parser(validator);

  return {
    result: valid,
    // Validator removes all the parsed keys, therefore the only ones that
    // remain are extraneous.
    extraKeys: Object.keys(input),
    invalidKeys: validator.invalidKeys
  };
}

export function validateNamedRecordConfig(v: ObjectValidator):
    NamedRecordConfig {
  return {
    title: v.requiredString('title'),
    config: validateRecordConfig(v.object('config')),
    key: v.requiredString('key')
  };
}

export function validateRecordConfig(v: ObjectValidator): RecordConfig {
  return {
    mode: v.recordMode('mode', 'STOP_WHEN_FULL'),
    durationMs: v.number('durationMs', 10000.0),
    maxFileSizeMb: v.number('maxFileSizeMb', 100),
    fileWritePeriodMs: v.number('fileWritePeriodMs', 2500),
    bufferSizeMb: v.number('bufferSizeMb', 64.0),

    cpuSched: v.boolean('cpuSched'),
    cpuFreq: v.boolean('cpuFreq'),
    cpuSyscall: v.boolean('cpuSyscall'),

    gpuFreq: v.boolean('gpuFreq'),
    gpuMemTotal: v.boolean('gpuMemTotal'),

    ftrace: v.boolean('ftrace'),
    atrace: v.boolean('atrace'),
    ftraceEvents: v.stringArray('ftraceEvents'),
    ftraceExtraEvents: v.string('ftraceExtraEvents'),
    atraceCats: v.stringArray('atraceCats'),
    atraceApps: v.string('atraceApps'),
    ftraceBufferSizeKb: v.number('ftraceBufferSizeKb', 2 * 1024),
    ftraceDrainPeriodMs: v.number('ftraceDrainPerionMs', 250),
    androidLogs: v.boolean('androidLogs'),
    androidLogBuffers: v.stringArray('androidLogBuffers'),
    androidFrameTimeline: v.boolean('androidFrameTimeline'),

    cpuCoarse: v.boolean('cpuCoarse'),
    cpuCoarsePollMs: v.number('cpuCoarsePollMs', 1000),

    batteryDrain: v.boolean('batteryDrain'),
    batteryDrainPollMs: v.number('batteryDrainPollMs', 1000),

    boardSensors: v.boolean('boardSensors'),

    memHiFreq: v.boolean('memHiFreq'),
    meminfo: v.boolean('meminfo'),
    meminfoPeriodMs: v.number('meminfoPeriodMs', 1000),
    meminfoCounters: v.stringArray('meminfoCounters'),

    vmstat: v.boolean('vmstat'),
    vmstatPeriodMs: v.number('vmstatPeriodMs', 1000),
    vmstatCounters: v.stringArray('vmstatCounters'),

    heapProfiling: v.boolean('heapProfiling'),
    hpSamplingIntervalBytes: v.number('hpSamplingIntervalBytes', 4096),
    hpProcesses: v.string('hpProcesses'),
    hpContinuousDumpsPhase: v.number('hpContinuousDumpsPhase'),
    hpContinuousDumpsInterval: v.number('hpContinuousDumpsInterval'),
    hpSharedMemoryBuffer: v.number('hpSharedMemoryBuffer', 8 * 1048576),
    hpBlockClient: v.boolean('hpBlockClient', true),
    hpAllHeaps: v.boolean('hpAllHeaps'),

    javaHeapDump: v.boolean('javaHeapDump'),
    jpProcesses: v.string('jpProcesses'),
    jpContinuousDumpsPhase: v.number('jpContinuousDumpsPhase'),
    jpContinuousDumpsInterval: v.number('jpContinuousDumpsInterval'),

    memLmk: v.boolean('memLmk'),
    procStats: v.boolean('procStats'),
    procStatsPeriodMs: v.number('procStatsPeriodMs', 1000),

    chromeCategoriesSelected: v.stringArray('chromeCategoriesSelected'),

    chromeLogs: v.boolean('chromeLogs'),
    taskScheduling: v.boolean('taskScheduling'),
    ipcFlows: v.boolean('ipcFlows'),
    jsExecution: v.boolean('jsExecution'),
    webContentRendering: v.boolean('webContentRendering'),
    uiRendering: v.boolean('uiRendering'),
    inputEvents: v.boolean('inputEvents'),
    navigationAndLoading: v.boolean('navigationAndLoading'),
    chromeHighOverheadCategoriesSelected:
        v.stringArray('chromeHighOverheadCategoriesSelected'),
    symbolizeKsyms: v.boolean('symbolizeKsyms'),
  };
}

export function createEmptyRecordConfig(): RecordConfig {
  return runParser(validateRecordConfig, {}).result;
}
