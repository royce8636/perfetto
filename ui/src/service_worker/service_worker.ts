// Copyright (C) 2020 The Android Open Source Project
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

// This script handles the caching of the UI resources, allowing it to work
// offline (as long as the UI site has been visited at least once).
// Design doc: http://go/perfetto-offline.

// When a new version of the UI is released (e.g. v1 -> v2), the following
// happens on the next visit:
// 1. The v1 (old) service worker is activated. At this point we don't know yet
//    that v2 is released.
// 2. /index.html is requested. The SW intercepts the request and serves it from
//    the network.
// 3a If the request fails (offline / server unreachable) or times out, the old
//    v1 is served.
// 3b If the request succeeds, the browser receives the index.html for v2. That
//    will try to fetch resources from /v2/frontend_bundle.ts.
// 4. When the SW sees the /v2/ request, will have a cache miss and will issue
//    a network fetch(), returning the fresh /v2/ content.
// 4. The v2 site will call serviceWorker.register('service_worker.js?v=v2').
//    This (i.e. the different querystring) will cause a re-installation of the
//    service worker (even if the service_worker.js script itself is unchanged).
// 5. In the "install" step, the service_worker.js script will fetch the newer
//    version (v2).
//    Note: the v2 will be fetched twice, once upon the first request that
//    causes causes a cache-miss, the second time while re-installing the SW.
//    The  latter though will hit a HTTP 304 (Not Changed) and will be served
//    from the browser cache after the revalidation request.
// 6. The 'activate' handler is triggered. The old v1 cache is deleted at this
//    point.

declare let self: ServiceWorkerGlobalScope;
export {};

const LOG_TAG = `ServiceWorker: `;
const CACHE_NAME = 'ui-perfetto-dev';

// If the fetch() for the / doesn't respond within 3s, return a cached version.
// This is to avoid that a user waits too much if on a flaky network.
const INDEX_TIMEOUT_MS = 3000;

// Use more relaxed timeouts when caching the subresources for the new version
// in the background.
const INSTALL_TIMEOUT_MS = 30000;

// The install() event is fired:
// 1. On the first visit, when there is no SW installed.
// 2. Every time the user opens the site and the version has been updated (they
//    will get the newer version regardless, unless we hit INDEX_TIMEOUT_MS).
// The latter happens because:
// - / (index.html) is always served from the network (% timeout) and it pulls
//   /v1.2-sha/frontend_bundle.js.
// - /v1.2-sha/frontend_bundle.js will register /service_worker.js?v=v1.2-sha.
// The service_worker.js script itself never changes, but the browser
// re-installs it because the version in the V? query-string argument changes.
// The reinstallation will cache the new files from the v.1.2-sha/manifest.json.
self.addEventListener('install', (event) => {
  const doInstall = async () => {
    // If we can not access the cache we must give up on the service
    // worker:
    let bypass = true;
    try {
      bypass = await caches.has('BYPASS_SERVICE_WORKER');
    } catch (_) {
      // TODO(288483453)
    }
    if (bypass) {
      // Throw will prevent the installation.
      throw new Error(LOG_TAG + 'skipping installation, bypass enabled');
    }

    // Delete old cache entries from the pre-feb-2021 service worker.
    try {
      for (const key of await caches.keys()) {
        if (key.startsWith('dist-')) {
          await caches.delete(key);
        }
      }
    } catch (_) {
      // TODO(288483453)
      // It's desirable to delete the old entries but it's not actually
      // damaging to keep them around so don't give up on the
      // installation if this fails.
    }

    // The UI should register this as service_worker.js?v=v1.2-sha. Extract the
    // version number and pre-fetch all the contents for the version.
    const match = /\bv=([\w.-]*)/.exec(location.search);
    if (!match) {
      throw new Error(
          'Failed to install. Was epecting a query string like ' +
          `?v=v1.2-sha query string, got "${location.search}" instead`);
    }
    await installAppVersionIntoCache(match[1]);

    // skipWaiting() still waits for the install to be complete. Without this
    // call, the new version would be activated only when all tabs are closed.
    // Instead, we ask to activate it immediately. This is safe because the
    // subresources are versioned (e.g. /v1.2-sha/frontend_bundle.js). Even if
    // there is an old UI tab opened while we activate() a newer version, the
    // activate() would just cause cache-misses, hence fetch from the network,
    // for the old tab.
    self.skipWaiting();
  };
  event.waitUntil(doInstall());
});

self.addEventListener('activate', (event) => {
  console.info(LOG_TAG + 'activated');
  const doActivate = async () => {
    // This makes a difference only for the very first load, when no service
    // worker is present. In all the other cases the skipWaiting() will hot-swap
    // the active service worker anyways.
    await self.clients.claim();
  };
  event.waitUntil(doActivate());
});

self.addEventListener('fetch', (event) => {
  // The early return here will cause the browser to fall back on standard
  // network-based fetch.
  if (!shouldHandleHttpRequest(event.request)) {
    console.debug(LOG_TAG + `serving ${event.request.url} from network`);
    return;
  }

  event.respondWith(handleHttpRequest(event.request));
});


