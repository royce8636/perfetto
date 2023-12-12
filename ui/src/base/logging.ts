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

import {VERSION} from '../gen/perfetto_version';

export type ErrorType = 'ERROR'|'PROMISE_REJ'|'OTHER';
export interface ErrorStackEntry {
  name: string;      // e.g. renderCanvas
  location: string;  // e.g. frontend_bundle.js:12:3
}
export interface ErrorDetails {
  errType: ErrorType;
  message: string;  // Uncaught StoreError: No such subtree: tracks,1374,state
  stack: ErrorStackEntry[];
}

export type ErrorHandler = (err: ErrorDetails) => void;
const errorHandlers: ErrorHandler[] = [];

export function assertExists<A>(value: A | null | undefined): A {
  if (value === null || value === undefined) {
    throw new Error('Value doesn\'t exist');
  }
  return value;
}

export function assertTrue(value: boolean, optMsg?: string) {
  if (!value) {
    throw new Error(optMsg ?? 'Failed assertion');
  }
}

export function assertFalse(value: boolean, optMsg?: string) {
  assertTrue(!value, optMsg);
}

export function addErrorHandler(handler: ErrorHandler) {
  if (!errorHandlers.includes(handler)) {
    errorHandlers.push(handler);
  }
}

export function reportError(err: ErrorEvent|PromiseRejectionEvent|{}) {
  let errorObj = undefined;
  let errMsg = '';
  let errType: ErrorType;
  const stack: ErrorStackEntry[] = [];
  const baseUrl = `${location.protocol}//${location.host}`;

  if (err instanceof ErrorEvent) {
    errType = 'ERROR';
    errMsg = err.message;
    errorObj = err.error;
  } else if (err instanceof PromiseRejectionEvent) {
    errType = 'PROMISE_REJ';
    errMsg = `PromiseRejection: ${err.reason}`;
    errorObj = err.reason;
  } else {
    errType = 'OTHER';
    errMsg = `Err: ${err}`;
  }
  if (errorObj !== undefined && errorObj !== null) {
    const maybeStack = (errorObj as {stack?: string}).stack;
    let errStack = maybeStack !== undefined ? `${maybeStack}` : '';
    errStack = errStack.replaceAll(/\r/g, '');  // Strip Windows CR.
    for (let line of errStack.split('\n')) {
      if (errMsg.includes(line)) continue;
      // Chrome, Firefox and safari don't agree on the stack format:
      // Chrome: prefixes entries with a '  at ' and uses the format
      //         function(https://url:line:col), e.g.
      //         '    at FooBar (https://.../frontend_bundle.js:2073:15)'
      //         however, if the function name is not known, it prints just:
      //         '     at https://.../frontend_bundle.js:2073:15'
      //         or also:
      //         '     at <anonymous>:5:11'
      // Firefox and Safari: don't have any prefix and use @ as a separator:
      //         redrawCanvas@https://.../frontend_bundle.js:468814:26
      //         @debugger eval code:1:32

      // Here we first normalize Chrome into the Firefox/Safari format by
      // removing the '   at ' prefix and replacing (xxx)$ into @xxx.
      line = line.replace(/^\s*at\s*/, '');
      line = line.replace(/\s*\(([^)]+)\)$/, '@$1');

      // This leaves us still with two possible options here:
      // 1. FooBar@https://ui.perfetto.dev/v123/frontend_bundle.js:2073:15
      // 2. https://ui.perfetto.dev/v123/frontend_bundle.js:2073:15
      const lastAt = line.lastIndexOf('@');
      let entryName = '';
      let entryLocation = '';
      if (lastAt >= 0) {
        entryLocation = line.substring(lastAt + 1);
        entryName = line.substring(0, lastAt);
      } else {
        entryLocation = line;
      }

      // Remove redundant https://ui.perfetto.dev/v38.0-d6ed090ee/ as we have
      // that information already and don't need to repeat it on each line.
      if (entryLocation.includes(baseUrl)) {
        entryLocation = entryLocation.replace(baseUrl, '');
        entryLocation = entryLocation.replace(`/${VERSION}/`, '');
      }
      stack.push({name: entryName, location: entryLocation});
    }
  }
  // Invoke all the handlers registered through addErrorHandler.
  // There are usually two handlers registered, one for the UI (error_dialog.ts)
  // and one for Analytics (analytics.ts).
  for (const handler of errorHandlers) {
    handler({
      errType,
      message: errMsg,
      stack,
    } as ErrorDetails);
  }
}

// This function serves two purposes.
// 1) A runtime check - if we are ever called, we throw an exception.
// This is useful for checking that code we suspect should never be reached is
// actually never reached.
// 2) A compile time check where typescript asserts that the value passed can be
// cast to the "never" type.
// This is useful for ensuring we exhastively check union types.
export function assertUnreachable(_x: never) {
  throw new Error('This code should not be reachable');
}
