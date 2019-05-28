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

#include <string>
#include <deque>
#include "cerata/ipxact/xml.h"
#include "tinyxml2.h"

namespace cerata::ipxact {

/// @brief This group of elements identifies a top level item (e.g. a
/// component or a bus definition)  with vendor, library, name and a
/// version number.
struct VersionedIdentifier : ToXMLGroup {
  /// @brief Constructs a VersionedIdentifier with default vendor, library and
  /// version.
  VersionedIdentifier(std::string name)
      : vendor("fletcher"), library("fletcher"), name(name), version("0.1.0"){};
  /// @brief Constructs a VersionedIdentifier with provided vendor, library,
  /// name and version.
  VersionedIdentifier(std::string vendor, std::string library, std::string name,
                      std::string version)
      : vendor(vendor), library(library), name(name), version(version){};

  /// @brief Name of the vendor who supplies this file.
  std::string vendor;
  /// @brief Name of the logical library this element belongs to.
  std::string library;
  /// @brief The name of the object.
  std::string name;
  /// @brief Indicates the version of the named element.
  std::string version;

  /// @brief Implements ToXMLGroup
  std::deque<tinyxml2::XMLElement*> ToXML(tinyxml2::XMLNode &node);
};

}  // namespace cerata::ipxact
