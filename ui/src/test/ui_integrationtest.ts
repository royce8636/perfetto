// Copyright (C) 2021 The Android Open Source Project
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

import * as fs from 'fs';
import * as path from 'path';
import * as puppeteer from 'puppeteer';

import {assertExists} from '../base/logging';

import {
  compareScreenshots,
  failIfTraceProcessorHttpdIsActive,
  getTestTracePath,
  waitForPerfettoIdle
} from './perfetto_ui_test_helper';

declare var global: {__BROWSER__: puppeteer.Browser;};
const browser = assertExists(global.__BROWSER__);
const expectedScreenshotPath = path.join('test', 'data', 'ui-screenshots');

async function getPage(): Promise<puppeteer.Page> {
  const pages = (await browser.pages());
  expect(pages.length).toBe(1);
  return pages[pages.length - 1];
}

// Executed once at the beginning of the test. Navigates to the UI.
beforeAll(async () => {
  await failIfTraceProcessorHttpdIsActive();
  jest.setTimeout(60000);
  const page = await getPage();
  await page.setViewport({width: 1920, height: 1080});
  await page.goto('http://localhost:10000/?testing=1');
});

// After each test (regardless of nesting) capture a screenshot named after the
// test('') name and compare the screenshot with the expected one in
// /test/data/ui-screenshots.
afterEach(async () => {
  let testName = expect.getState().currentTestName;
  testName = testName.replace(/[^a-z0-9-]/gmi, '_').toLowerCase();
  const page = await getPage();

  // cwd() is set to //out/ui when running tests, just create a subdir in there.
  // The CI picks up this directory and uploads to GCS after every failed run.
  const tmpDir = path.resolve('./ui-test-artifacts');
  if (!fs.existsSync(tmpDir)) fs.mkdirSync(tmpDir);
  const screenshotName = `ui-${testName}.png`;
  const actualFilename = path.join(tmpDir, screenshotName);
  const expectedFilename = path.join(expectedScreenshotPath, screenshotName);
  await page.screenshot({path: actualFilename});
  const rebaseline = process.env['PERFETTO_UI_TESTS_REBASELINE'] === '1';
  if (rebaseline) {
    console.log('Saving reference screenshot into', expectedFilename);
    fs.copyFileSync(actualFilename, expectedFilename);
  } else {
    await compareScreenshots(actualFilename, expectedFilename);
  }
});

describe('android_trace_30s', () => {
  test('load', async () => {
    const page = await getPage();
    const file = await page.waitForSelector('input.trace_file');
    const tracePath = getTestTracePath('example_android_trace_30s.pb');
    assertExists(file).uploadFile(tracePath);
    await waitForPerfettoIdle(page);
  });

  test('expand_camera', async () => {
    const page = await getPage();
    await page.click('.main-canvas');
    await page.click('h1[title="com.google.android.GoogleCamera 5506"]');
    await page.evaluate(() => {
      document.querySelector('.scrolling-panel-container')!.scrollTo(0, 400);
    });
    await waitForPerfettoIdle(page);
  });

  test('search', async () => {
    const page = await getPage();
    const searchInput = '.omnibox input';
    await page.focus(searchInput);
    await page.keyboard.type('TrimMaps');
    await waitForPerfettoIdle(page);
    for (let i = 0; i < 10; i++) {
      await page.keyboard.type('\n');
    }
    await waitForPerfettoIdle(page);
  });
});

describe('chrome_rendering_desktop', () => {
  test('load', async () => {
    const page = await getPage();
    const file = await page.waitForSelector('input.trace_file');
    const tracePath = getTestTracePath('chrome_rendering_desktop.pftrace');
    assertExists(file).uploadFile(tracePath);
    await waitForPerfettoIdle(page);
  });

  test('expand_browser_proc', async () => {
    const page = await getPage();
    await page.click('.main-canvas');
    await page.click('h1[title="Browser 12685"]');
    await waitForPerfettoIdle(page);
  });

  test('select_slice_with_flows', async () => {
    const page = await getPage();
    const searchInput = '.omnibox input';
    await page.focus(searchInput);
    await page.keyboard.type('GenerateRenderPass');
    await waitForPerfettoIdle(page);
    for (let i = 0; i < 3; i++) {
      await page.keyboard.type('\n');
    }
    await waitForPerfettoIdle(page);
    await page.focus('canvas');
    await page.keyboard.type('f');  // Zoom to selection
    await waitForPerfettoIdle(page);
  });
});

