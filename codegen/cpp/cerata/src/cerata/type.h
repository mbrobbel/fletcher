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

#include <optional>
#include <utility>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include "cerata/utils.h"
#include "cerata/flattype.h"
#include "cerata/domain.h"

namespace cerata {

// Forward decl.
class Node;
class Literal;
class Graph;
Literal *rintl(int i);
std::shared_ptr<Literal> intl(int i);

using OptionalNode =  std::optional<std::shared_ptr<Node>>;

/**
 * @brief A Type
 *
 * Types can logically be classified as follows.
 *
 * Physical:
 * - Yes.
 *      They can be immediately represented as bits in hardware.
 * - No.
 *      They can not be represented as bits in hardware without a more elaborate definition.
 *
 * Nested:
 * - No.
 *      These types contain no subtype.
 * - Yes.
 *      These types contain some subtype, and so the realm depends on the sub-types used.
 *
 * Generic:
 * - No.
 *      Not parametrized by some node.
 * - Yes.
 *      Parameterized by some node.
 */
class Type : public Named, public std::enable_shared_from_this<Type> {
 public:
  /// @brief The Type ID. Used for convenient type checking.
  //
  //                Physical | Nested    | Generic
  //                ---------|-----------|--------
  enum ID {
    BIT,      ///<  Yes      | No        | No
    VECTOR,   ///<  Yes      | No        | Yes

    NUL,      ///<  No       | No        | No
    INTEGER,  ///<  No       | No        | No
    NATURAL,  ///<  No       | No        | No
    STRING,   ///<  No       | No        | No
    BOOLEAN,  ///<  No       | No        | No

    RECORD,   ///<  ?        | Yes       | ?
    STREAM    ///<  ?        | Yes       | ?
  };

  // TODO(johanpel): potentially remove stream as it could be just a record

  /**
   * @brief Type constructor.
   * @param name    The name of the Type.
   * @param id      The Type::ID.
   */
  explicit Type(std::string name, ID id);

  /// @brief Return the Type ID.
  inline ID id() const { return id_; }

  /**
   * @brief Determine if this Type is exactly equal to an other Type.
   * @param other   The other type.
   * @return        True if exactly equal, false otherwise.
   */
  virtual bool IsEqual(const Type &other) const;

  /// @brief Return true if the Type ID is type_id, false otherwise.
  bool Is(ID type_id) const;
  /// @brief Return true if the Type has an immediate physical representation, false otherwise.
  virtual bool IsPhysical() const = 0;
  /// @brief Return true if the Type is a nested, false otherwise.
  virtual bool IsNested() const = 0;
  /// @brief Return true if the Type is a generic type.
  virtual bool IsGeneric() const = 0;
  /// @brief Return the width of the type, if it is synthesizable.
  virtual std::optional<Node *> width() const { return {}; }
  /// @brief Return the Type ID as a human-readable string.
  std::string ToString(bool show_meta = false, bool show_mappers = false) const;

  /// @brief Return possible type mappers.
  std::vector<std::shared_ptr<TypeMapper>> mappers() const;
  /// @brief Add a type mapper.
  void AddMapper(const std::shared_ptr<TypeMapper> &mapper, bool remove_existing = true);
  /// @brief Get a mapper to another type, if it exists. Generates one, if possible, when generate_implicit = true.
  std::optional<std::shared_ptr<TypeMapper>> GetMapper(Type *other, bool generate_implicit = true);
  /// @brief Remove all mappers to a specific type
  int RemoveMappersTo(Type *other);
  /// @brief Get a mapper to another type, if it exists.
  std::optional<std::shared_ptr<TypeMapper>> GetMapper(const std::shared_ptr<Type> &other);
  /// @brief Check if a mapper can be generated to another specific type.
  virtual bool CanGenerateMapper(const Type &other) const { return false; }
  /// @brief Generate a new mapper to a specific other type. Should be checked with CanGenerateMapper first, or throws.
  virtual std::shared_ptr<TypeMapper> GenerateMapper(Type *other) { return nullptr; }

  /// @brief Obtain any nodes that this type uses as generics.
  virtual std::vector<Node *> GetGenerics() const { return {}; }
  /// @brief Obtain any nested types.
  virtual std::vector<Type *> GetNested() const { return {}; }

  /// @brief KV storage for metadata of tools or specific backend implementations
  std::unordered_map<std::string, std::string> meta;

  /**
  * @brief Make a copy of the type, and rebind any generic nodes that are keys in the rebind_map to their values.
  *
   * This is useful in case type generic nodes are on some instance graph and have to be copied over to a component
   * graph. In that case, the component graph has to copy over these nodes and rebind the generic nodes of the type.
  *
  * @return A copy of the type.
  */
  virtual std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> rebinding) const = 0;

 protected:
  /// Type ID
  ID id_;
  /// A list of mappers that can map this type to another type.
  std::vector<std::shared_ptr<TypeMapper>> mappers_;
};

