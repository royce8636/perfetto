// Copyright (C) 2022 The Android Open Source Project
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

import {Disposable, Trash} from '../base/disposable';
import {assertFalse} from '../base/logging';
import {ViewerImpl, ViewerProxy} from '../common/viewer';
import {globals} from '../frontend/globals';
import {
  BasePlugin,
  Command,
  EngineProxy,
  MetricVisualisation,
  Plugin,
  PluginClass,
  PluginContext,
  PluginContextTrace,
  PluginDescriptor,
  StatefulPlugin,
  Store,
  Track,
  TrackContext,
  TrackDescriptor,
  TrackRef,
  Viewer,
} from '../public';

import {Engine} from './engine';
import {Registry} from './registry';

// Every plugin gets its own PluginContext. This is how we keep track
// what each plugin is doing and how we can blame issues on particular
// plugins.
// The PluginContext exists for the whole duration a plugin is active.
export class PluginContextImpl implements PluginContext, Disposable {
  private trash = new Trash();
  private alive = true;

  constructor(
      readonly pluginId: string, readonly viewer: ViewerProxy,
      private commandRegistry: Map<string, Command>) {
    this.trash.add(viewer);
  }

  addCommand(cmd: Command): void {
    // Silently ignore if context is dead.
    if (!this.alive) return;

    const {id} = cmd;
    assertFalse(this.commandRegistry.has(id));
    this.commandRegistry.set(id, cmd);

    this.trash.add({
      dispose: () => {
        this.commandRegistry.delete(id);
      },
    });
  }

  dispose(): void {
    this.trash.dispose();
    this.alive = false;
  }
}

// This TracePluginContext implementation provides the plugin access to trace
// related resources, such as the engine and the store.
// The TracePluginContext exists for the whole duration a plugin is active AND a
// trace is loaded.
class TracePluginContextImpl<T> implements PluginContextTrace<T>, Disposable {
  private trash = new Trash();
  private alive = true;

  constructor(
      private ctx: PluginContext, readonly store: Store<T>,
      readonly engine: EngineProxy,
      readonly trackRegistry: Map<string, TrackDescriptor>,
      private defaultTracks: Set<TrackRef>,
      private commandRegistry: Map<string, Command>) {
    this.trash.add(engine);
    this.trash.add(store);
  }

  addCommand(cmd: Command): void {
    // Silently ignore if context is dead.
    if (!this.alive) return;

    const {id} = cmd;
    assertFalse(this.commandRegistry.has(id));
    this.commandRegistry.set(id, cmd);

    this.trash.add({
      dispose: () => {
        this.commandRegistry.delete(id);
      },
    });
  }

  get viewer(): Viewer {
    return this.ctx.viewer;
  }

  registerTrack(trackDesc: TrackDescriptor): void {
    // Silently ignore if context is dead.
    if (!this.alive) return;

    this.trackRegistry.set(trackDesc.uri, trackDesc);
    this.trash.addCallback(() => this.trackRegistry.delete(trackDesc.uri));
  }

  addDefaultTrack(track: TrackRef): void {
    // Silently ignore if context is dead.
    if (!this.alive) return;

    this.defaultTracks.add(track);
    this.trash.addCallback(() => this.defaultTracks.delete(track));
  }

  registerStaticTrack(track: TrackDescriptor&TrackRef): void {
    this.registerTrack(track);

    // TODO(stevegolton): Once we've sorted out track_decider, we should also
    // add this track to the default track list here. E.g.
    // this.addDefaultTrack({
    //   uri: trackDetails.uri,
    //   displayName: trackDetails.displayName,
    //   sortKey: PrimaryTrackSortKey.ORDINARY_TRACK,
    // });
  }

  dispose(): void {
    this.trash.dispose();
    this.alive = false;
  }
}

// 'Static' registry of all known plugins.
export class PluginRegistry extends Registry<PluginDescriptor<unknown>> {
  constructor() {
    super((info) => info.pluginId);
  }
}

interface PluginDetails<T> {
  plugin: Plugin<T>;
  context: PluginContext&Disposable;
  traceContext?: TracePluginContextImpl<unknown>;
}

function isPluginClass<T>(v: unknown): v is PluginClass<T> {
  return typeof v === 'function' && !!(v.prototype.onActivate);
}

function makePlugin<T>(info: PluginDescriptor<T>): Plugin<T> {
  const {plugin} = info;

  if (typeof plugin === 'function') {
    if (isPluginClass(plugin)) {
      const PluginClass = plugin;
      return new PluginClass();
    } else {
      return plugin();
    }
  } else {
    return plugin;
  }
}

export class PluginManager {
  private registry: PluginRegistry;
  private plugins: Map<string, PluginDetails<unknown>>;
  private engine?: Engine;
  readonly trackRegistry = new Map<string, TrackDescriptor>();
  readonly commandRegistry = new Map<string, Command>();
  readonly defaultTracks = new Set<TrackRef>();

  constructor(registry: PluginRegistry) {
    this.registry = registry;
    this.plugins = new Map();
  }

