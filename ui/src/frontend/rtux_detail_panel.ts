import m from 'mithril';

import {time, Time} from '../base/time';
import {Actions} from '../common/actions';
// import {colorForFtrace} from '../common/colorizer';
import {StringListPatch} from '../common/state';
import {DetailsShell} from '../widgets/details_shell';
import {
  MultiSelectDiff,
  Option as MultiSelectOption,
  PopupMultiSelect,
} from '../widgets/multiselect';
import {PopupPosition} from '../widgets/popup';
import {VirtualScrollContainer} from '../widgets/virtual_scroll_container';
import { rtux_loader } from './rtux_loader';
import {globals} from './globals';
import {Timestamp} from './widgets/timestamp';

const ROW_H = 20;
const PAGE_SIZE = 250;

export class RTUXDetailsTab implements m.ClassComponent {
  private page: number = 0;
  private pageCount: number = 0;

  view(_: m.CVnode<{}>) {
    return m(
      DetailsShell,
      {
        title: this.renderTitle(),
        buttons: this.renderFilterPanel(),
      },
      m(
        VirtualScrollContainer,
        {
          onScroll: this.onScroll,
        },
        m('.ftrace-panel', this.renderRows()),
      ),
    );
  }

  recomputeVisibleRowsAndUpdate(scrollContainer: HTMLElement) {
    const prevPage = this.page;
    const prevPageCount = this.pageCount;

    const visibleRowOffset = Math.floor(scrollContainer.scrollTop / ROW_H);
    const visibleRowCount = Math.ceil(scrollContainer.clientHeight / ROW_H);

    // Work out which "page" we're on
    this.page = Math.floor(visibleRowOffset / PAGE_SIZE) - 1;
    this.pageCount = Math.ceil(visibleRowCount / PAGE_SIZE) + 2;

    if (this.page !== prevPage || this.pageCount !== prevPageCount) {
      globals.dispatch(Actions.updateRtuxPagination({
        offset: Math.max(0, this.page) * PAGE_SIZE,
        count: this.pageCount * PAGE_SIZE,
      }));
    }
  }

  onremove(_: m.CVnodeDOM) {
    globals.dispatch(Actions.updateRtuxPagination({
      offset: 0,
      count: 0,
    }));
  }

  onScroll = (container: HTMLElement) => {
    this.recomputeVisibleRowsAndUpdate(container);
  };

  onRowOver(ts: time) {
    globals.dispatch(Actions.setHoverCursorTimestamp({ts}));
  }

  onRowOut() {
    globals.dispatch(Actions.setHoverCursorTimestamp({ts: Time.INVALID}));
  }

  private renderTitle() {
    if (globals.rtuxPanelData) {
    //   const {numEvents} = globals.rtuxPanelData;
      return `RTUX Details Tab`;
    } else {
      return 'RTUX Details Tab';
    }
  }

  private renderFilterPanel() {
    if (!globals.rtuxCounters) {
      return null;
    }

    const options: MultiSelectOption[] =
        globals.rtuxCounters.map(({name, count}) => {
          return {
            id: name,
            name: `${name} (${count})`,
            checked: !globals.state.ftraceFilter.excludedNames.some(
              (excluded: string) => excluded === name),
          };
        });

    return m(
      PopupMultiSelect,
      {
        label: 'Filter',
        minimal: true,
        icon: 'filter_list_alt',
        popupPosition: PopupPosition.Top,
        options,
        onChange: (diffs: MultiSelectDiff[]) => {
          const excludedNames: StringListPatch[] = diffs.map(
            ({id, checked}) => [checked ? 'remove' : 'add', id],
          );
          globals.dispatchMultiple([
            Actions.updateFtraceFilter({excludedNames}),
            Actions.requestTrackReload({}),
          ]);
        },
      },
    );
  }


  // Render all the rows including the first title row
  private renderRows() {
    const data = globals.rtuxPanelData;
    const rows: m.Children = [];

    rows.push(m(
      `.row`,
      m('.cell.row-header', 'Timestamp'),
      m('.cell.row-header', 'Event'),
      m('.cell.row-header', 'Path'),
    //   m('.cell.row-header', 'Process'),
    //   m('.cell.row-header', 'Args'),
    ));
    // const paths = rtux_loader.getSortedFilePaths();
    if (data) {
      const {events, offset, numEvents} = data;
      for (let i = 0; i < events.length; i++) {
        const {ts, event} = events[i];
        const path = processPaths(i);
        const timestamp = m(Timestamp, {ts});

        const rank = i + offset;

        const color = 'rgb(255, 0, 0)';


        rows.push(m(
          `.row`,
          {
            style: {top: `${(rank + 1.0) * ROW_H}px`},
            onmouseover: this.onRowOver.bind(this, ts),
            onmouseout: this.onRowOut.bind(this),
          },
          m('.cell', timestamp),
          m('.cell', m('span.colour', {style: {background: color}}), event),
        //   m('.cell', paths),
        ));
      }
      return m('.rows', {style: {height: `${numEvents * ROW_H}px`}}, rows);
    } else {
      return m('.rows', rows);
    }
  }
}

async function processPaths(i: number) {
    const paths = await rtux_loader.getSortedFilePaths();
    return paths[i];
}


