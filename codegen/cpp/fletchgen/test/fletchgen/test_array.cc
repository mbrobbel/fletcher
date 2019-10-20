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
#include <cerata/api.h>
#include <memory>
#include <string>

#include "fletcher/common.h"
#include "fletchgen/array.h"
#include "fletchgen/test_utils.h"

namespace fletchgen {

// TODO(johanpel): load component decls from file and check for equivalence.

TEST(Kernel, ArrayReader) {
  auto top = array(fletcher::Mode::READ);
  auto generated = GenerateDecl(top);
}

TEST(Kernel, ArrayWriter) {
  auto top = array(fletcher::Mode::WRITE);
  GenerateDecl(top);
}

}  // namespace fletchgen
