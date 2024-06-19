
import m from 'mithril';
import { rtux_loader } from './rtux_loader';
import { Tree, TreeNode } from '../widgets/tree';
import { Section } from '../widgets/section';
import { GridLayout } from '../widgets/grid_layout';
import { DetailsShell } from '../widgets/details_shell';
// import { Time, time } from '../base/time';
// import { Time} from '../base/time';

export class RTUXDetailsTab implements m.ClassComponent{
  imageUrl: string = '';
  // imageTime: time = Time.INVALID;
  // imageTimeString: string = '';

  // oninit() {
  //   rtux_loader.subscribe(this.updateImage.bind(this));
  // }
  // updateImage(imageUrl: string) {
  //   this.imageUrl = imageUrl;
  // }
  view() {
    this.imageUrl = rtux_loader.getImageToDisplay();
    // this.imageTime = rtux_loader.getImageDisplayedTime();
    // this.imageTimeString = this.imageTime ? Time.toTimecode(this.imageTime).toString('\u2009') : 'N/A';
    const match = this.imageUrl.match(/(\d+)\.png$/);
    const imageNumber = match ? match[1] : null;
    const hasImage = this.imageUrl && imageNumber;
    const leftText = hasImage ? [
          m('div', {style: {'font-weight': 'bold', 'text-decoration': 'underline' }}, 'Image Number:'),
          m('div', {style: {'font-weight': 'normal !important' }}, imageNumber),
          // m('div', {style: {'font-weight': 'bold', 'text-decoration': 'underline' }}, 'Displayed Time:'),
          // m('div', {style: {'font-weight': 'normal !important' }}, this.imageTimeString)
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
                // right: rtux_loader.getImageToDisplay() != "" ?
                    // m('img', {src: this.imageUrl, alt: 'Descriptive alt text'},) :
                    // 'No image available',
                right: rtux_loader.getImageToDisplay() != "" ?
                    m('img', {
                        src: this.imageUrl,
                        alt: 'Descriptive alt text',
                        style: {
                            'max-width': '100%',
                            'max-height': '100%',
                            // 'object-fit': 'contain',
                            'object-fit': 'scale-down',
                            'display': 'block', 
                            'margin': '0 auto' 
                        }
                    }) : "No image available",
            }),
          ),
        )),
    );
  }
} 