// // Copyright (C) 2019 The Android Open Source Project
// //
// // Licensed under the Apache License, Version 2.0 (the "License");
// // you may not use size file except in compliance with the License.
// // You may obtain a copy of the License at
// //
// //      http://www.apache.org/licenses/LICENSE-2.0
// //
// // Unless required by applicable law or agreed to in writing, software
// // distributed under the License is distributed on an "AS IS" BASIS,
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// // See the License for the specific language governing permissions and
// // limitations under the License.

// import m from 'mithril';

// import {Actions} from '../common/actions';
// import {translateState} from '../common/thread_state';
// import {THREAD_STATE_TRACK_KIND} from '../tracks/thread_state';
// import {Anchor} from '../widgets/anchor';
// import {DetailsShell} from '../widgets/details_shell';
// import {GridLayout} from '../widgets/grid_layout';
// import {Section} from '../widgets/section';
// import {SqlRef} from '../widgets/sql_ref';
// import {Tree, TreeNode} from '../widgets/tree';

// import {globals, SliceDetails, ThreadDesc} from './globals';
// import {scrollToTrackAndTs} from './scroll_helper';
// import {SlicePanel} from './slice_panel';
// // import {DurationWidget} from './widgets/duration';
// import {Timestamp} from './widgets/timestamp';

// export class RtuxDetailpanel extends SlicePanel {
//   view() {
//     const sliceInfo = globals.sliceDetails;
//     if (sliceInfo.utid === undefined) return;
//     const threadInfo = globals.threads.get(sliceInfo.utid);

//     return m(
//       DetailsShell,
//       {
//         title: 'RTUX Event Details',
//         // description: this.renderDescription(sliceInfo),
//       },
//       m(
//         GridLayout,
//         this.renderDetails(sliceInfo, threadInfo),
//         // this.renderSchedLatencyInfo(sliceInfo),
//       ),
//     );
//   }

//   private renderDetails(sliceInfo: SliceDetails, threadInfo?: ThreadDesc):
//       m.Children {
//     if (!threadInfo || sliceInfo.ts === undefined ||
//         sliceInfo.dur === undefined) {
//       return null;
//     } else {
//       const extras: m.Children = [];

//       for (const [key, value] of this.getProcessThreadDetails(sliceInfo)) {
//         if (value !== undefined) {
//           extras.push(m(TreeNode, {left: key, right: value}));
//         }
//       }

//       const treeNodes = [
//         m(TreeNode, {
//           left: 'Process',
//           right: `${threadInfo.procName} [${threadInfo.pid}]`,
//         }),
//         m(TreeNode, {
//           left: 'Thread',
//           right:
//               m(Anchor,
//                 {
//                   icon: 'call_made',
//                   onclick: () => {
//                     this.goToThread();
//                   },
//                 },
//                 `${threadInfo.threadName} [${threadInfo.tid}]`),
//         }),
//         m(TreeNode, {
//           left: 'Cmdline',
//           right: threadInfo.cmdline,
//         }),
//         m(TreeNode, {
//           left: 'Start time',
//           right: m(Timestamp, {ts: sliceInfo.ts}),
//         }),
//         m(TreeNode, {
//           left: 'Duration',
//           right: this.computeDuration(sliceInfo.ts, sliceInfo.dur),
//         }),
//         m(TreeNode, {
//           left: 'Prio',
//           right: sliceInfo.priority,
//         }),
//         m(TreeNode, {
//           left: 'End State',
//           right: translateState(sliceInfo.endState),
//         }),
//         m(TreeNode, {
//           left: 'SQL ID',
//           right: m(SqlRef, {table: 'sched', id: sliceInfo.id}),
//         }),
//         ...extras,
//       ];

//       return m(
//         Section,
//         {title: 'Details'},
//         m(Tree, treeNodes),
//       );
//     }
//   }

//   goToThread() {
//     const sliceInfo = globals.sliceDetails;
//     if (sliceInfo.utid === undefined) return;
//     const threadInfo = globals.threads.get(sliceInfo.utid);

//     if (sliceInfo.id === undefined || sliceInfo.ts === undefined ||
//         sliceInfo.dur === undefined || sliceInfo.cpu === undefined ||
//         threadInfo === undefined) {
//       return;
//     }

//     let trackKey: string|number|undefined;
//     for (const track of Object.values(globals.state.tracks)) {
//       const trackDesc = globals.trackManager.resolveTrackInfo(track.uri);
//       // TODO(stevegolton): Handle v2.
//       if (trackDesc && trackDesc.kind === THREAD_STATE_TRACK_KIND &&
//           trackDesc.utid === threadInfo.utid) {
//         trackKey = track.key;
//       }
//     }

//     // eslint-disable-next-line @typescript-eslint/strict-boolean-expressions
//     if (trackKey && sliceInfo.threadStateId) {
//       globals.makeSelection(Actions.selectThreadState({
//         id: sliceInfo.threadStateId,
//         trackKey: trackKey.toString(),
//       }));

//       scrollToTrackAndTs(trackKey, sliceInfo.ts, true);
//     }
//   }

//   renderCanvas() {}
// }

