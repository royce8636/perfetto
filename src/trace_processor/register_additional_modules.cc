/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/trace_processor/register_additional_modules.h"
#include "src/trace_processor/importers/proto/android_probes_module.h"
#include "src/trace_processor/importers/proto/graphics_event_module.h"
#include "src/trace_processor/importers/proto/heap_graph_module.h"
#include "src/trace_processor/importers/proto/system_probes_module.h"

namespace perfetto {
namespace trace_processor {

void RegisterAdditionalModules(TraceProcessorContext* context) {
  context->modules.emplace_back(new AndroidProbesModule(context));
  context->modules.emplace_back(new GraphicsEventModule(context));
  context->modules.emplace_back(new HeapGraphModule(context));
  context->modules.emplace_back(new SystemProbesModule(context));
}

}  // namespace trace_processor
}  // namespace perfetto
