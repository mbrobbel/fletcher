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

TEST(Types, Flatten) {
  auto a = bit();
  auto b = vector<8>();
  auto c = stream(b);

  auto d = record("inner", {
      field("a", a),
      field("b", b),
      field("c", c)});

  auto e = stream(c);

  auto f = record("outer", {
      field("d", d),
      field("e", e)});

  auto flat = Flatten(f.get());

  ASSERT_EQ(flat[0].type_, f.get());
  ASSERT_EQ(flat[1].type_, d.get());
  ASSERT_EQ(flat[2].type_, a.get());
  ASSERT_EQ(flat[3].type_, b.get());
  ASSERT_EQ(flat[4].type_, c.get());
  ASSERT_EQ(flat[5].type_, b.get());
  ASSERT_EQ(flat[6].type_, e.get());
  ASSERT_EQ(flat[7].type_, c.get());
  ASSERT_EQ(flat[8].type_, b.get());

  ASSERT_EQ(flat[0].name(NamePart("x"), "_"), "x");
  ASSERT_EQ(flat[1].name(NamePart("x"), "_"), "x_d");
  ASSERT_EQ(flat[2].name(NamePart("x"), "_"), "x_d_a");
  ASSERT_EQ(flat[3].name(NamePart("x"), "_"), "x_d_b");
  ASSERT_EQ(flat[4].name(NamePart("x"), "_"), "x_d_c");
  ASSERT_EQ(flat[5].name(NamePart("x"), "_"), "x_d_c");
  ASSERT_EQ(flat[6].name(NamePart("x"), "_"), "x_e");
  ASSERT_EQ(flat[7].name(NamePart("x"), "_"), "x_e");
  ASSERT_EQ(flat[8].name(NamePart("x"), "_"), "x_e");
}

TEST(Types, TypeMapper) {
  auto a = bit();
  auto b = vector<8>();
  auto c = record("rec_K", {field("a", a),
                                  field("b", b)});
  auto d = stream(c);

  auto q = bit();
  auto r = vector<8>();
  auto s = record("rec_L", {field("q", q),
                                  field("r0", r),
                                  field("r1", stream(r))});
  auto t = stream(s);

  TypeMapper conv(t.get(), d.get());

  // TODO(johanpel): make type converter mapping less obscure
  conv.Add(0, 0)
      .Add(2, 2)
      .Add(3, 3)
      .Add(4, 0)
      .Add(5, 3);

  auto ump = conv.GetUniqueMappingPairs();

  // TODO(johanpel): come up with some test.

  std::cout << conv.ToString();

}

}  // namespace cerata
