// Copyright 2018 Delft University of Technology
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

#include <gtest/gtest.h>

#include "tinyxml2.h"

#include "cerata/nodes.h"
#include "cerata/types.h"

#include "cerata/ipxact/component.h"
#include "cerata/ipxact/identifier.h"
#include "cerata/ipxact/ipxact.h"

#include "../test_designs.h"

namespace cerata {

TEST(IPXACT, VersionedIdentifier) {
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLPrinter printer;

    auto identifier = cerata::ipxact::VersionedIdentifier("some-name");
    for (const auto &e : identifier.ToXML(doc)) {
      doc.InsertEndChild(e);
    }

    doc.Print(&printer);
    ASSERT_STREQ(
        printer.CStr(),
        "<ipxact:vendor>fletcher</ipxact:vendor>\n\n<ipxact:library>fletcher</"
        "ipxact:library>\n\n<ipxact:name>some-name</"
        "ipxact:name>\n\n<ipxact:version>0.1.0</ipxact:version>\n");
  }
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLPrinter printer;

    auto identifier = cerata::ipxact::VersionedIdentifier("vendor", "library",
                                                          "name", "version");
    for (const auto &e : identifier.ToXML(doc)) {
      doc.InsertEndChild(e);
    }
    
    doc.Print(&printer);
    ASSERT_STREQ(
        printer.CStr(),
        "<ipxact:vendor>vendor</ipxact:vendor>\n\n<ipxact:library>library</"
        "ipxact:library>\n\n<ipxact:name>name</"
        "ipxact:name>\n\n<ipxact:version>version</ipxact:version>\n");
  }
}

TEST(IPXACT, EmptyComponent) {
  auto component =
      cerata::ipxact::Component(Component::Make("empty-component"));
  tinyxml2::XMLPrinter printer;
  component.ToXML()->Print(&printer);

  ASSERT_STREQ(
      printer.CStr(),
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<ipxact:component "
      "xmlns:ipxact=\"http://www.accellera.org/XMLSchema/IPXACT/1685-2014\" "
      "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
      "xsi:schemaLocation=\"http://www.accellera.org/XMLSchema/IPXACT/"
      "1685-2014 "
      "http://www.accellera.org/XMLSchema/IPXACT/1685-2014/index.xsd\">\n    "
      "<ipxact:vendor>fletcher</ipxact:vendor>\n    "
      "<ipxact:library>fletcher</ipxact:library>\n    "
      "<ipxact:name>empty-component</ipxact:name>\n    "
      "<ipxact:version>0.1.0</ipxact:version>\n</ipxact:component>\n");
}

// TEST(IPXACT, Example) {
//   auto top = GetAllPortTypesComponent();
//   std::string output_dir = ".";
//   std::deque<OutputGenerator::OutputSpec> output_spec;
//   output_spec.push_back(OutputGenerator::OutputSpec(top));
//   cerata::ipxact::IPXACTOutputGenerator(output_dir, output_spec).Generate();
// }

}  // namespace cerata
