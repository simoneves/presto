/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <functional>

#include "velox/core/PlanFragment.h"
#include "velox/core/QueryCtx.h"

namespace facebook::presto {

/// Rewrites the Velox plan fragment of a task. A transformer gets the fragment
/// converted from the coordinator's plan and the task's query context, and
/// returns the fragment the task runs: the same fragment, or a new one. It
/// must keep the fragment's output and its node ids that splits, exchanges and
/// stats refer to. An exception fails the task with its message.
using PlanTransformer = std::function<velox::core::PlanFragment(
    velox::core::PlanFragment,
    const velox::core::QueryCtx&)>;

/// Adds a transformer for every task the worker creates from now on.
/// Transformers run in registration order. Meant for plugins, which the worker
/// loads at startup from 'plugin.dir'.
void registerPlanTransformer(PlanTransformer transformer);

/// Runs the registered transformers on 'fragment', in registration order. With
/// none registered it returns 'fragment' unchanged.
velox::core::PlanFragment applyPlanTransformers(
    velox::core::PlanFragment fragment,
    const velox::core::QueryCtx& queryCtx);

/// Removes all registered transformers. For tests.
void clearPlanTransformers();

} // namespace facebook::presto