  activatePlugin(id: string, viewer: ViewerImpl): void {
    if (this.isActive(id)) {
      return;
    }

    const pluginInfo = this.registry.get(id);
    const plugin = makePlugin(pluginInfo);

    const viewerProxy = viewer.getProxy(id);
    const context =
        new PluginContextImpl(id, viewerProxy, this.commandRegistry);

    plugin.onActivate && plugin.onActivate(context);

    const pluginDetails: PluginDetails<unknown> = {
      plugin,
      context,
    };

    // If a trace is already loaded when plugin is activated, make sure to
    // call onTraceLoad().
    if (this.engine) {
      this.doPluginTraceLoad(pluginDetails, this.engine, id);
    }

    this.plugins.set(id, pluginDetails);
  }

  deactivatePlugin(id: string): void {
    const pluginDetails = this.getPluginContext(id);
    if (pluginDetails === undefined) {
      return;
    }
    const {context, plugin} = pluginDetails;

    maybeDoPluginTraceUnload(pluginDetails);

    plugin.onDeactivate && plugin.onDeactivate(context);
    context.dispose();

    this.plugins.delete(id);
  }

  isActive(pluginId: string): boolean {
    return this.getPluginContext(pluginId) !== undefined;
  }

  getPluginContext(pluginId: string): PluginDetails<unknown>|undefined {
    return this.plugins.get(pluginId);
  }

  findPotentialTracks(): TrackRef[] {
    return Array.from(this.defaultTracks);
  }

  onTraceLoad(engine: Engine): void {
    this.engine = engine;
    for (const [id, pluginDetails] of this.plugins) {
      this.doPluginTraceLoad(pluginDetails, engine, id);
    }
  }

  onTraceClose() {
    for (const pluginDetails of this.plugins.values()) {
      maybeDoPluginTraceUnload(pluginDetails);
    }
    this.engine = undefined;
  }

  commands(): Command[] {
    return Array.from(this.commandRegistry.values());
  }

  metricVisualisations(): MetricVisualisation[] {
    return Array.from(this.plugins.values()).flatMap((ctx) => {
      const tracePlugin = ctx.plugin;
      if (tracePlugin && tracePlugin.metricVisualisations) {
        return tracePlugin.metricVisualisations(ctx.context);
      } else {
        return [];
      }
    });
  }

  // Look up track into for a given track's URI.
  // Returns |undefined| if no track can be found.
  resolveTrackInfo(uri: string): TrackDescriptor|undefined {
    return this.trackRegistry.get(uri);
  }

  // Create a new plugin track object from its URI.
  // Returns undefined if no such track is registered.
  createTrack(uri: string, trackCtx: TrackContext): Track|undefined {
    const trackInfo = pluginManager.trackRegistry.get(uri);
    return trackInfo && trackInfo.track(trackCtx);
  }

  private doPluginTraceLoad<T>(
      pluginDetails: PluginDetails<T>, engine: Engine, pluginId: string): void {
    const {plugin, context} = pluginDetails;

    const engineProxy = engine.getProxy(pluginId);

    // Migrate state & write back to store.
    if (isStatefulPlugin(plugin)) {
      const initialState = globals.store.state.plugins[pluginId];
      const migratedState = plugin.migrate(initialState);
      globals.store.edit((draft) => {
        draft.plugins[pluginId] = migratedState;
      });

      const proxyStore = globals.store.createProxy<T>(['plugins', pluginId]);
      const traceCtx = new TracePluginContextImpl(
          context,
          proxyStore,
          engineProxy,
          this.trackRegistry,
          this.defaultTracks,
          this.commandRegistry);
      pluginDetails.traceContext = traceCtx;

      // TODO(stevegolton): Await onTraceLoad.
      plugin.onTraceLoad && plugin.onTraceLoad(traceCtx);
    } else {
      // Stateless plugin i.e. the plugin's state type is undefined.
      // Just provide a store proxy over this plugin's state, the plugin can
      // work the state out for itself if it wants to, but we're not going to
      // help it out by calling migrate().
      const proxyStore = globals.store.createProxy<T>(['plugins', pluginId]);
      const traceCtx = new TracePluginContextImpl(
          context,
          proxyStore,
          engineProxy,
          this.trackRegistry,
          this.defaultTracks,
          this.commandRegistry);
      pluginDetails.traceContext = traceCtx;

      // TODO(stevegolton): Await onTraceLoad.
      plugin.onTraceLoad && plugin.onTraceLoad(traceCtx);
    }
  }
}

function isStatefulPlugin<T>(plugin: BasePlugin<T>|
                             StatefulPlugin<T>): plugin is StatefulPlugin<T> {
  return 'migrate' in plugin;
}

function maybeDoPluginTraceUnload(pluginDetails: PluginDetails<unknown>): void {
  const {traceContext, plugin} = pluginDetails;

  if (traceContext) {
    // TODO(stevegolton): Await onTraceUnload.
    plugin.onTraceUnload && plugin.onTraceUnload(traceContext);
    traceContext.dispose();
    pluginDetails.traceContext = undefined;
  }
}


// TODO(hjd): Sort out the story for global singletons like these:
export const pluginRegistry = new PluginRegistry();
export const pluginManager = new PluginManager(pluginRegistry);