function shouldHandleHttpRequest(req: Request): boolean {
  // Suppress warning: 'only-if-cached' can be set only with 'same-origin' mode.
  // This seems to be a chromium bug. An internal code search suggests this is a
  // socially acceptable workaround.
  if (req.cache === 'only-if-cached' && req.mode !== 'same-origin') {
    return false;
  }

  const url = new URL(req.url);
  if (url.pathname === '/live_reload') return false;

  if (req.destination === 'image' && url.origin !== self.location.origin) {
    console.debug(LOG_TAG + `Not caching image ${req.url}`)
    return false;
  }
  return req.method === 'GET' && url.origin === self.location.origin;
}

async function handleHttpRequest(req: Request): Promise<Response> {
  if (!shouldHandleHttpRequest(req)) {
    throw new Error(LOG_TAG + `${req.url} shouldn't have been handled`);
  }

  // We serve from the cache even if req.cache == 'no-cache'. It's a bit
  // contra-intuitive but it's the most consistent option. If the user hits the
  // reload button*, the browser requests the "/" index with a 'no-cache' fetch.
  // However all the other resources (css, js, ...) are requested with a
  // 'default' fetch (this is just how Chrome works, it's not us). If we bypass
  // the service worker cache when we get a 'no-cache' request, we can end up in
  // an inconsistent state where the index.html is more recent than the other
  // resources, which is undesirable.
  // * Only Ctrl+R. Ctrl+Shift+R will always bypass service-worker for all the
  // requests (index.html and the rest) made in that tab.

  const cacheOps = {cacheName: CACHE_NAME} as CacheQueryOptions;
  const url = new URL(req.url);
  if (url.pathname === '/') {
    try {
      console.debug(LOG_TAG + `Fetching live ${req.url}`);
      // The await bleow is needed to fall through in case of an exception.
      return await fetchWithTimeout(req, INDEX_TIMEOUT_MS);
    } catch (err) {
      console.warn(LOG_TAG + `Failed to fetch ${req.url}, using cache.`, err);
      // Fall through the code below.
    }
  } else if (url.pathname === '/offline') {
    // Escape hatch to force serving the offline version without attempting the
    // network fetch.
    const cachedRes = await caches.match(new Request('/'), cacheOps);
    if (cachedRes) return cachedRes;
  }

  const cachedRes = await caches.match(req, cacheOps);
  if (cachedRes) {
    console.debug(LOG_TAG + `serving ${req.url} from cache`);
    return cachedRes;
  }

  // In any other case, just propagate the fetch on the network, which is the
  // safe behavior.
  console.warn(LOG_TAG + `cache miss on ${req.url}, using live network`);
  return fetch(req);
}

async function installAppVersionIntoCache(version: string) {
  const manifestUrl = `${version}/manifest.json`;
  try {
    console.log(LOG_TAG + `Starting installation of ${manifestUrl}`);
    await caches.delete(CACHE_NAME);
    const resp = await fetchWithTimeout(manifestUrl, INSTALL_TIMEOUT_MS);
    const manifest = await resp.json();
    const manifestResources = manifest['resources'];
    if (!manifestResources || !(manifestResources instanceof Object)) {
      throw new Error(`Invalid manifest ${manifestUrl} : ${manifest}`);
    }

    const cache = await caches.open(CACHE_NAME);
    const urlsToCache: RequestInfo[] = [];

    // We use cache:reload to make sure that the index is always current and we
    // don't end up in some cycle where we keep re-caching the index coming from
    // the service worker itself.
    urlsToCache.push(new Request('/', {cache: 'reload', mode: 'same-origin'}));

    for (const [resource, integrity] of Object.entries(manifestResources)) {
      // We use cache: no-cache rather then reload here because the versioned
      // sub-resources are expected to be immutable and should never be
      // ambiguous. A revalidation request is enough.
      const reqOpts: RequestInit = {
        cache: 'no-cache',
        mode: 'same-origin',
        integrity: `${integrity}`,
      };
      urlsToCache.push(new Request(`${version}/${resource}`, reqOpts));
    }
    await cache.addAll(urlsToCache);
    console.log(LOG_TAG + 'installation completed for ' + version);
  } catch (err) {
    console.error(LOG_TAG + `Installation failed for ${manifestUrl}`, err);
    await caches.delete(CACHE_NAME);
    throw err;
  }
}

function fetchWithTimeout(req: Request|string, timeoutMs: number) {
  const url = (req as {url?: string}).url || `${req}`;
  return new Promise<Response>((resolve, reject) => {
    const timerId = setTimeout(() => {
      reject(new Error(`Timed out while fetching ${url}`));
    }, timeoutMs);
    fetch(req).then((resp) => {
      clearTimeout(timerId);
      if (resp.ok) {
        resolve(resp);
      } else {
        reject(new Error(
            `Fetch failed for ${url}: ${resp.status} ${resp.statusText}`));
      }
    }, reject);
  });
}