/// @brief Return a static bit type.
std::shared_ptr<Type> bit(const std::string &name = "bit");
/// @brief A bit type.
struct Bit : public Type {
  bool IsPhysical() const override { return true; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// @brief Bit type constructor.
  explicit Bit(std::string name) : Type(std::move(name), Type::BIT) {}
  /// @brief Bit width returns integer literal 1.
  std::optional<Node *> width() const override;

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> rebinding) const override;
};

/// @brief Return a static Nul type.
std::shared_ptr<Type> nul();
/// @brief Void type. Useful for e.g. empty streams.
struct Nul : public Type {
  bool IsPhysical() const override { return false; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// Void type constructor.
  explicit Nul(std::string name) : Type(std::move(name), Type::NUL) {}

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> generic_map) const override { return nul(); }
};

/// @brief Static boolean type.
std::shared_ptr<Type> boolean();

/// @brief Boolean type.
struct Boolean : public Type {
  bool IsPhysical() const override { return false; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// @brief Boolean type constructor.
  explicit Boolean(std::string name) : Type(std::move(name), Type::BOOLEAN) {}

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> generic_map) const override { return boolean(); }
};

/// @brief Return a static integer type.
std::shared_ptr<Type> integer();
/// @brief Integer type.
struct Integer : public Type {
  bool IsPhysical() const override { return false; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// @brief Integer type constructor.
  explicit Integer(std::string name) : Type(std::move(name), Type::INTEGER) {}

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> generic_map) const override { return integer(); }
};

/// @brief Return a static natural type.
std::shared_ptr<Type> natural();
/// @brief Natural type.
struct Natural : public Type {
  bool IsPhysical() const override { return false; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// @brief Integer type constructor.
  explicit Natural(std::string name) : Type(std::move(name), Type::NATURAL) {}

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> generic_map) const override { return natural(); }
};

/// @brief Static string type.
std::shared_ptr<Type> string();

/// @brief String type.
struct String : public Type {
  bool IsPhysical() const override { return false; }
  bool IsGeneric() const override { return false; }
  bool IsNested() const override { return false; }

  /// @brief String type constructor.
  explicit String(std::string name) : Type(std::move(name), Type::STRING) {}

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> generic_map) const override { return string(); }
};

/// @brief Vector type.
class Vector : public Type {
 public:
  bool IsPhysical() const override { return true; }
  bool IsGeneric() const override { return true; }
  bool IsNested() const override { return false; }

  /// @brief Vector constructor.
  Vector(std::string name, const OptionalNode &width);
  /// @brief Return a pointer to the node representing the width of this vector, if specified.
  std::optional<Node *> width() const override;
  /// @brief Set the width of this vector.
  Type &SetWidth(std::shared_ptr<Node> width);
  /// @brief Determine if this Type is exactly equal to an other Type.
  bool IsEqual(const Type &other) const override;
  /// @brief Returns the width parameter of this vector, if any. Otherwise an empty list;
  std::vector<Node *> GetGenerics() const override;

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> rebinding) const override;;

 private:
  /// The optional vector width.
  OptionalNode width_;
};

/// @brief Create a new vector type, and return a shared pointer to it.
std::shared_ptr<Type> vector(const std::string &name, const OptionalNode &width);

/// @brief Create a new vector type of width W and return a shared pointer to it.
template<int W>
std::shared_ptr<Type> vector(const std::string &name) {
  return std::make_shared<Vector>(name, intl(W));
}

/// @brief Create a new vector type of width W and returns a shared pointer to it.
template<int W>
std::shared_ptr<Type> vector() {
  auto result = std::make_shared<Vector>("vec" + std::to_string(W), intl(W));
  return result;
}

/// @brief Create a new Vector Type of some width.
std::shared_ptr<Type> vector(unsigned int width);

/// @brief Create a new Vector Type of some width.
std::shared_ptr<Type> vector(std::string name, unsigned int width);

/// @brief A Record field.
class Field : public Named {
 public:
  /// @brief RecordField constructor.
  Field(std::string name, std::shared_ptr<Type> type, bool invert = false, bool sep = true);
  /// @brief Return the type of the RecordField.
  std::shared_ptr<Type> type() const { return type_; }
  /// @brief Return if this individual field should be inverted w.r.t. parent Record type itself on graph edges.
  bool invert() const { return invert_; }
  /// @brief Return true if in name generation of this field name for flattened types a separator should be placed.
  bool sep() const { return sep_; }
  /// @brief Disable the separator in name generation of this field.
  void NoSep() { sep_ = false; }
  /// @brief Enable the separator in name generation of this field.
  void UseSep() { sep_ = true; }
  /// @brief Metadata for back-end implementations
  std::unordered_map<std::string, std::string> meta;
  /// @brief Create a copy of the field.
  std::shared_ptr<Field> Copy(std::unordered_map<Node *, Node *> rebinding = {}) const;

