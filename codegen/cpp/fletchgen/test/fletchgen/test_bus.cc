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

#include "fletchgen/bus.h"
#include "fletchgen/test_utils.h"

namespace fletchgen {

using cerata::intl;
using cerata::Instance;
using cerata::Port;

TEST(Bus, BusArbiter) {
  cerata::default_component_pool()->Clear();
  auto top = cerata::component("top");
  BusParam param(top);
  top->AddInstanceOf(bus_arbiter(BusFunction::READ));

  GenerateAll(top);
}

}  // namespace fletchgen
