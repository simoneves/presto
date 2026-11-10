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

#include <gtest/gtest.h>

#include "velox/common/base/tests/GTestUtils.h"
#include "velox/common/memory/Memory.h"
#include "velox/exec/tests/utils/PlanBuilder.h"

using namespace facebook::velox;

namespace facebook::presto {
namespace {

class PlanTransformersTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    memory::MemoryManager::testingSetInstance(memory::MemoryManager::Options{});
  }

  void SetUp() override {
    clearPlanTransformers();
    queryCtx_ = core::QueryCtx::create();
    fragment_.planNode =
        exec::test::PlanBuilder().tableScan(ROW({"c0"}, {BIGINT()})).planNode();
  }

  void TearDown() override {
    clearPlanTransformers();
  }

  // A transformer that puts a LimitNode named 'id' on top of the plan and
  // records that it ran.
  PlanTransformer addLimit(std::string id, std::vector<std::string>& calls) {
    return [this, id, &calls](
               core::PlanFragment fragment, const core::QueryCtx& queryCtx) {
      EXPECT_EQ(&queryCtx, queryCtx_.get());
      calls.push_back(id);
      fragment.planNode = std::make_shared<core::LimitNode>(
          id, 0, 10, false, fragment.planNode);
      return fragment;
    };
  }

  std::shared_ptr<core::QueryCtx> queryCtx_;
  core::PlanFragment fragment_;
};

TEST_F(PlanTransformersTest, noTransformers) {
  const auto root = fragment_.planNode;
  EXPECT_EQ(applyPlanTransformers(fragment_, *queryCtx_).planNode, root);
}

TEST_F(PlanTransformersTest, registrationOrder) {
  std::vector<std::string> calls;
  registerPlanTransformer(addLimit("first", calls));
  registerPlanTransformer(addLimit("second", calls));
  const auto scan = fragment_.planNode;

  const auto result = applyPlanTransformers(fragment_, *queryCtx_);

  EXPECT_EQ(calls, (std::vector<std::string>{"first", "second"}));
  EXPECT_EQ(result.planNode->id(), "second");
  ASSERT_EQ(result.planNode->sources().size(), 1);
  EXPECT_EQ(result.planNode->sources()[0]->id(), "first");
  EXPECT_EQ(result.planNode->sources()[0]->sources()[0], scan);
}

TEST_F(PlanTransformersTest, exceptionFailsTheConversion) {
  registerPlanTransformer(
      [](core::PlanFragment, const core::QueryCtx&) -> core::PlanFragment {
        VELOX_USER_FAIL("unsupported plan shape");
      });
  VELOX_ASSERT_USER_THROW(
      applyPlanTransformers(fragment_, *queryCtx_), "unsupported plan shape");
}

TEST_F(PlanTransformersTest, emptyResult) {
  registerPlanTransformer(
      [](core::PlanFragment fragment, const core::QueryCtx&) {
        fragment.planNode = nullptr;
        return fragment;
      });
  VELOX_ASSERT_THROW(
      applyPlanTransformers(fragment_, *queryCtx_),
      "A plan transformer returned no plan");
}

TEST_F(PlanTransformersTest, emptyTransformer) {
  VELOX_ASSERT_THROW(
      registerPlanTransformer(nullptr), "A plan transformer must be callable");
}

TEST_F(PlanTransformersTest, clear) {
  std::vector<std::string> calls;
  registerPlanTransformer(addLimit("first", calls));
  clearPlanTransformers();
  const auto root = fragment_.planNode;
  EXPECT_EQ(applyPlanTransformers(fragment_, *queryCtx_).planNode, root);
  EXPECT_TRUE(calls.empty());
}

} // namespace
} // namespace facebook::presto
