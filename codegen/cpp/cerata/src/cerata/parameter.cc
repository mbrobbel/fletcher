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

#include <utility>
#include <vector>
#include <string>
#include <memory>

#include "cerata/parameter.h"
#include "cerata/pool.h"

namespace cerata {

Parameter::Parameter(std::string name, const std::shared_ptr<Type> &type, std::shared_ptr<Node> value)
    : MultiOutputNode(std::move(name), Node::NodeID::PARAMETER, type), value_(std::move(value)) {
  if (value_ == nullptr) {
    switch (type->id()) {
      case Type::ID::STRING: value_ = strl("");
        break;
      case Type::ID::BOOLEAN: value_ = booll(false);
        break;
      case Type::ID::INTEGER: value_ = intl(0);
        break;
      default:CERATA_LOG(FATAL, "Parameter value can not be set implicitly.");
    }
  }
}

std::shared_ptr<Object> Parameter::Copy() const {
  auto result = parameter(name(), type_, value_);
  result->meta = this->meta;
  return result;
}

std::shared_ptr<Parameter> parameter(const std::string &name,
                                     const std::shared_ptr<Type> &type,
                                     std::shared_ptr<Node> value) {
  auto p = new Parameter(name, type, std::move(value));
  return std::shared_ptr<Parameter>(p);
}

Parameter *Parameter::SetValue(const std::shared_ptr<Node> &value) {
  if (value->IsSignal() || value->IsPort()) {
    CERATA_LOG(FATAL, "Parameter value can not be or refer to signal or port nodes.");
  }
  value_ = value;
  return this;
}

void Parameter::AppendReferences(std::vector<Object *> *out) const {
  out->push_back(value_.get());
  Node::AppendReferences(out);
  value_->AppendReferences(out);
}

}  // namespace cerata
