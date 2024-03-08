
import m from 'mithril';
import { rtux_loader } from './rtux_loader';
import { Tree, TreeNode } from '../widgets/tree';
import { Section } from '../widgets/section';
import { GridLayout } from '../widgets/grid_layout';
import { DetailsShell } from '../widgets/details_shell';
import { Time, time } from '../base/time';

export class RTUXDetailsTab implements m.ClassComponent{
  imageUrl: string = '';
  imageTime: time = Time.INVALID;

  // oninit() {
  //   rtux_loader.subscribe(this.updateImage.bind(this));
  // }
  // updateImage(imageUrl: string) {
  //   this.imageUrl = imageUrl;
  // }
  view() {
    this.imageUrl = rtux_loader.getImageToDisplay();
    this.imageTime = rtux_loader.getImageDisplayedTime();
    const match = this.imageUrl.match(/(\d+)\.png$/);
    const imageNumber = match ? match[1] : null;
    const hasImage = this.imageUrl && imageNumber;
    const leftText = hasImage ? [
          m('div', `Image Number: ${imageNumber}`),
          m('div', `Displayed Time: ${this.imageTime ? this.imageTime : 'N/A'}`)
      ]: 'No Image Available';
  
    return m(
      DetailsShell,
      m(GridLayout,
        m(
          Section,
          {title: 'Properties'},
          m(
            Tree,
            m(TreeNode, {
                // left: hasImage ? `Image Number: ${imageNumber}` : 'No Number',
                left: leftText,
                right: rtux_loader.getImageToDisplay() != "" ?
                    m('img', {src: this.imageUrl, alt: 'Descriptive alt text'}) :
                    'No image available',
            
            }),
          ),
        )),
    );
  }
} 