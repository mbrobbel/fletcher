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

#include <memory>
#include "cerata/graphs.h"
#include "cerata/ipxact/businterface.h"
#include "cerata/ipxact/identifier.h"
#include "cerata/ipxact/xml.h"
#include "tinyxml2.h"

namespace cerata::ipxact {

/// @brief This is the root element for all non platform-core components.
struct Component : ToXMLDocument {
  /// @brief Constructs a Component based on a Cerata Component.
  Component(std::shared_ptr<cerata::Component> component)
      : component(component),
        versionedIdentifier(VersionedIdentifier(component->name())){};

  /// @brief Shared pointer to the Cerata Component.
  std::shared_ptr<cerata::Component> component;

  /// @brief VersionedIdentifier for this Component.
  VersionedIdentifier versionedIdentifier;
  /// @brief BusInterfaces for this Component.
  std::optional<BusInterfaces> busInterfaces;

  /// @brief Implements ToXMLDocument
  std::unique_ptr<tinyxml2::XMLDocument> ToXML();
};

}  // namespace cerata::ipxact
