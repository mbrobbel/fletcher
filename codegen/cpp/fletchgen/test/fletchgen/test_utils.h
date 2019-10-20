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
#include <fstream>
#include <string>
#include <utility>

namespace fletchgen {

inline std::string GenerateDecl(cerata::Component *comp, std::string name = "") {
  if (name.empty()) {
    name = comp->name();
  }
  auto src = cerata::vhdl::Decl::Generate(*comp, false).ToString();
  auto o = std::ofstream(name + ".comp.gen.vhd");
  o << src;
  o.close();

  std::cout << "VHDL SOURCE:\n";
  std::cout << src << std::endl;

  cerata::dot::Grapher dot;
  dot.style.config = cerata::dot::Config::all();
  dot.GenFile(*comp, name);

  return src;
}

inline std::string GenerateAll(cerata::Component *comp, std::string name = "") {
  if (name.empty()) {
    name = comp->name();
  }

  auto design = cerata::vhdl::Design(comp);
  auto src = design.Generate().ToString();
  auto o = std::ofstream(name + ".gen.vhd");
  o << src;
  o.close();

  std::cout << "VHDL SOURCE:\n";
  std::cout << src << std::endl;

  cerata::dot::Grapher dot;
  dot.style.config = cerata::dot::Config::all();
  dot.GenFile(*comp, name);

  return src;
}

inline std::string GenerateAll(const std::shared_ptr<cerata::Component>& comp, std::string name = "") {
  return GenerateAll(comp.get(), std::move(name));
}

}  // namespace fletchgen
