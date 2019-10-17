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

#pragma once

#include <cerata/api.h>

#include <vector>
#include <memory>
#include <utility>
#include <string>

namespace cerata {

std::shared_ptr<Component> GetTypeExpansionComponent() {
  auto width = parameter("width", integer(), intl(8));
  auto vec_type = vector("data", width);
  auto rec_type = record("rec_type", {
      field("cerata", vec_type),
      field("is", vec_type),
      field("awesome", vec_type)
  });
  auto stream_type = stream("stream_type", rec_type);
  auto data_in = port("data", stream_type, Port::Dir::IN);
  auto data_out = port("data", stream_type, Port::Dir::OUT);
  auto foo = component("foo", {width, data_in});
  auto bar = component("bar", {width, data_out});
  auto top = component("top");
  auto foo_inst = top->AddInstanceOf(foo.get(), "foo");
  auto bar_inst = top->AddInstanceOf(bar.get(), "bar");
  Connect(foo_inst->prt("data"), bar_inst->prt("data"));
  return top;
}

std::shared_ptr<Component> GetArrayToArrayInternalComponent(bool invert = false) {
  auto data = vector<8>();

  std::string a = !invert ? "src" : "dst";
  std::string x = !invert ? "dst0" : "src0";
  std::string y = !invert ? "dst1" : "src1";

  auto top_comp = component("top_comp");

  auto a_size = parameter("size", integer(), intl(0));
  auto a_array = port_array("array", data, a_size, invert ? Term::IN : Term::OUT);
  auto a_comp = component(a, {a_size, a_array});
  auto a_inst = instance(a_comp.get());

  auto x_size = parameter("size", integer(), intl(0));
  auto x_array = port_array("array", data, x_size, invert ? Term::OUT : Term::IN);
  auto x_comp = component(x, {x_size, x_array});
  auto x_inst = instance(x_comp.get());

  auto y_size = parameter("size", integer(), intl(0));
  auto y_array = port_array("array", data, y_size, invert ? Term::OUT : Term::IN);
  auto y_comp = component(y, {y_size, y_array});
  auto y_inst = instance(y_comp.get());

  a_inst->prta("array")->Append();
  a_inst->prta("array")->Append();
  a_inst->prta("array")->Append();
  a_inst->prta("array")->Append();

  x_inst->prta("array")->Append();
  x_inst->prta("array")->Append();
  y_inst->prta("array")->Append();
  y_inst->prta("array")->Append();

  if (!invert) {
    Connect(x_inst->prta("array")->node(0), a_inst->prta("array")->node(1));
    Connect(x_inst->prta("array")->node(1), a_inst->prta("array")->node(0));
    Connect(y_inst->prta("array")->node(0), a_inst->prta("array")->node(3));
    Connect(y_inst->prta("array")->node(1), a_inst->prta("array")->node(2));
  } else {
    Connect(a_inst->prta("array")->node(1), x_inst->prta("array")->node(0));
    Connect(a_inst->prta("array")->node(0), x_inst->prta("array")->node(1));
    Connect(a_inst->prta("array")->node(3), y_inst->prta("array")->node(0));
    Connect(a_inst->prta("array")->node(2), y_inst->prta("array")->node(1));
  }

  top_comp->AddChild(std::move(a_inst));
  top_comp->AddChild(std::move(x_inst));
  top_comp->AddChild(std::move(y_inst));
  return top_comp;
}

std::shared_ptr<Component> GetArrayToArrayComponent(bool invert = false) {
  auto data = vector<8>();

  auto top_size = parameter("top_size", integer(), intl(0));
  auto top_array = port_array("top_array", data, top_size, invert ? Term::OUT : Term::IN);
  auto top_comp = component("top_comp", {top_size, top_array});

  auto child_size = parameter("child_size", integer(), intl(0));
  auto child_array = port_array("child_array", data, child_size, invert ? Term::OUT : Term::IN);
  auto child_comp = component("child_comp", {child_size, child_array});
  auto child_inst = instance(child_comp.get());

  if (invert) {
    child_inst->prta("child_array")->Append();
    top_array->Append();
    top_array->Append();
    Connect(top_array->node(0), child_inst->prta("child_array")->node(0));
    Connect(top_array->node(1), child_inst->prta("child_array")->node(0));
  } else {
    child_inst->prta("child_array")->Append();
    child_inst->prta("child_array")->Append();
    top_array->Append();
    Connect(child_inst->prta("child_array")->node(0), top_array->node(0));
    Connect(child_inst->prta("child_array")->node(1), top_array->node(0));
  }
  top_comp->AddChild(std::move(child_inst));
  return top_comp;
}

std::shared_ptr<Component> GetArrayComponent() {
  auto size = parameter("size", integer(), intl(0));
  auto data = vector<8>();
  auto pA = port_array("A", data, size, Term::OUT);
  auto pB = port("B", data, Term::IN);
  auto pC = port("C", data, Term::IN);

  auto top = component("top");
  auto x_comp = component("X", {size, pA});
  auto y_comp = component("Y", {pB, pC});

  auto x = instance(x_comp.get());
  auto y = instance(y_comp.get());

  Connect(y->prt("B"), x->prta("A")->Append());
  Connect(y->prt("C"), x->prta("A")->Append());

  top->AddChild(std::move(x))
      .AddChild(std::move(y));

  return top;
}

std::shared_ptr<Component> GetTypeConvComponent() {
  auto t_wide = vector<4>();
  auto t_narrow = vector<2>();
  // Flat indices:
  auto tA = record("rec_A", {  // 0
      field("q", t_wide),      // 1
      field("r", t_narrow),    // 2
      field("s", t_narrow),    // 3
      field("t", t_wide),      // 4
  });

  auto tB = record("rec_B", {  // 0
      field("u", t_wide),      // 1
      field("v", t_narrow),    // 2
      field("w", t_narrow),    // 3
      field("x", t_wide),      // 4
  });

  // Create a type mapping from tA to tE
  auto mapper = std::make_shared<TypeMapper>(tA.get(), tB.get());
  mapper->Add(0, 0);
  mapper->Add(1, 2).Add(1, 3);
  mapper->Add(3, 1);
  mapper->Add(2, 1);
  mapper->Add(4, 4);
  tA->AddMapper(mapper);

  // Ports
  auto pA = port("A", tA, Port::OUT);
  auto pB = port("B", tB, Port::IN);

  // Components and instantiations
  auto top = component("top");
  auto x_comp = component("X", {pA});
  auto y_comp = component("Y", {pB});
  auto x = instance(x_comp.get());
  auto y = instance(y_comp.get());
  auto xr = x.get();
  auto yr = y.get();
  top->AddChild(std::move(x))
      .AddChild(std::move(y));

  // Connect ports
  Connect(yr->prt("B"), xr->prt("A"));

  return top;
}

std::shared_ptr<Component> GetArrayTypeConvComponent() {
  auto t_wide = vector<4>();
  auto t_narrow = vector<2>();
  // Flat indices:
  auto tA = record("TA", {   // 0
      field("q", t_wide),    // 1
  });

  auto tB = record("TB", {   // 0
      field("r", t_narrow),  // 1
      field("s", t_narrow),  // 2
  });

  // Create a type mapping from tA to tE
  auto mapper = std::make_shared<TypeMapper>(tA.get(), tB.get());
  mapper->Add(1, 1);
  mapper->Add(1, 2);
  tA->AddMapper(mapper);

  // Ports
  auto parSize = parameter("ARRAY_SIZE", integer(), intl(0));
  auto pA = port_array("A", tA, parSize, Port::OUT);
  auto pB = port("B", tB, Port::OUT);
  auto pC = port("C", tB, Port::OUT);

  // Components and instantiations
  auto top = component("top", {pB, pC});
  auto x_comp = component("X", {parSize, pA});
  auto x = instance(x_comp.get());
  auto xr = x.get();
  top->AddChild(std::move(x));

  // Drive B and C from A
  pB <<= xr->prta("A")->Append();
  pC <<= xr->prta("A")->Append();

  return top;
}

std::shared_ptr<Component> GetStreamConcatComponent() {
  // Flat indices:
  auto tA = stream("split",                  // 0
                   record("a", {             // 1
                       field("other",
                             bit()),         // 2
                       field("child",
                             stream("se",    // 3
                                    bit()))  // 4
                   }));

  auto tB = stream("concat",  // 0
                   bit(),     // 1
                   "data");

  auto tC = stream("concat",  // 0
                   bit(),     // 1
                   "data");

  // Create a type mapping from tA to tB
  auto mapperB = std::make_shared<TypeMapper>(tA.get(), tB.get());
  mapperB->Add(0, 0);
  mapperB->Add(2, 1);
  mapperB->Add(3, 0);
  mapperB->Add(4, 1);
  tA->AddMapper(mapperB);

  // Create a type mapping from tA to tC
  auto mapperC = std::make_shared<TypeMapper>(tA.get(), tC.get());
  mapperC->Add(0, 0);
  mapperC->Add(2, 1);
  mapperC->Add(3, 0);
  mapperC->Add(4, 1);
  tA->AddMapper(mapperC);

  // Ports
  auto pA0 = port("A0", tA, Port::OUT);
  auto pA1 = port("A1", tA, Port::OUT);
  auto pB = port("B", tB, Port::OUT);
  auto pC = port("C", tC, Port::OUT);

  // Components and instantiations
  auto x_comp = component("X", {pA0, pA1});
  auto y_comp = component("Y", {pB, pC});
  y_comp->meta()[vhdl::meta::PRIMITIVE] = "true";
  y_comp->meta()[vhdl::meta::LIBRARY] = "test";
  y_comp->meta()[vhdl::meta::PACKAGE] = "test";
  auto y = instance(y_comp.get());
  auto yr = y.get();
  x_comp->AddChild(std::move(y));

  // Connect ports
  Connect(x_comp->prt("A0"), yr->prt("B"));
  Connect(x_comp->prt("A1"), yr->prt("C"));

  return x_comp;
}

std::shared_ptr<Component> GetAllPortTypesComponent() {
  auto r_type = record("rec", {
      field("a", vector<8>()),
      field("b", vector<32>())
  });
  auto s_type = stream("stream", vector<16>());

  auto clk_domain = ClockDomain::Make("domain0");
  auto clk_port = port("clk", bit(), Port::Dir::IN, clk_domain);
  auto rst_port = port("reset", bit(), Port::Dir::IN, clk_domain);
  auto b_port = port("some_bool", boolean(), Port::OUT);
  auto v_port = port("some_vector", vector<64>());
  auto r_port = port("some_record", r_type, Port::OUT);
  auto s_port = port("some_port", s_type);

  auto par = parameter("depth", integer(), intl(16));

  auto a_type = component("a", {par, clk_port, rst_port, b_port, v_port, r_port, s_port});

  return a_type;
}

std::shared_ptr<Component> GetExampleDesign() {
  auto vec_width = parameter("vec_width", integer(), intl(32));
  // Construct a deeply nested type to showcase Cerata's capabilities.
  auto my_type = record("my_record_type", {field("bit", bit()),
                                           field("vec", vector("param_vec", vec_width)),
                                           field("stream",
                                                 stream("d", record("other_rec_type", {
                                                     field("substream",
                                                           stream(vector<32>())),
                                                     field("int", integer())})))});

  // Construct two components with a port made from these types
  auto my_array_size = parameter("array_size", integer());
  auto my_comp = component("my_comp", {vec_width,
                                       port_array("my_array", my_type, my_array_size, Port::OUT)});

  auto my_other_comp = component("my_other_comp", {vec_width,
                                                   port("my_port", my_type)});

  // Create a top level and add instances of each component
  auto my_top = component("my_top_level");
  auto my_inst = my_top->AddInstanceOf(my_comp.get());

  // Create a bunch of instances and connect to other component
  std::vector<Instance *> my_other_instances;
  for (int i = 0; i < 10; i++) {
    my_other_instances.push_back(my_top->AddInstanceOf(my_other_comp.get(), "my_inst_" + std::to_string(i)));
    Connect(my_other_instances[i]->prt("my_port"), my_inst->prta("my_array")->Append());
  }

  return my_top;
}

}  // namespace cerata