 private:
  /// The type of the field.
  std::shared_ptr<Type> type_;
  /// Whether this field should be inverted in directed use of the parent type.
  bool invert_;
  /// Whether this field should generate a separator for name/identifier generation in downstream tools.
  bool sep_;
};

/// @brief Create a new RecordField, and return a shared pointer to it.
std::shared_ptr<Field> field(const std::string &name,
                             const std::shared_ptr<Type> &type,
                             bool invert = false,
                             bool sep = true);

/// @brief Create a new RecordField, and return a shared pointer to it. The name will be taken from the type.
std::shared_ptr<Field> field(const std::shared_ptr<Type> &type, bool invert = false, bool sep = true);

/// @brief Convenience function to disable the separator for a record field.
std::shared_ptr<Field> NoSep(std::shared_ptr<Field> field);

/// @brief A Record type containing zero or more RecordFields.
class Record : public Type {
 public:
  bool IsPhysical() const override;
  bool IsGeneric() const override;
  bool IsNested() const override { return true; }

  /// @brief Record constructor.
  explicit Record(std::string name, std::vector<std::shared_ptr<Field>> fields = {});
  /// @brief Add a RecordField to this Record.
  Record &AddField(const std::shared_ptr<Field> &field, std::optional<size_t> index = std::nullopt);
  /// @brief Return the RecordField at index i contained by this record.
  std::shared_ptr<Field> field(size_t i) const { return fields_[i]; }
  /// @brief Return all fields contained by this record.
  std::vector<std::shared_ptr<Field>> fields() const { return fields_; }
  /// @brief Return the number of fields in this record.
  inline size_t num_fields() const { return fields_.size(); }
  /// @brief Determine if this Type is exactly equal to an other Type.
  bool IsEqual(const Type &other) const override;
  /// @brief Return all nodes that potentially parametrize the fields of this record.
  std::vector<Node *> GetGenerics() const override;
  /// @brief Obtain any nested types.
  std::vector<Type *> GetNested() const override;

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> rebinding) const override;

 private:
  std::vector<std::shared_ptr<Field>> fields_;
};

/// @brief Create a new Record Type, and return a shared pointer to it.
std::shared_ptr<Record> record(const std::string &name,
                               const std::vector<std::shared_ptr<Field>> &fields = {});

/// @brief A Stream type.
class Stream : public Type {
 public:
  bool IsPhysical() const override;
  bool IsGeneric() const override;
  bool IsNested() const override { return true; }
  /**
   * @brief                 Construct a Stream type
   * @param type_name       The name of the Stream type.
   * @param element_type    The type of the elements transported by the stream
   * @param element_name    The name of the elements transported by the stream
   * @param epc             Maximum elements per cycle
   */
  Stream(const std::string &type_name, std::shared_ptr<Type> element_type, std::string element_name, int epc = 1);
  /// @brief Set the type of the elements of this stream. Forgets any existing mappers.
  void SetElementType(std::shared_ptr<Type> type);
  /// @brief Return the type of the elements of this stream.
  std::shared_ptr<Type> element_type() const { return element_type_; }
  /// @brief Set the name of the elements of this stream.
  void SetElementName(std::string name) { element_name_ = std::move(name); }
  /// @brief Return the name of the elements of this stream.
  std::string element_name() const { return element_name_; }

  /// @brief Return the maximum number of elements per cycle this stream can deliver.
  // TODO(johanpel): turn EPC into a parameter or literal node
  int epc() { return epc_; }

  /// @brief Determine if this Stream is exactly equal to another Stream.
  bool IsEqual(const Type &other) const override;

  /// @brief Check if a mapper can be generated to another specific type.
  bool CanGenerateMapper(const Type &other) const override;
  /// @brief Generate a new mapper to a specific other type. Should be checked with CanGenerateMapper first, or throws.
  std::shared_ptr<TypeMapper> GenerateMapper(Type *other) override;

  std::vector<Type *> GetNested() const override;

  std::shared_ptr<Type> Copy(std::unordered_map<Node *, Node *> rebinding) const override;

 private:
  /// @brief The type of the elements traveling over this stream.
  std::shared_ptr<Type> element_type_;
  /// @brief The name of the elements traveling over this stream.
  std::string element_name_;

  /// TODO(johanpel): let streams have a clock domain so we can instantiate CDC automatically.

  /// @brief Elements Per Cycle
  int epc_ = 1;
};

/// @brief Create a smart pointer to a new Stream type. Stream name will be stream:\<type name\>, the elements "data".
std::shared_ptr<Stream> stream(const std::shared_ptr<Type> &element_type, int epc = 1);
/// @brief Shorthand to create a smart pointer to a new Stream type. The elements are named "data".
std::shared_ptr<Stream> stream(const std::string &name, const std::shared_ptr<Type> &element_type, int epc = 1);
/// @brief Shorthand to create a smart pointer to a new Stream type.
std::shared_ptr<Stream> stream(const std::string &name,
                               const std::shared_ptr<Type> &element_type,
                               const std::string &element_name,
                               int epc = 1);

}  // namespace cerata
