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


import m from 'mithril';

import {EngineProxy} from '../common/engine';
import {QueryResponse, runQuery} from '../common/queries';
import {raf} from '../core/raf_scheduler';

import {addTab} from './bottom_tab';
import {globals} from './globals';
import {createPage} from './pages';
import {QueryHistoryComponent, queryHistoryStorage} from './query_history';
import {QueryResultTab} from './query_result_tab';
import {QueryTable} from './query_table';
import {Callout} from './widgets/callout';
import {Editor} from './widgets/editor';

interface QueryPageState {
  enteredText: string;
  executedQuery?: string;
  queryResult?: QueryResponse;
  heightPx: string;
}

const state: QueryPageState = {
  enteredText: '',
  heightPx: '100px',
};

function runManualQuery(query: string) {
  state.executedQuery = query;
  state.queryResult = undefined;
  const engine = getEngine();
  if (engine) {
    runQuery(query, engine).then((resp: QueryResponse) => {
      addTab({
        kind: QueryResultTab.kind,
        tag: 'analyze_page_query',
        config: {
          query: query,
          title: 'Standalone Query',
          prefetchedResponse: resp,
        },
      });
      // We might have started to execute another query. Ignore it in that case.
      if (state.executedQuery !== query) {
        return;
      }
      state.queryResult = resp;
      raf.scheduleFullRedraw();
    });
  }
  raf.scheduleDelayedFullRedraw();
}

function getEngine(): EngineProxy|undefined {
  const engineId = globals.getCurrentEngine()?.id;
  if (engineId === undefined) {
    return undefined;
  }
  const engine = globals.engines.get(engineId)?.getProxy('QueryPage');
  return engine;
}

class QueryInput implements m.ClassComponent {
  private resizeObserver?: ResizeObserver;

  oncreate({dom}: m.CVnodeDOM): void {
    this.resizeObserver = new ResizeObserver(() => {
      state.heightPx = (dom as HTMLElement).style.height;
    });
    this.resizeObserver.observe(dom);
    (dom as HTMLElement).style.height = state.heightPx;
  }

  onremove(): void {
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
      this.resizeObserver = undefined;
    }
  }

  view() {
    return m(Editor, {
      initialText: state.enteredText,

      onExecute: (text: string) => {
        if (!text) {
          return;
        }
        queryHistoryStorage.saveQuery(text);
        runManualQuery(text);
      },

      onUpdate: (text: string) => {
        state.enteredText = text;
      },
    });
  }
}

export const QueryPage = createPage({
  view() {
    return m(
        '.query-page',
        m(Callout, 'Enter query and press Cmd/Ctrl + Enter'),
        m(QueryInput),
        state.executedQuery === undefined ? null : m(QueryTable, {
          query: state.executedQuery,
          resp: state.queryResult,
          onClose: () => {
            state.executedQuery = undefined;
            state.queryResult = undefined;
            raf.scheduleFullRedraw();
          },
          fillParent: false,
        }),
        m(QueryHistoryComponent, {
          runQuery: runManualQuery,
        }));
  },
});
