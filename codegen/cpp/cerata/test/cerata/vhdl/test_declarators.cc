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

#include "cerata/test_designs.h"

namespace cerata {

TEST(VHDL_DECL, Signal) {
  auto sig = signal("test", vector(8));
  auto code = vhdl::Decl::Generate(*sig).ToString();
  ASSERT_EQ(code, "signal test : std_logic_vector(7 downto 0);\n");
}

TEST(VHDL_DECL, SignalRecord) {
  auto sig = signal("test",
                    record({field("a", vector(8)),
                            field("b", bit())}));
  auto code = vhdl::Decl::Generate(*sig).ToString();
  ASSERT_EQ(code, "signal test_a : std_logic_vector(7 downto 0);\n"
                  "signal test_b : std_logic;\n");
}

TEST(VHDL_DECL, SignalArray) {
  auto size = intl(2);
  auto sig_array = signal_array("test", bit(), size);
  auto code = vhdl::Decl::Generate(*sig_array).ToString();
  ASSERT_EQ(code, "signal test : std_logic_vector(1 downto 0);\n");
}

TEST(VHDL_DECL, SignalRecordArray) {
  auto size = intl(2);
  auto sig_array = signal_array("test",
                                record({field("a", vector(8)),
                                        field("b", bit())}),
                                size);
  auto code = vhdl::Decl::Generate(*sig_array).ToString();
  ASSERT_EQ(code, "signal test_a : std_logic_vector(15 downto 0);\n"
                  "signal test_b : std_logic_vector(1 downto 0);\n");
}

TEST(VHDL_DECL, SignalRecordArrayParam) {
  auto size = parameter("SIZE", integer());
  auto sig_array = signal_array("test",
                                record({field("a", vector(8)),
                                        field("b", bit())}),
                                size);
  auto code = vhdl::Decl::Generate(*sig_array).ToString();
  ASSERT_EQ(code, "signal test_a : std_logic_vector(SIZE*8-1 downto 0);\n"
                  "signal test_b : std_logic_vector(SIZE-1 downto 0);\n");
}

TEST(VHDL_DECL, SignalRecordParamArrayParam) {
  auto size = parameter("SIZE", integer());
  auto width = parameter("WIDTH", integer());
  auto sig_array = signal_array("test",
                                record({field("a", vector(width)),
                                        field("b", bit())}),
                                size);
  auto code = vhdl::Decl::Generate(*sig_array).ToString();
  ASSERT_EQ(code, "signal test_a : std_logic_vector(SIZE*WIDTH-1 downto 0);\n"
                  "signal test_b : std_logic_vector(SIZE-1 downto 0);\n");
}

TEST(VHDL_DECL, Component) {
  default_component_pool()->Clear();
  auto code = vhdl::Decl::Generate(*GetAllPortTypesComponent());
  std::cout << code.ToString() << std::endl;
}

TEST(VHDL_DECL, ArrayPort) {
  default_component_pool()->Clear();
  auto top = GetArrayComponent();
  auto design = vhdl::Design(top);
  std::cout << design.Generate().ToString() << std::endl;
}

}  // namespace cerata
