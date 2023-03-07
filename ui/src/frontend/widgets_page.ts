// Copyright (C) 2023 The Android Open Source Project
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

import {Anchor} from './anchor';
import {classNames} from './classnames';
import {globals} from './globals';
import {createPage} from './pages';
import {TableShowcase} from './tables/table_showcase';
import {Button} from './widgets/button';
import {Checkbox} from './widgets/checkbox';
import {EmptyState} from './widgets/empty_state';
import {Icon} from './widgets/icon';
import {Popup, PopupPosition} from './widgets/popup';
import {Portal} from './widgets/portal';
import {TextInput} from './widgets/text_input';

function PortalButton() {
  let portalOpen = false;

  return {
    view: function({attrs}: any) {
      const {
        zIndex = true,
        absolute = true,
        top = true,
      } = attrs;
      return [
        m(Button, {
          label: 'Toggle Portal',
          onclick: () => {
            portalOpen = !portalOpen;
            globals.rafScheduler.scheduleFullRedraw();
          },
        }),
        portalOpen &&
            m(Portal,
              {
                style: {
                  position: absolute && 'absolute',
                  top: top && '0',
                  zIndex: zIndex ? '10' : '0',
                  background: 'white',
                },
              },
              m('', `A very simple portal - a div rendered outside of the normal
              flow of the page`)),
      ];
    },
  };
}

function lorem() {
  const text =
      `Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod
      tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim
      veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea
      commodo consequat.Duis aute irure dolor in reprehenderit in voluptate
      velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat
      cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id
      est laborum.`;
  return m('', {style: {width: '200px'}}, text);
}


function ControlledPopup() {
  let popupOpen = false;

  return {
    view: function() {
      return m(
          Popup,
          {
            trigger:
                m(Button, {label: `${popupOpen ? 'Close' : 'Open'} Popup`}),
            isOpen: popupOpen,
            onChange: (shouldOpen: boolean) => popupOpen = shouldOpen,
          },
          m(Button, {
            label: 'Close Popup',
            onclick: () => {
              popupOpen = !popupOpen;
              globals.rafScheduler.scheduleFullRedraw();
            },
          }),
      );
    },
  };
}

type Options = {
  [key: string]: EnumOption|boolean
};

interface WidgetShowcaseAttrs {
  initialOpts?: Options;
  renderWidget: (options: any) => any;
  wide?: boolean;
}

class EnumOption {
  constructor(public initial: string, public options: string[]) {}
}

// A little helper class to render any vnode with a dynamic set of options
class WidgetShowcase implements m.ClassComponent<WidgetShowcaseAttrs> {
  private optValues: any = {};
  private opts?: Options;

  renderOptions(listItems: m.Child[]): m.Child {
    if (listItems.length === 0) {
      return null;
    }
    return m(
        '.widget-controls',
        m('h3', 'Options'),
        m('ul', listItems),
    );
  }

  oninit({attrs: {initialOpts: opts}}: m.Vnode<WidgetShowcaseAttrs, this>) {
    this.opts = opts;
    if (opts) {
      // Make the initial options values
      for (const key in opts) {
        if (Object.prototype.hasOwnProperty.call(opts, key)) {
          const option = opts[key];
          if (option instanceof EnumOption) {
            this.optValues[key] = option.initial;
          } else if (typeof option === 'boolean') {
            this.optValues[key] = option;
          }
        }
      }
    }
  }

  view({attrs: {renderWidget, wide}}: m.CVnode<WidgetShowcaseAttrs>) {
    const listItems = [];

    if (this.opts) {
      for (const key in this.opts) {
        if (Object.prototype.hasOwnProperty.call(this.opts, key)) {
          listItems.push(m('li', this.renderControlForOption(key)));
        }
      }
    }

    return [
      m(
          '.widget-block',
          m(
              'div',
              {
                class: classNames(
                    'widget-container',
                    wide && 'widget-container-wide',
                    ),
              },
              renderWidget(this.optValues),
              ),
          this.renderOptions(listItems),
          ),
    ];
  }

  private renderControlForOption(key: string) {
    if (!this.opts) return null;
    const value = this.opts[key];
    if (value instanceof EnumOption) {
      return this.renderEnumOption(key, value);
    } else if (typeof value === 'boolean') {
      return this.renderBooleanOption(key);
    } else {
      return null;
    }
  }

