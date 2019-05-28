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

#include "cerata/ipxact/identifier.h"

namespace cerata::ipxact {

std::deque<tinyxml2::XMLElement *> VersionedIdentifier::ToXML(
    tinyxml2::XMLNode &node) {
  auto doc = node.GetDocument();

  auto vendor = doc->NewElement("ipxact:vendor");
  vendor->SetText(this->vendor.c_str());

  auto library = doc->NewElement("ipxact:library");
  library->SetText(this->library.c_str());

  auto name = doc->NewElement("ipxact:name");
  name->SetText(this->name.c_str());

  auto version = doc->NewElement("ipxact:version");
  version->SetText(this->version.c_str());

  return {vendor, library, name, version};
}

}  // namespace cerata::ipxact
