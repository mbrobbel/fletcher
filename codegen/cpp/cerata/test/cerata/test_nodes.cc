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

#include <gmock/gmock.h>

#include <string>
#include <fstream>
#include <cerata/api.h>

namespace cerata {

TEST(Nodes, ParamReferences) {
  auto lit = strl("foo");
  auto a = parameter("a", string(), lit);
  auto b = parameter("b", string(), a);
  auto c = parameter("c", string(), b);
  auto expr = c * 2;
  auto d = parameter("d", string(), expr);

  std::vector<Object *> trace;
  d->AppendReferences(&trace);

  ASSERT_EQ(trace[0], expr.get());
  ASSERT_EQ(trace[1], c.get());
  ASSERT_EQ(trace[2], b.get());
  ASSERT_EQ(trace[3], a.get());
  ASSERT_EQ(trace[4], lit.get());
  ASSERT_EQ(trace[5], rintl(2));

}

}  // namespace cerata