describe('routing', () => {
  describe('open_two_traces_then_go_back', () => {
    let page: puppeteer.Page;

    beforeAll(async () => {
      page = await getPage();
    });

    test('open_first_trace_from_url', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1/#!/?url=http://localhost:10000/test/data/chrome_memory_snapshot.pftrace');
      await waitForPerfettoIdle(page);
    });

    test('open_second_trace_from_url', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1#!/?url=http://localhost:10000/test/data/chrome_scroll_without_vsync.pftrace');
      await waitForPerfettoIdle(page);
    });

    test('access_subpage_then_go_back', async () => {
      await waitForPerfettoIdle(page);
      await page.goto(
          'http://localhost:10000/?testing=1/#!/metrics?trace_id=76c25a80-25dd-1eb7-2246-d7b3c7a10f91');
      await page.goBack();
      await waitForPerfettoIdle(page);
    });
  });

  describe('start_from_no_trace', () => {
    let page: puppeteer.Page;

    beforeAll(async () => {
      page = await getPage();
    });

    test('go_to_page_with_no_trace', async () => {
      await page.goto('http://localhost:10000/?testing=1#!/info');
      await waitForPerfettoIdle(page);
    });

    test('open_trace ', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1#!/viewer?trace_id=76c25a80-25dd-1eb7-2246-d7b3c7a10f91');
      await waitForPerfettoIdle(page);
    });

    test('refresh', async () => {
      await page.reload();
      await waitForPerfettoIdle(page);
    });

    test('open_second_trace', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1#!/viewer?trace_id=00000000-0000-0000-e13c-bd7db4ff646f');
      await waitForPerfettoIdle(page);

      // click on the 'Continue' button in the interstitial
      await page.click('[id="trace_id_open"]');
      await waitForPerfettoIdle(page);
    });

    test('go_back_to_first_trace', async () => {
      await page.goBack();
      await waitForPerfettoIdle(page);
      // click on the 'Continue' button in the interstitial
      await page.click('[id="trace_id_open"]');
      await waitForPerfettoIdle(page);
    });

    test('open_invalid_trace', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1#!/viewer?trace_id=invalid');
      await waitForPerfettoIdle(page);
    });
  });

  describe('navigate', () => {
    let page: puppeteer.Page;

    beforeAll(async () => {
      page = await getPage();
    });

    test('open_trace_from_url', async () => {
      await page.goto('http://localhost:10000/?testing=1');
      await page.goto(
          'http://localhost:10000/?testing=1/#!/?url=http://localhost:10000/test/data/chrome_memory_snapshot.pftrace');
      await waitForPerfettoIdle(page);
    });

    test('navigate_back_and_forward', async () => {
      await page.goto(
          'http://localhost:10000/?testing=1#!/info?trace_id=00000000-0000-0000-e13c-bd7db4ff646f');
      await page.goto(
          'http://localhost:10000/?testing=1#!/metrics?trace_id=00000000-0000-0000-e13c-bd7db4ff646f');
      await page.goBack();
      await page.goBack();
      await waitForPerfettoIdle(page);  // wait for trace to load
      await page.goBack();
      await page.goForward();
      await waitForPerfettoIdle(page);  // wait for trace to load
      await page.goForward();
      await page.goForward();
      await waitForPerfettoIdle(page);
    });
  });

  test('open_trace_and_go_back_to_landing_page', async () => {
    const page = await getPage();
    await page.goto('http://localhost:10000/?testing=1');
    await page.goto(
        'http://localhost:10000/?testing=1#!/viewer?trace_id=76c25a80-25dd-1eb7-2246-d7b3c7a10f91');
    await waitForPerfettoIdle(page);
    await page.goBack();
    await waitForPerfettoIdle(page);
  });

  test('open_invalid_trace_from_blank_page', async () => {
    const page = await getPage();
    await page.goto('about:blank');
    await page.goto(
        'http://localhost:10000/?testing=1#!/viewer?trace_id=invalid');
    await waitForPerfettoIdle(page);
  });
});
