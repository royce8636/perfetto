
import m from 'mithril';
import { rtux_loader } from './rtux_loader';
import { Tree, TreeNode } from '../widgets/tree';
import { Section } from '../widgets/section';
import { GridLayout } from '../widgets/grid_layout';
import { DetailsShell } from '../widgets/details_shell';

export class RTUXDetailsTab implements m.ClassComponent{
  imageUrl: string = '';

  // oninit() {
  //   rtux_loader.subscribe(this.updateImage.bind(this));
  // }
  // updateImage(imageUrl: string) {
  //   this.imageUrl = imageUrl;
  // }
  view() {
    this.imageUrl = rtux_loader.getImageToDisplay();
    const match = this.imageUrl.match(/(\d+)\.png$/);
    const imageNumber = match ? match[1] : null;
    const hasImage = this.imageUrl && imageNumber;
    return m(
      DetailsShell,
      m(GridLayout,
        m(
          Section,
          {title: 'Properties'},
          m(
            Tree,
            m(TreeNode, {
            //   left: 'Image',
                left: hasImage ? `Image Number: ${imageNumber}` : 'No Number',
                // left: this.imageUrl,
                // right: m('img', {src: rtux_loader.getImageToDisplay(), alt: 'Descriptive alt text'}),
                right: rtux_loader.getImageToDisplay() != "" ?
                // m('img', {src: rtux_loader.getImageToDisplay(), alt: 'Descriptive alt text'}) :
                // 'No image available',
                m('img', {src: this.imageUrl, alt: 'Descriptive alt text'}) :
                'No image available',
            
            }),
          ),
        )),
    );
  }
} 