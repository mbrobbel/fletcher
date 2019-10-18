// Copyright 2018-2019 Delft University of Technology
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <memory>
#include <cerata/api.h>

#include "fletcher/test_schemas.h"
#include "fletchgen/schema.h"
#include "fletchgen/recordbatch.h"
#include "fletchgen/test_utils.h"

namespace fletchgen {

static void TestRecordBatchReader(const std::shared_ptr<arrow::Schema> &schema) {
  cerata::default_component_pool()->Clear();
  auto fs = FletcherSchema::Make(schema);
  fletcher::RecordBatchDescription rbd;
  fletcher::SchemaAnalyzer sa(&rbd);
  sa.Analyze(*schema);
  auto rbr = record_batch("Test_" + fs->name(), fs, rbd);
  GenerateDebugOutput(rbr);
}

TEST(RecordBatch, StringRead) {
  TestRecordBatchReader(fletcher::GetStringReadSchema());
}

TEST(RecordBatch, NullablePrimRead) {
  TestRecordBatchReader(fletcher::GetNullablePrimReadSchema());
}

TEST(RecordBatch, TwoPrimRead) {
  TestRecordBatchReader(fletcher::GetTwoPrimReadSchema());
}

}  // namespace fletchgen
