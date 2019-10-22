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
#include <memory>

#include "cerata/api.h"
#include "cerata/test_utils.h"

namespace cerata {

TEST(Instances, NodeMap) {
  auto par = parameter("par", integer(), intl(8));
  auto sig = signal("sig", vector(par));
  auto lit = strl("str");
  auto exp = par * 2;
  auto prt = port("prt", vector(exp));

  auto comp = component("test", {par, sig, lit, exp, prt});

  auto top = component("top");
  auto inst = top->Instantiate(comp, "inst");

  auto map = inst->comp_to_inst_map();

  // Ports and parameters should be on the instance graph.
  ASSERT_EQ(map[par.get()], inst->par("par"));
  ASSERT_EQ(map[prt.get()], inst->prt("prt"));

  // The port should be parameterized with an expressions that uses to the instance parameter,
  // not the component parameter.
  inst->prt("prt")->type()->GetGenerics();

  // Signals, expressions and literals shouldn't be on the instance graph.
  ASSERT_EQ(map.count(sig.get()), 0);
  ASSERT_EQ(map.count(exp.get()), 0);
  ASSERT_EQ(map.count(lit.get()), 0);
}

}