  private renderBooleanOption(key: string) {
    return m(Checkbox, {
      checked: this.optValues[key],
      label: key,
      onchange: () => {
        this.optValues[key] = !this.optValues[key];
        globals.rafScheduler.scheduleFullRedraw();
      },
    });
  }

  private renderEnumOption(key: string, opt: EnumOption) {
    const optionElements = opt.options.map((option: string) => {
      return m('option', {value: option}, option);
    });
    return m(
        'select',
        {
          value: this.optValues[key],
          onchange: (e: Event) => {
            const el = e.target as HTMLSelectElement;
            this.optValues[key] = el.value;
            globals.rafScheduler.scheduleFullRedraw();
          },
        },
        optionElements);
  }
}


export const WidgetsPage = createPage({
  view() {
    return m(
        '.widgets-page',
        m('h1', 'Widgets'),
        m('h2', 'Button'),
        m(WidgetShowcase, {
          renderWidget: ({label, icon, ...rest}) => m(Button, {
            icon: icon ? 'send' : undefined,
            label: label ? 'Button' : '',
            ...rest,
          }),
          initialOpts: {
            label: true,
            icon: false,
            disabled: false,
            minimal: false,
            active: false,
            compact: false,
          },
        }),
        m('h2', 'Checkbox'),
        m(WidgetShowcase, {
          renderWidget: (opts) => m(Checkbox, {label: 'Checkbox', ...opts}),
          initialOpts: {
            disabled: false,
          },
        }),
        m('h2', 'Text Input'),
        m(WidgetShowcase, {
          renderWidget: ({placeholder, ...rest}) => m(TextInput, {
            placeholder: placeholder ? 'Placeholder...' : '',
            ...rest,
          }),
          initialOpts: {
            placeholder: true,
            disabled: false,
          },
        }),
        m('h2', 'Empty State'),
        m(WidgetShowcase, {
          renderWidget: ({header, content}) =>
              m(EmptyState,
                {
                  header: header && 'No search results found...',
                },
                content && m(Button, {label: 'Try again'})),
          initialOpts: {
            header: true,
            content: true,
          },
        }),
        m('h2', 'Anchor'),
        m(WidgetShowcase, {
          renderWidget: ({icon}) => m(
              Anchor,
              {
                icon: icon && 'open_in_new',
                href: 'https://perfetto.dev/docs/',
                target: '_blank',
              },
              'Docs',
              ),
          initialOpts: {
            icon: true,
          },
        }),
        m('h2', 'Table'),
        m(WidgetShowcase,
          {renderWidget: () => m(TableShowcase), initialOpts: {}, wide: true}),
        m('h2', 'Portal'),
        m('p', `A portal is a div rendered out of normal flow of the
        hierarchy.`),
        m(WidgetShowcase, {
          renderWidget: (opts) => m(PortalButton, opts),
          initialOpts: {
            absolute: true,
            zIndex: true,
            top: true,
          },
        }),
        m('h2', 'Popup'),
        m('p', `A popup is a nicely styled portal element whose position is
        dynamically updated to appear to float alongside a specific element on
        the page, even as the element is moved and scrolled around.`),
        m(WidgetShowcase, {
          renderWidget: (opts) => m(
              Popup,
              {
                trigger: m(Button, {label: 'Toggle Popup'}),
                ...opts,
              },
              lorem(),
              ),
          initialOpts: {
            position: new EnumOption(
                PopupPosition.Auto,
                Object.values(PopupPosition),
                ),
            closeOnEscape: true,
            closeOnOutsideClick: true,
          },
        }),
        m('h2', 'Controlled Popup'),
        m('p', `The open/close state of a controlled popup is passed in via
        the 'isOpen' attribute. This means we can get open or close the popup
        from wherever we like. E.g. from a button inside the popup.
        Keeping this state external also means we can modify other parts of the
        page depending on whether the popup is open or not, such as the text
        on this button.
        Note, this is the same component as the popup above, but used in
        controlled mode.`),
        m(WidgetShowcase, {
          renderWidget: (opts) => m(ControlledPopup, opts),
          initialOpts: {},
        }),
        m('h2', 'Icon'),
        m(WidgetShowcase, {
          renderWidget: (opts) => m(Icon, {icon: 'star', ...opts}),
          initialOpts: {filled: false},
        }));
  },
});
