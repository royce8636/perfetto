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

import * as m from 'mithril';

import {PERMALINK_ID} from '../common/permalinks';
import {globals} from './globals';
import {Sidebar} from './sidebar';
import {Topbar} from './topbar';

function renderPermalink(): m.Children {
  if (!globals.state.permalink) {
    return null;
  }
  const config =
      globals.trackDataStore.get(PERMALINK_ID) as {} as {url: string};
  const url = config ? config.url : null;
  return m(
      '.alert-permalink',
      url ? ['Permalink: ', m(`a[href=${url}]`, url)] : 'Uploading...');
}

const Alerts: m.Component = {
  view() {
    return m('.alerts', renderPermalink());
  },
};

/**
 * Wrap component with common UI elements (nav bar etc).
 */
export function createPage(component: m.Component): m.Component {
  return {
    view() {
      return [
        m(Sidebar),
        m(Topbar),
        m(component),
        m(Alerts),
      ];
    },
  };
}
