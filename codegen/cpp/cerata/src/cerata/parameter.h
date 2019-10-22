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

#include <vector>
#include <string>
#include <memory>

#include "cerata/node.h"

namespace cerata {
/**
 * @brief A Parameter node.
 *
 * Can be used as a type generic node or a static input to signal or port nodes.
 */
class Parameter : public MultiOutputNode {
 public:
  /// @brief Construct a new Parameter, optionally defining a default value Literal.
  Parameter(std::string name, const std::shared_ptr<Type> &type, std::shared_ptr<Node> value);
  /// @brief Create a copy of this Parameter.
  std::shared_ptr<Object> Copy() const override;
  /// @brief Return the value node.
  Node *value() const { return value_.get(); }
  /// @brief Set the value of the parameter node. Can only be expression, parameter, or literal.
  Parameter *SetValue(const std::shared_ptr<Node> &value);

  /// @brief Return all objects that this parameter owns. This includes the value node and type generics.
  void AppendReferences(std::vector<Object *> *out) const override;

  // TODO(johanpel): Work-around for parameters nodes that are size nodes of arrays.
  //  To prevent this, it requires a restructuring of node arrays.
  std::optional<NodeArray *> node_array_parent;

  /// Parameter value.
  std::shared_ptr<Node> value_;
};

/// @brief Get a smart pointer to a new Parameter, optionally owning a default value Literal.
std::shared_ptr<Parameter> parameter(const std::string &name,
                                     const std::shared_ptr<Type> &type,
                                     std::shared_ptr<Node> value = nullptr);

}  // namespace cerata
