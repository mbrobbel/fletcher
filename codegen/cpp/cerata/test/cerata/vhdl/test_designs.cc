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
#include <cerata/api.h>
#include <string>

#include "cerata/test_utils.h"
#include "cerata/test_designs.h"
#include "cerata/type.h"

namespace cerata {

TEST(VHDL_DESIGN, Simple) {
  default_component_pool()->Clear();

  auto static_vec = vector<8>();
  auto param = parameter("vec_width", integer(), intl(8));
  auto param_vec = vector("param_vec_type", param);
  auto veca = port("static_vec", static_vec);
  auto vecb = port("param_vec", param_vec);
  auto comp = component("simple", {param, veca, vecb});

  auto generated = GenerateDebugOutput(comp);

  auto expected =
      "library ieee;\n"
      "use ieee.std_logic_1164.all;\n"
      "use ieee.numeric_std.all;\n"
      "\n"
      "entity simple is\n"
      "  generic (\n"
      "    VEC_WIDTH : integer := 8\n"
      "  );\n"
      "  port (\n"
      "    static_vec : in std_logic_vector(7 downto 0);\n"
      "    param_vec  : in std_logic_vector(vec_width-1 downto 0)\n"
      "  );\n"
      "end entity;\n"
      "\n"
      "architecture Implementation of simple is\n"
      "begin\n"
      "end architecture;\n";

  ASSERT_EQ(generated, expected);
}

TEST(VHDL_DESIGN, CompInst) {
  default_component_pool()->Clear();

  auto a = port("a", bit(), Port::Dir::IN);
  auto b = port("b", bit(), Port::Dir::OUT);
  auto ca = component("comp_a", {a});
  auto cb = component("comp_b", {b});
  auto top = component("top");
  auto ia = top->AddInstanceOf(ca.get());
  auto ib = top->AddInstanceOf(cb.get());
  Connect(ia->prt("a"), ib->prt("b"));

  auto generated = GenerateDebugOutput(top);

  auto expected =
      "library ieee;\n"
      "use ieee.std_logic_1164.all;\n"
      "use ieee.numeric_std.all;\n"
      "\n"
      "entity top is\n"
      "end entity;\n"
      "\n"
      "architecture Implementation of top is\n"
      "  component comp_a is\n"
      "    port (\n"
      "      a : in std_logic\n"
      "    );\n"
      "  end component;\n"
      "\n"
      "  component comp_b is\n"
      "    port (\n"
      "      b : out std_logic\n"
      "    );\n"
      "  end component;\n"
      "\n"
      "  signal comp_a_inst_a : std_logic;\n"
      "  signal comp_b_inst_b : std_logic;\n"
      "\n"
      "begin\n"
      "  comp_a_inst_a <= comp_b_inst_b;\n"
      "\n"
      "  comp_a_inst : comp_a\n"
      "    port map (\n"
      "      a => comp_a_inst_a\n"
      "    );\n"
      "\n"
      "  comp_b_inst : comp_b\n"
      "    port map (\n"
      "      b => comp_b_inst_b\n"
      "    );\n"
      "\n"
      "end architecture;\n";

  ASSERT_EQ(generated, expected);
}

}  // namespace cerata

