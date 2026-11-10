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
#include "presto_cpp/main/PlanTransformers.h"

#include <folly/Synchronized.h>

#include "velox/common/base/Exceptions.h"

#include <vector>

namespace facebook::presto {
namespace {
folly::Synchronized<std::vector<PlanTransformer>>& transformers() {
  static folly::Synchronized<std::vector<PlanTransformer>> instance;
  return instance;
}
} // namespace

void registerPlanTransformer(PlanTransformer transformer) {
  VELOX_CHECK_NOT_NULL(transformer, "A plan transformer must be callable");
  transformers().wlock()->push_back(std::move(transformer));
}

velox::core::PlanFragment applyPlanTransformers(
    velox::core::PlanFragment fragment,
    const velox::core::QueryCtx& queryCtx) {
  // A copy, so a transformer runs without the lock held.
  const auto registered = transformers().copy();
  for (const auto& transformer : registered) {
    fragment = transformer(std::move(fragment), queryCtx);
    VELOX_CHECK_NOT_NULL(
        fragment.planNode, "A plan transformer returned no plan");
  }
  return fragment;
}

void clearPlanTransformers() {
  transformers().wlock()->clear();
}

} // namespace facebook::presto
