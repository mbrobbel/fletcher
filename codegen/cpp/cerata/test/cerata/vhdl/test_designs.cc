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
  auto ia = top->Instantiate(ca.get());
  auto ib = top->Instantiate(cb.get());
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

TEST(VHDL_DESIGN, Streams) {
  default_component_pool()->Clear();

  auto a = port("a",
                stream(
                    record(
                        {field("q", bit()),
                         field("r", vector(8))})),
                Port::Dir::IN);

  auto b = port("b",
                stream(
                    record(
                        {field("s", bit()),
                         field("t", vector(8))})),
                Port::Dir::OUT);

  // A and B can be implicitly mapped.

  auto x = component("x", {a});
  auto y = component("y", {b});
  auto top = component("top");
  auto ix = top->Instantiate(x);
  auto iy = top->Instantiate(y);
  Connect(ix->prt("a"), iy->prt("b"));

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
      "  component x is\n"
      "    port (\n"
      "      a_valid : in  std_logic;\n"
      "      a_ready : out std_logic;\n"
      "      a_q     : in  std_logic;\n"
      "      a_r     : in  std_logic_vector(7 downto 0)\n"
      "    );\n"
      "  end component;\n"
      "\n"
      "  component y is\n"
      "    port (\n"
      "      b_valid : out std_logic;\n"
      "      b_ready : in  std_logic;\n"
      "      b_s     : out std_logic;\n"
      "      b_t     : out std_logic_vector(7 downto 0)\n"
      "    );\n"
      "  end component;\n"
      "\n"
      "  signal x_inst_a_valid : std_logic;\n"
      "  signal x_inst_a_ready : std_logic;\n"
      "  signal x_inst_a_q     : std_logic;\n"
      "  signal x_inst_a_r     : std_logic_vector(7 downto 0);\n"
      "\n"
      "  signal y_inst_b_valid : std_logic;\n"
      "  signal y_inst_b_ready : std_logic;\n"
      "  signal y_inst_b_s     : std_logic;\n"
      "  signal y_inst_b_t     : std_logic_vector(7 downto 0);\n"
      "\n"
      "begin\n"
      "  x_inst_a_valid <= y_inst_b_valid;\n"
      "  y_inst_b_ready <= x_inst_a_ready;\n"
      "  x_inst_a_q     <= y_inst_b_s;\n"
      "  x_inst_a_r     <= y_inst_b_t;\n"
      "\n"
      "  x_inst : x\n"
      "    port map (\n"
      "      a_valid => x_inst_a_valid,\n"
      "      a_ready => x_inst_a_ready,\n"
      "      a_q     => x_inst_a_q,\n"
      "      a_r     => x_inst_a_r\n"
      "    );\n"
      "\n"
      "  y_inst : y\n"
      "    port map (\n"
      "      b_valid => y_inst_b_valid,\n"
      "      b_ready => y_inst_b_ready,\n"
      "      b_s     => y_inst_b_s,\n"
      "      b_t     => y_inst_b_t\n"
      "    );\n"
      "\n"
      "end architecture;\n";

  ASSERT_EQ(generated, expected);
}

}  // namespace cerata

