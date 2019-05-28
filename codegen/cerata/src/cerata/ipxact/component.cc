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

#include "cerata/ipxact/component.h"

namespace cerata::ipxact {

std::unique_ptr<tinyxml2::XMLDocument> Component::ToXML() {
  auto doc = std::make_unique<tinyxml2::XMLDocument>();
  doc->InsertEndChild(doc->NewDeclaration());

  // Create component node
  auto component = doc->NewElement("ipxact:component");
  doc->InsertEndChild(component);

  // Set component attributes
  component->SetAttribute(
      "xmlns:ipxact", "http://www.accellera.org/XMLSchema/IPXACT/1685-2014");
  component->SetAttribute("xmlns:xsi",
                          "http://www.w3.org/2001/XMLSchema-instance");
  component->SetAttribute(
      "xsi:schemaLocation",
      "http://www.accellera.org/XMLSchema/IPXACT/1685-2014 "
      "http://www.accellera.org/XMLSchema/IPXACT/1685-2014/index.xsd");

  // Add VersionedIdentifier
  for (const auto &e : this->versionedIdentifier.ToXML(*component)) {
    component->InsertEndChild(e);
  }

  return doc;
}

}  // namespace cerata::ipxact
