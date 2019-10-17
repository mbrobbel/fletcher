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

#include "cerata/stream.h"

#include <utility>

#include "cerata/type.h"

namespace cerata {

Stream &Stream::SetElementType(std::shared_ptr<Type> type) {
  // Invalidate mappers that point to this type from the other side
  for (auto &mapper : mappers_) {
    mapper->b()->RemoveMappersTo(this);
  }
  // Invalidate all mappers from this type
  mappers_ = {};
  // Set the new element type
  fields_.back()->SetType(std::move(type));

  return *this;
}

bool Stream::CanGenerateMapper(const Type &other) const {
  if (other.Is(Type::RECORD)) {
    auto rec = dynamic_cast<const Record &>(other);
    // If this and the other streams are "equal", a mapper can be generated
    if (IsEqual(other)) {
      return true;
    } else {
      // We can also map an empty stream, without mapping the elements. In practise, a back-end might emit e.g.
      // additional ready/valid/count wires, connect those, but not any data elements.
      if ((this->fields_.back()->type() == nul()) || (rec.fields().back()->type() == nul())) {
        return true;
      }
    }
  }
  return false;
}

std::shared_ptr<TypeMapper> Stream::GenerateMapper(Type *other) {
  // Check if we can even do this:
  if (!CanGenerateMapper(*other)) {
    CERATA_LOG(FATAL, "No mapper generator known from Stream to " + other->name() + ToString(other->id()));
  }
  if (IsEqual(*other)) {
    return TypeMapper::MakeImplicit(this, other);
  } else {
    // If this or the other stream has a no element type:
    if ((fields_.back()->type() == nul()) || (dynamic_cast<Stream *>(other)->fields_.back()->type() == nul())) {
      auto mapper = TypeMapper::Make(this, other);
      // Only connect the two stream flat types.
      auto matrix = mapper->map_matrix();
      matrix(0, 0) = 1;
      mapper->SetMappingMatrix(matrix);
      return mapper;
    }
  }
  return nullptr;
}

Stream::Stream(const std::string &name,
               const std::string &data_name,
               const std::shared_ptr<Type> &data_type,
               const std::vector<std::shared_ptr<Field>> &control)
    : Record(name) {
  for (const auto &f : control) {
    AddField(f);
  }
  AddField(field(data_name, data_type));
}

std::shared_ptr<Type> Stream::valid() {
  static auto result = bit("valid");
  return result;
}

std::shared_ptr<Type> Stream::ready() {
  static auto result = bit("ready");
  return result;
}

std::shared_ptr<Stream> stream(const std::string &name,
                               const std::string &element_name,
                               const std::shared_ptr<Type> &element_type,
                               const std::vector<std::shared_ptr<Field>> &control) {
  return std::make_shared<Stream>(name, element_name, element_type, control);
}

std::shared_ptr<Stream> stream(const std::string &element_name, const std::shared_ptr<Type> &element_type) {
  return stream(element_name + "_stream", element_name, element_type);
}

std::shared_ptr<Stream> stream(const std::shared_ptr<Type> &element_type) {
  return stream(element_type->name(), element_type);
}

}  // namespace cerata