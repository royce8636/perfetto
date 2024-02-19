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

import {globals} from './globals';
import {Timestamp} from './widgets/timestamp';

const ROW_H = 20;
const PAGE_SIZE = 250;

export class RTUXPanel implements m.ClassComponent {
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
      const {numEvents} = globals.rtuxPanelData;
      return `RTUX Events (${numEvents})`;
    } else {
      return 'RTUX Rows';
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
    //   m('.cell.row-header', 'CPU'),
    //   m('.cell.row-header', 'Process'),
    //   m('.cell.row-header', 'Args'),
    ));

    if (data) {
      const {events, offset, numEvents} = data;
      for (let i = 0; i < events.length; i++) {
        const {ts, event} = events[i];

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
        ));
      }
      return m('.rows', {style: {height: `${numEvents * ROW_H}px`}}, rows);
    } else {
      return m('.rows', rows);
    }
  }
}
