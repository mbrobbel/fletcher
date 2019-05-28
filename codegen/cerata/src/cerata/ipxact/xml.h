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
#include "tinyxml2.h"

namespace cerata::ipxact {

/// @brief Abstract class indicating implementor outputs an XMLElement.
class ToXMLElement {
 public:
  /// @brief Returns an XMLNode.
  virtual tinyxml2::XMLElement* ToXML(tinyxml2::XMLNode& node) = 0;
};

/// @brief Abstract class indicating implementor outputs a group of
/// XMLElements.
class ToXMLGroup {
 public:
  /// @brief Returns the corresponding IPXACT XML elements.
  virtual std::deque<tinyxml2::XMLElement*> ToXML(tinyxml2::XMLNode& node) = 0;
};

/// @brief Abstract class indicating implementor outputs an XMLDocument.
class ToXMLDocument {
 public:
  /// @brief Returns the corresponding IPXACT XML elements.
  virtual std::unique_ptr<tinyxml2::XMLDocument> ToXML() = 0;
};

}  // namespace cerata::ipxact
