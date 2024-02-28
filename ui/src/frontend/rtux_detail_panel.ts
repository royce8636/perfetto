import m from 'mithril';


import { rtux_loader } from './rtux_loader';
import {globals} from './globals';
import {Timestamp} from './widgets/timestamp';
import {Tree, TreeNode} from '../widgets/tree';
import {Section} from '../widgets/section';
import {GridLayout} from '../widgets/grid_layout';
import {DetailsShell} from '../widgets/details_shell';

export class RTUXDetailsTab implements m.ClassComponent {
    view() {
        const imageUrl = rtux_loader.getImageToDisplay();
        const counterInfo = globals.counterDetails;
        if (counterInfo.startTime && counterInfo.name !== undefined &&
            counterInfo.value !== undefined && counterInfo.delta !== undefined &&
            counterInfo.duration !== undefined) {
          return m(
            DetailsShell,
            {title: 'Counter', description: `${counterInfo.name}`},
            m(GridLayout,
              m(
                Section,
                {title: 'Properties'},
                m(
                  Tree,
                  m(TreeNode, {left: 'Name', right: `${counterInfo.name}`}),
                  m(TreeNode, {
                    left: 'Start time',
                    right: m(Timestamp, {ts: counterInfo.startTime}),
                  }),
                  m(TreeNode, {
                    left: 'Value',
                    right: `${counterInfo.value.toLocaleString()}`,
                  }),
                  m(TreeNode, {
                    left: 'Delta',
                    right: `${counterInfo.delta.toLocaleString()}`,
                  }),
                  m(TreeNode, {
                    left: 'Image',
                    right: m('img', {src: imageUrl, alt: 'Descriptive alt text'}),
                  }),
                ),
              )),
          );
        } else {
          return m(DetailsShell, {title: 'Counter', description: 'Loading...'});
        }
      }
    
      renderCanvas() {}
    }