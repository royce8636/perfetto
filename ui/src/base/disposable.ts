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

// Represents an object that can/should be disposed of to release resources or
// perform cleanup operations.
export interface Disposable {
  dispose(): void;
}

// Perform some operation using a disposable object guaranteeing it is disposed
// of after the operation completes.
// This can be replaced by the native "using" when Typescript 5.2 lands.
// See: https://www.totaltypescript.com/typescript-5-2-new-keyword-using
// Usage:
//   using(createDisposable(), (x) => {doSomethingWith(x)});
export function using<T extends Disposable>(x: T, func?: (x: T) => void) {
  try {
    func && func(x);
  } finally {
    x.dispose();
  }
}
