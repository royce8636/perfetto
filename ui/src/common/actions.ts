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

export interface Action { type: string; }

export function openTrace(url: string) {
  return {
    type: 'OPEN_TRACE',
    url,
  };
}

// TODO(hjd): Remove CPU and add a generic way to handle track specific state.
export function addTrack(engineId: string, trackKind: string, cpu: number) {
  return {
    type: 'ADD_TRACK',
    engineId,
    trackKind,
    cpu,
  };
}

export function executeQuery(engineId: string, query: string) {
  return {
    type: 'EXECUTE_QUERY',
    engineId,
    query,
  };
}

export function navigate(route: string) {
  return {
    type: 'NAVIGATE',
    route,
  };
}

export function moveTrack(trackId: string, direction: 'up'|'down') {
  return {
    type: 'MOVE_TRACK',
    trackId,
    direction,
  };
}