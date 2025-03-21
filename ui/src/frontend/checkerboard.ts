// Copyright (C) 2018 The Android Open Source Project
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

const LOADING_TEXT = 'lol Loading...';
let LOADING_TEXT_WIDTH = 0;

// Checker board the range [leftPx, rightPx].
export function checkerboard(
  ctx: CanvasRenderingContext2D,
  heightPx: number,
  leftPx: number,
  rightPx: number): void {
  const widthPx = rightPx - leftPx;
  ctx.font = '12px Roboto Condensed';
  ctx.fillStyle = '#eee';
  ctx.fillRect(leftPx, 0, widthPx, heightPx);
  ctx.fillStyle = '#666';
  const oldBaseline = ctx.textBaseline;
  ctx.textBaseline = 'middle';
  if (LOADING_TEXT_WIDTH === 0) {
    LOADING_TEXT_WIDTH = ctx.measureText(LOADING_TEXT).width;
  }
  if (LOADING_TEXT_WIDTH <= widthPx) {
    ctx.fillText(
      LOADING_TEXT,
      leftPx + widthPx / 2 - LOADING_TEXT_WIDTH / 2,
      heightPx / 2);
  }
  ctx.textBaseline = oldBaseline;
}

// Checker board everything between [startPx, endPx] except [leftPx, rightPx].
export function checkerboardExcept(
  ctx: CanvasRenderingContext2D,
  heightPx: number,
  startPx: number,
  endPx: number,
  leftPx: number,
  rightPx: number): void {
  // [leftPx, rightPx] doesn't overlap [startPx, endPx] at all:
  if (rightPx <= startPx || leftPx >= endPx) {
    checkerboard(ctx, heightPx, startPx, endPx);
    return;
  }

  // Checkerboard [startPx, leftPx]:
  if (leftPx > startPx) {
    checkerboard(ctx, heightPx, startPx, leftPx);
  }

  // Checkerboard [rightPx, endPx]:
  if (rightPx < endPx) {
    checkerboard(ctx, heightPx, rightPx, endPx);
  }
}

// Checker board the range [leftPx, rightPx].
export function checkerboard_debug(
  ctx: CanvasRenderingContext2D,
  heightPx: number,
  leftPx: number,
  rightPx: number): void {
  const widthPx = rightPx - leftPx;
  ctx.font = '12px Roboto Condensed';
  ctx.fillStyle = '#eee';
  ctx.fillRect(leftPx, 0, widthPx, heightPx);
  ctx.fillStyle = '#666';
  const oldBaseline = ctx.textBaseline;
  ctx.textBaseline = 'middle';
  const loading_text_debug = `${leftPx} ~ ${rightPx}`;
  if (LOADING_TEXT_WIDTH === 0) {
    LOADING_TEXT_WIDTH = ctx.measureText(LOADING_TEXT).width;
  }
  if (LOADING_TEXT_WIDTH <= widthPx) {
    ctx.fillText(
      loading_text_debug,
      leftPx + widthPx / 2 - LOADING_TEXT_WIDTH / 2,
      heightPx / 2);
  }
  ctx.textBaseline = oldBaseline;
}

// Checker board everything between [startPx, endPx] except [leftPx, rightPx].
export function checkerboardExcept_debug(
  ctx: CanvasRenderingContext2D,
  heightPx: number,
  startPx: number,
  endPx: number,
  leftPx: number,
  rightPx: number): void {
  // [leftPx, rightPx] doesn't overlap [startPx, endPx] at all:
  if (rightPx <= startPx || leftPx >= endPx) {
    checkerboard_debug(ctx, heightPx, startPx, endPx);
    return;
  }

  // Checkerboard [startPx, leftPx]:
  if (leftPx > startPx) {
    checkerboard_debug(ctx, heightPx, startPx, leftPx);
  }

  // Checkerboard [rightPx, endPx]:
  if (rightPx < endPx) {
    checkerboard_debug(ctx, heightPx, rightPx, endPx);
  }
}