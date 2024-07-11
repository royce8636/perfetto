import m from 'mithril';
import {time, Time} from '../base/time';
import {Actions} from '../common/actions';
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
import {RtuxPanelData} from './globals';

const ROW_H = 20;
const PAGE_SIZE = 250;
const DEFAULT_COLUMN_WIDTH = 100;

interface Column {
  name: string;
  width: number;
}

export class RTUXPanel implements m.ClassComponent {
  private page: number = 0;
  private pageCount: number = 0;
  private columns: Column[] = [
    {name: 'Timestamp', width: DEFAULT_COLUMN_WIDTH},
    {name: 'Name', width: DEFAULT_COLUMN_WIDTH * 2},
    {name: 'Type', width: DEFAULT_COLUMN_WIDTH},
    {name: 'Level', width: DEFAULT_COLUMN_WIDTH},
  ];

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

  private renderRows() {
    const data: RtuxPanelData | undefined = globals.rtuxPanelData;
    const rows: m.Children = [];

    rows.push(m('.row', this.columns.map((col) => 
      m('.cell.row-header', {
        style: {width: `${col.width}px`},
      }, col.name)
    )));

    if (data) {
      const {events, offset, numEvents} = data;
      
      const sortedEvents = [...events].sort((a, b) => Number(a.ts) - Number(b.ts));

      for (let i = 0; i < sortedEvents.length; i++) {
        const {ts, event} = sortedEvents[i];
        const parsedEvent = this.parseEvent(event);

        const timestamp = m(Timestamp, {ts});
        const rank = i + offset;
        const color = parsedEvent.type === 'detection' ? 'rgb(255, 0, 0)' : 'rgb(0, 0, 255)';

        rows.push(m(
          '.row',
          {
            style: {top: `${(rank + 1.0) * ROW_H}px`},
            onmouseover: this.onRowOver.bind(this, ts),
            onmouseout: this.onRowOut.bind(this),
          },
          m('.cell', {style: {width: `${this.columns[0].width}px`}}, timestamp),
          m('.cell', {style: {width: `${this.columns[1].width}px`}}, [
            m('span.colour', {style: {background: color}}),
            ' ',
            parsedEvent.name
          ]),
          m('.cell', {style: {width: `${this.columns[2].width}px`}}, parsedEvent.type),
          m('.cell', {style: {width: `${this.columns[3].width}px`}}, parsedEvent.level || 'N/A'),
        ));
      }
      return m('.rows', {style: {height: `${numEvents * ROW_H}px`}}, rows);
    } else {
      return m('.rows', rows);
    }
  }

  private parseEvent(eventString: string): { type: string; name: string; level: string } {
    const match = eventString.match(/^(\w+):(.+?)\s*\((\d+):(\d+)\)$/);
    if (match) {
      const [, type, name, , level] = match;
      return { type, name: name.trim(), level };
    }
    // Fallback parsing if the regex doesn't match
    const [type, ...rest] = eventString.split(':');
    const name = rest.join(':').trim();
    const level = 'N/A';
    return { type, name, level };
  }

}