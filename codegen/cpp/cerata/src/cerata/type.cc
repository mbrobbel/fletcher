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

#include "cerata/type.h"

#include <utility>
#include <string>
#include <iostream>

#include "cerata/utils.h"
#include "cerata/node.h"
#include "cerata/flattype.h"
#include "cerata/pool.h"

namespace cerata {

bool Type::Is(Type::ID type_id) const {
  return type_id == id_;
}

Type::Type(std::string name, Type::ID id) : Named(std::move(name)), id_(id) {}

std::string Type::ToString(bool show_meta, bool show_mappers) const {
  std::string ret;
  switch (id_) {
    case BIT    : ret = name() + ":Bit";
      break;
    case VECTOR : ret = name() + ":Vec";
      break;
    case INTEGER: ret = name() + ":Int";
      break;
    case STRING : ret = name() + ":Str";
      break;
    case BOOLEAN: ret = name() + ":Bo";
      break;
    case RECORD : ret = name() + ":Rec";
      break;
    case STREAM : ret = name() + ":Stm";
      break;
    default :throw std::runtime_error("Corrupted Type ID.");
  }

  if (show_meta || show_mappers) {
    ret += "[";
    // Append metadata
    ret += ::cerata::ToString(meta);
    if (show_mappers && !mappers_.empty()) {
      ret += " ";
    }

    // Append mappers
    if (show_mappers && !mappers_.empty()) {
      ret += "mappers={";
      size_t i = 0;
      for (const auto &m : mappers_) {
        ret += m->b()->ToString();
        if (i != mappers_.size() - 1) {
          ret += ", ";
        }
        i++;
      }
      ret += "}";
    }
    ret += "]";
  }
  return ret;
}

std::vector<std::shared_ptr<TypeMapper>> Type::mappers() const {
  return mappers_;
}

void Type::AddMapper(const std::shared_ptr<TypeMapper> &mapper, bool remove_existing) {
  Type *other = mapper->b();
  // Check if a mapper doesn't already exists
  if (GetMapper(other, false)) {
    if (!remove_existing) {
      CERATA_LOG(FATAL, "Mapper already exists to convert "
                        "from " + this->ToString(true, true) + " to " + other->ToString(true, true));
    } else {
      RemoveMappersTo(other);
    }
  }

  // Check if the supplied mapper does not convert from this type.
  if (mapper->a() != this) {
    CERATA_LOG(FATAL, "Type converter does not convert from " + name());
  }

  // Add the converter to this Type
  mappers_.push_back(mapper);

  // If the other type doesnt already have it, add the inverse map there as well.
  if (!other->GetMapper(this)) {
    other->AddMapper(mapper->Inverse());
  }
}

std::optional<std::shared_ptr<TypeMapper>> Type::GetMapper(const std::shared_ptr<Type> &other) {
  return GetMapper(other.get());
}

std::optional<std::shared_ptr<TypeMapper>> Type::GetMapper(Type *other, bool generate_implicit) {
  // Search for an existing type mapper.
  for (const auto &m : mappers_) {
    if (m->CanConvert(this, other)) {
      return m;
    }
  }

  if (generate_implicit) {
    // Implicit type mappers maybe be generated in three cases:

    // If it's exactly the same type object,
    if (other == this) {
      // Generate a type mapper to itself using the TypeMapper constructor.
      return TypeMapper::Make(this);
    }

    // If there is an explicit function in this Type to generate a mapper:
    if (CanGenerateMapper(*other)) {
      // Generate, add and return the new mapper.
      auto new_mapper = GenerateMapper(other);
      this->AddMapper(new_mapper);
      return new_mapper;
    }

    // Or if its an "equal" type, where each flattened type is equal.
    if (IsEqual(*other)) {
      // Generate an implicit type mapping.
      return TypeMapper::MakeImplicit(this, other);
    }
  }

  // There is no mapper
  return {};
}

int Type::RemoveMappersTo(Type *other) {
  int removed = 0;
  for (auto m = mappers_.begin(); m < mappers_.end(); m++) {
    if ((*m)->CanConvert(this, other)) {
      mappers_.erase(m);
      removed++;
    }
  }
  return removed;
}

bool Type::IsEqual(const Type &other) const {
  return other.id() == id_;
}

Vector::Vector(std::string name, const std::shared_ptr<Node> &width)
    : Type(std::move(name), Type::VECTOR) {
  // Sanity check the width generic node.
  if (!(width->IsParameter() || width->IsLiteral() || width->IsExpression())) {
    CERATA_LOG(FATAL, "Vector width can only be Parameter, Literal or Expression node.");
  }
  width_ = width;
}

std::shared_ptr<Type> vector(const std::string &name, const std::shared_ptr<Node> &width) {
  return std::make_shared<Vector>(name, width);
}

std::shared_ptr<Type> vector(const std::shared_ptr<Node> &width) {
  return std::make_shared<Vector>("Vec_" + width->ToString(), width);
}

std::shared_ptr<Type> vector(unsigned int width) {
  return vector("vec_" + std::to_string(width), intl(static_cast<int>(width)));
}

std::shared_ptr<Type> vector(std::string name, unsigned int width) {
  auto ret = vector(width);
  ret->SetName(std::move(name));
  return ret;
}

std::optional<Node *> Vector::width() const {
  return width_.get();
}

bool Vector::IsEqual(const Type &other) const {
  // Must also be a vector
  if (other.Is(Type::VECTOR)) {
    // Must both have a width
    if (width_ && other.width()) {
      // TODO(johanpel): implement proper width checking..
      return true;
    }
  }
  return false;
}

std::vector<Node *> Vector::GetGenerics() const {
  return {width_.get()};
}

Type &Vector::SetWidth(std::shared_ptr<Node> width) {
  width_ = std::move(width);
  return *this;
}

std::shared_ptr<Stream> stream(const std::string &name, const std::shared_ptr<Type> &element_type, int epc) {
  return std::make_shared<Stream>(name, element_type, "", epc);
}

std::shared_ptr<Stream> stream(const std::string &name,
                               const std::shared_ptr<Type> &element_type,
                               const std::string &element_name,
                               int epc) {
  return std::make_shared<Stream>(name, element_type, element_name, epc);
}

std::shared_ptr<Stream> stream(const std::shared_ptr<Type> &element_type, int epc) {
  return std::make_shared<Stream>("stream-" + element_type->name(), element_type, "", epc);
}

Stream::Stream(const std::string &type_name, std::shared_ptr<Type> element_type, std::string element_name, int epc)
    : Type(type_name, Type::STREAM),
      element_type_(std::move(element_type)),
      element_name_(std::move(element_name)),
      epc_(epc) {
  if (element_type_ == nullptr) {
    throw std::runtime_error("Stream element type cannot be nullptr.");
  }
}

std::shared_ptr<Type> bit(const std::string &name) {
  std::shared_ptr<Type> result = std::make_shared<Bit>(name);
  return result;
}

std::shared_ptr<Type> nul() {
  static std::shared_ptr<Type> result = std::make_shared<Nul>("null");
  return result;
}

std::shared_ptr<Type> boolean() {
  static std::shared_ptr<Type> result = std::make_shared<Boolean>("boolean");
  return result;
}

std::shared_ptr<Type> integer() {
  static std::shared_ptr<Type> result = std::make_shared<Integer>("integer");
  return result;
}

std::shared_ptr<Type> string() {
  static std::shared_ptr<Type> result = std::make_shared<String>("string");
  return result;
}

std::optional<Node *> Bit::width() const {
  return rintl(1);
}

Field::Field(std::string name, std::shared_ptr<Type> type, bool invert, bool sep)
    : Named(std::move(name)), type_(std::move(type)), invert_(invert), sep_(sep) {}

std::shared_ptr<Field> field(const std::string &name, const std::shared_ptr<Type> &type, bool invert, bool sep) {
  return std::make_shared<Field>(name, type, invert, sep);
}

std::shared_ptr<Field> field(const std::shared_ptr<Type> &type, bool invert, bool sep) {
  return std::make_shared<Field>(type->name(), type, invert, sep);
}

std::shared_ptr<Field> NoSep(std::shared_ptr<Field> field) {
  field->NoSep();
  return field;
}

Record &Record::AddField(const std::shared_ptr<Field> &field, std::optional<size_t> index) {
  if (index) {
    auto it = fields_.begin() + *index;
    fields_.insert(it, field);
  } else {
    fields_.push_back(field);
  }
  return *this;
}

Record::Record(std::string name, std::vector<std::shared_ptr<Field>> fields)
    : Type(std::move(name), Type::RECORD), fields_(std::move(fields)) {}

std::shared_ptr<Record> record(const std::string &name, const std::vector<std::shared_ptr<Field>> &fields) {
  return std::make_shared<Record>(name, fields);
}

std::shared_ptr<Record> record(const std::vector<std::shared_ptr<Field>> &fields) {
  return record("", fields);
}

bool Stream::IsEqual(const Type &other) const {
  if (other.Is(Type::STREAM)) {
    auto &other_stream = dynamic_cast<const Stream &>(other);
    bool eq = element_type()->IsEqual(*other_stream.element_type());
    return eq;
  }
  return false;
}

void Stream::SetElementType(std::shared_ptr<Type> type) {
  // Invalidate mappers that point to this type from the other side
  for (auto &mapper : mappers_) {
    mapper->b()->RemoveMappersTo(this);
  }
  // Invalidate all mappers from this type
  mappers_ = {};
  // Set the new element type
  element_type_ = std::move(type);
}

bool Stream::CanGenerateMapper(const Type &other) const {
  switch (other.id()) {
    case Type::STREAM:
      // If this and the other streams are "equal", a mapper can be generated
      if (IsEqual(other)) {
        return true;
      } else {
        // We can also map an empty stream, without mapping the elements. In practise, a back-end might emit e.g.
        // additional ready/valid/count wires, connect those, but not any data elements.
        if ((this->element_type() == nul()) || (dynamic_cast<const Stream &>(other).element_type() == nul())) {
          return true;
        }
      }
    default:return false;
  }
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
    if ((this->element_type() == nul()) || (dynamic_cast<Stream *>(other)->element_type() == nul())) {
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

bool Stream::IsPhysical() const { return element_type_->IsPhysical(); }
bool Stream::IsGeneric() const { return element_type_->IsGeneric(); }

std::vector<Type *> Stream::GetNested() const {
  std::vector<Type *> result;
  result.push_back(element_type().get());
  // Push back any nested types.
  auto nested = element_type()->GetNested();
  result.insert(result.end(), nested.begin(), nested.end());
  return result;
}

bool Record::IsEqual(const Type &other) const {
  if (&other == this) {
    return true;
  }
  // Must also be a record
  if (!other.Is(Type::RECORD)) {
    return false;
  }
  // Must have same number of fields
  auto &other_record = dynamic_cast<const Record &>(other);
  if (other_record.num_fields() != this->num_fields()) {
    return false;
  }
  // Each field must also be of equal type
  for (size_t i = 0; i < this->num_fields(); i++) {
    auto a = this->field(i)->type();
    auto b = other_record.field(i)->type();
    if (!a->IsEqual(*b)) {
      return false;
    }
  }
  // If we didn't return already, the Record Types are the same
  return true;
}

std::vector<Node *> Record::GetGenerics() const {
  std::vector<Node *> result;
  for (const auto &field : fields_) {
    auto field_params = field->type()->GetGenerics();
    result.insert(result.end(), field_params.begin(), field_params.end());
  }
  return result;
}

std::vector<Type *> Record::GetNested() const {
  std::vector<Type *> result;
  for (const auto &field : fields_) {
    // Push back the field type itself.
    result.push_back(field->type().get());
    // Push back any nested types.
    auto nested = field->type()->GetNested();
    result.insert(result.end(), nested.begin(), nested.end());
  }
  return result;
}

bool Record::IsPhysical() const {
  for (const auto &f : fields_) {
    if (!f->type()->IsPhysical()) {
      return false;
    }
  }
  return true;
}

bool Record::IsGeneric() const {
  for (const auto &f : fields_) {
    if (f->type()->IsGeneric()) {
      return true;
    }
  }
  return false;
}

std::shared_ptr<Type> Bit::Copy(std::unordered_map<Node *, Node *> rebinding) const {
  std::shared_ptr<Type> result;
  result = bit(name());

  result->meta = meta;

  for (const auto &mapper : mappers_) {
    auto new_mapper = mapper->Make(result.get(), mapper->b());
    new_mapper->SetMappingMatrix(mapper->map_matrix());
    result->AddMapper(new_mapper);
  }

  return result;
}

std::shared_ptr<Type> Vector::Copy(std::unordered_map<Node *, Node *> rebinding) const {
  std::shared_ptr<Type> result;
  std::optional<std::shared_ptr<Node>> new_width = width_;
  if (rebinding.count(width_.get()) > 0) {
    new_width = rebinding.at(width_.get())->shared_from_this();
  }
  result = vector(name(), *new_width);

  result->meta = meta;

  for (const auto &mapper : mappers_) {
    auto new_mapper = mapper->Make(result.get(), mapper->b());
    new_mapper->SetMappingMatrix(mapper->map_matrix());
    result->AddMapper(new_mapper);
  }

  return result;
}

std::shared_ptr<Field> Field::Copy(std::unordered_map<Node *, Node *> rebinding) const {
  std::shared_ptr<Field> result;
  auto type = type_;
  if (type_->IsGeneric()) {
    type = type_->Copy(std::move(rebinding));
  }
  result = field(name(), type, invert_, sep_);
  result->meta = meta;
  return result;
}

std::shared_ptr<Type> Record::Copy(std::unordered_map<Node *, Node *> rebinding) const {
  std::shared_ptr<Type> result;
  std::vector<std::shared_ptr<Field>> fields;
  for (const auto &f : fields_) {
    fields.push_back(f->Copy(rebinding));
  }
  result = record(name(), fields);

  result->meta = meta;

  for (const auto &mapper : mappers_) {
    auto new_mapper = mapper->Make(result.get(), mapper->b());
    new_mapper->SetMappingMatrix(mapper->map_matrix());
    result->AddMapper(new_mapper);
  }

  return result;
}

std::shared_ptr<Type> Stream::Copy(std::unordered_map<Node *, Node *> rebinding) const {
  std::shared_ptr<Type> result;
  result = stream(name(), element_type()->Copy(rebinding), element_name(), epc_);

  result->meta = meta;

  for (const auto &mapper : mappers_) {
    auto new_mapper = mapper->Make(result.get(), mapper->b());
    new_mapper->SetMappingMatrix(mapper->map_matrix());
    result->AddMapper(new_mapper);
  }

  return result;
}

}  // namespace cerata
