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

#include <memory>
#include <string>
#include <optional>
#include <utility>
#include <vector>
#include <unordered_map>

#include "cerata/node.h"
#include "cerata/node_array.h"
#include "cerata/pool.h"

namespace cerata {

// Forward Decl.
class Component;
class Instance;

/**
 * @brief Get any sub-objects that are used by an object, e.g. type generic nodes or array size nodes.
 * @param obj The object from which to derive the required objects.
 * @param out The output.
 */
void GetSubObjects(const Object &obj, std::vector<Object *> *out);

/**
 * @brief Get generic types.
 * @param obj The object from which to derive the required objects.
 * @param out The output.
 * @param include_literals Whether to include literal nodes.
 */
void GetGenericTypes(const Object &obj, std::vector<Type *> *out);

/**
 * @brief A graph representing a hardware structure.
 */
class Graph : public Named {
 public:
  /// Graph type ID for convenient run-time type checking.
  enum ID {
    COMPONENT,  ///< A component graph
    INSTANCE    ///< An instance graph
  };

  /// @brief Construct a new graph
  Graph(std::string name, ID id) : Named(std::move(name)), id_(id) {}

  /// @brief Return the graph type ID.
  ID id() const { return id_; }
  /// @brief Return true if this graph is a component, false otherwise.
  bool IsComponent() const { return id_ == COMPONENT; }
  /// @brief Return true if this graph is an instance, false otherwise.
  bool IsInstance() const { return id_ == INSTANCE; }
  /// @brief Add an object to the component.
  virtual Graph &Add(const std::shared_ptr<Object> &object);
  /// @brief Add a list of objects to the component.
  virtual Graph &Add(const std::vector<std::shared_ptr<Object>> &objects);
  /// @brief Remove an object from the component
  virtual Graph &Remove(Object *obj);

  /// @brief Get all objects of a specific type.
  template<typename T>
  std::vector<T *> GetAll() const {
    std::vector<T *> ret;
    for (const auto &o : objects_) {
      auto co = std::dynamic_pointer_cast<T>(o);
      if (co != nullptr)
        ret.push_back(co.get());
    }
    return ret;
  }

  /// @brief Get a NodeArray object of a specific type with a specific name
  std::optional<NodeArray *> GetArray(Node::NodeID node_id, const std::string &array_name) const;
  /// @brief Get a Node of a specific type with a specific name
  std::optional<Node *> GetNode(const std::string &node_name) const;
  /// @brief Get a Node of a specific type with a specific name
  Node *GetNode(Node::NodeID node_id, const std::string &node_name) const;
  /// @brief Obtain all nodes which ids are in a list of Node::IDs
  std::vector<Node *> GetNodesOfTypes(std::initializer_list<Node::NodeID> ids) const;
  /// @brief Count nodes of a specific node type
  size_t CountNodes(Node::NodeID id) const;
  /// @brief Count nodes of a specific array type
  size_t CountArrays(Node::NodeID id) const;
  /// @brief Get all nodes.
  std::vector<Node *> GetNodes() const { return GetAll<Node>(); }
  /// @brief Get all nodes of a specific type.
  std::vector<Node *> GetNodesOfType(Node::NodeID id) const;
  /// @brief Get all arrays of a specific type.
  std::vector<NodeArray *> GetArraysOfType(Node::NodeID id) const;
  /// @brief Return all graph nodes that do not explicitly belong to the graph.
  std::vector<Node *> GetImplicitNodes() const;

  /// @brief Shorthand to Get(, ..)
  PortArray *prta(const std::string &port_name) const;
  /// @brief Shorthand to Get(Node::PORT, ..)
  Port *prt(const std::string &port_name) const;
  /// @brief Shorthand to Get(Node::SIGNAL, ..)
  Signal *sig(const std::string &signal_name) const;
  /// @brief Shorthand to Get(Node::PARAMETER, ..)
  Parameter *par(const std::string &signal_name) const;
  /// @brief Get a parameter by supplying another parameter. Lookup is done according to the name of the supplied param.
  Parameter *par(const Parameter &param) const;
  /// @brief Get a parameter by supplying another parameter. Lookup is done according to the name of the supplied param.
  Parameter *par(const std::shared_ptr<Parameter> &param) const;

  /// @brief Return a copy of the metadata.
  std::unordered_map<std::string, std::string> meta() const { return meta_; }
  /// @brief Get all objects.
  std::vector<Object *> objects() const { return ToRawPointers(objects_); }
  /// @brief Return true if object with name already exists on graph.
  bool Has(const std::string &name);

  /// @brief Set metadata
  Graph &SetMeta(const std::string &key, std::string value);

  /// @brief Return a human-readable representation.
  std::string ToString() const { return name(); }

 protected:
  /// Graph type id for convenience
  ID id_;
  /// Graph objects.
  std::vector<std::shared_ptr<Object>> objects_;
  /// KV storage for metadata of tools or specific backend implementations
  std::unordered_map<std::string, std::string> meta_;
};

/**
 * @brief A Component graph.
 *
 * A component graph may contain all node types.
 */
class Component : public Graph {
 public:
  /// @brief Construct an empty Component.
  explicit Component(std::string name) : Graph(std::move(name), COMPONENT) {}
  /**
   * @brief Add and take ownership of an Instance graph.
   * @param child   The child graph to add.
   * @return        This component if successful.
   */
  Component &AddChild(std::unique_ptr<Instance> child);
  /**
   * @brief Add a child Instance from a component.
   * @param comp    The component to instantiate and add.
   * @param name    The name of the new instance. If left blank, it will use the Component name + "_inst".
   * @return        A pointer to the instantiated component.
   */
  Instance *AddInstanceOf(Component *comp, const std::string &name = "");
  /// @brief Returns all Instance graphs from this Component.
  std::vector<Instance *> children() const { return ToRawPointers(children_); }
  /// @brief Returns all unique Components that are referred to by child Instances of this graph.
  virtual std::vector<const Component *> GetAllInstanceComponents() const;

 protected:
  /// Graph children / subgraphs.
  std::vector<std::unique_ptr<Instance>> children_;
};

/// @brief Construct a Component with initial nodes
std::shared_ptr<Component> component(std::string name,
                                     const std::vector<std::shared_ptr<Object>> &nodes,
                                     ComponentPool *component_pool = default_component_pool());
/// @brief Construct an empty Component with only a name.
std::shared_ptr<Component> component(std::string name,
                                     ComponentPool *component_pool = default_component_pool());

/**
 * @brief An instance.
 *
 * An instance graph may not contain any signals.
 */
class Instance : public Graph {
 public:
  /// @brief Construct an Instance of a Component, copying over all its ports and parameters
  explicit Instance(Component *comp, std::string name);
  /// @brief Add a node to the component, throwing an exception if the node is a signal.
  Graph &Add(const std::shared_ptr<Object> &obj) override;
  /// @brief Return the component this is an instance of.
  Component *component() const { return component_; }
  /// @brief Return the parent graph.
  Graph *parent() const { return parent_; }
  /// @brief Set the parent.
  Graph &SetParent(Graph *parent);
 protected:
  /// The component that this instance instantiates.
  Component *component_{};
  /// The parent of this instance.
  Graph *parent_{};
};

/// @brief Construct a shared pointer to a Component
std::unique_ptr<Instance> instance(Component *component, const std::string &name = "");

/// @brief Rebind a type generic node to a component.
void RebindGeneric(Component *comp, Node *generic, std::unordered_map<Node *, Node *> *rebinding);

}  // namespace cerata
