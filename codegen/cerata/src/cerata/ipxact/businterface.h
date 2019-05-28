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

#include <deque>
#include <string>
#include "cerata/ipxact/xml.h"
#include "tinyxml2.h"

namespace cerata::ipxact {

/// @brief Describes one of the bus interfaces supported by this component.
struct BusInterface : ToXMLElement {

  NameGroup nameGroup;

  std::optional<bool> isPresent;

  /// @brief The bus type of this interface. Refers to bus definition using
  /// vendor, library, name, version attributes along with any configurable 
  /// element values needed to configure this interface.
  BusType busType;

  std::optional<AbstractionTypes> abstractionTypes;

  InterfaceMode interfaceMode;

  /// @brief Indicates whether a connection to this interface is required for 
  /// proper component functionality.
  std::optional<bool> connectionRequired;

  std::optional<Parameters> parameters;

  tinyxml2::XMLElement* ToXML(tinyxml2::XMLNode& node);
};

/// @brief A list of bus interfaces supported by this component.
struct BusInterfaces : ToXMLGroup {
  BusInterfaces()

  std::deque<BusInterface> busInterfaces;
  std::deque<tinyxml2::XMLElement*> ToXML(tinyxml2::XMLNode& node);
};

/// @brief Describes one of the bus interfaces supported by this component.
}  // namespace cerata::ipxact
