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

#include "cerata/graph.h"

#include <string>
#include <memory>
#include <map>

#include "cerata/utils.h"
#include "cerata/node.h"
#include "cerata/logging.h"
#include "cerata/object.h"
#include "cerata/pool.h"
#include "cerata/edge.h"

namespace cerata {

void GetObjectReferences(const Object &obj, std::vector<Object *> *out) {
  if (obj.IsNode()) {
    // If the object is a node, its type may be a generic type.
    auto &node = dynamic_cast<const Node &>(obj);
    // Obtain any potential type generic nodes.
    auto params = node.type()->GetGenerics();
    for (const auto &p : params) {
      out->push_back(p);
    }
    // If the node is an expression, we need to grab every object from the tree.

  } else if (obj.IsArray()) {
    // If the object is an array, we can obtain the generic nodes from the base type.
    auto &array = dynamic_cast<const NodeArray &>(obj);
    // Add the type generic nodes of the base node first.
    GetObjectReferences(*array.base(), out);
    // An array has a size node. Add that as well.
    out->push_back(array.size());
  }
}

Graph &Graph::Add(const std::shared_ptr<Object> &object) {
  // Check for duplicates in name / ownership
  for (const auto &o : objects_) {
    if (o->name() == object->name()) {
      if (o.get() == object.get()) {
        // Graph already owns object. We can skip adding.
        return *this;
      } else {
        CERATA_LOG(FATAL, "Graph " + name() + " already contains an object with name " + object->name());
      }
    }
  }
  // Get any objects referenced by this object. They must already be on this graph.
  std::vector<Object *> generics;
  GetObjectReferences(*object, &generics);
  for (const auto &gen : generics) {
    // If the sub-object has a parent and it is this graph, everything is OK.
    if (gen->parent()) {
      if (gen->parent().value() == this) {
        continue;
      }
    }
    // Literals are owned by the literal pool, so everything is OK as well in that case.
    if (gen->IsNode()) {
      auto gen_node = dynamic_cast<Node *>(gen);
      if (gen_node->IsLiteral()) {
        continue;
      }
      if (gen_node->IsExpression()) {
        continue;
      }
    }
    // Otherwise throw an error.
    CERATA_LOG(FATAL, "Object [" + gen->name()
        + "] bound to object [" + object->name()
        + "] is not present on Graph " + name());
  }
  // No conflicts, add the object
  objects_.push_back(object);
  // Set this graph as parent.
  object->SetParent(this);

  return *this;
}

Graph &Graph::Add(const std::vector<std::shared_ptr<Object>> &objects) {
  for (const auto &obj : objects) {
    Add(obj);
  }
  return *this;
}

Graph &Graph::Remove(Object *obj) {
  for (auto o = objects_.begin(); o < objects_.end(); o++) {
    if (o->get() == obj) {
      objects_.erase(o);
    }
  }
  return *this;
}

Instance *Component::AddInstanceOf(Component *comp, const std::string &name) {
  auto inst = instance(comp, name);
  auto raw_ptr = inst.get();
  AddChild(std::move(inst));
  return raw_ptr;
}

Component &Component::AddChild(std::unique_ptr<Instance> child) {
  // Add this graph to the child's parent graphs
  child->SetParent(this);
  // Add the child graph
  children_.push_back(std::move(child));
  return *this;
}

std::shared_ptr<Component> component(std::string name,
                                     const std::vector<std::shared_ptr<Object>> &objects,
                                     ComponentPool *component_pool) {
  // Create the new component.
  auto *ptr = new Component(std::move(name));
  auto ret = std::shared_ptr<Component>(ptr);
  // Add the component to the pool.
  component_pool->Add(ret);
  // Add the objects shown in the vector.
  for (const auto &object : objects) {
    // Add the object to the graph.
    ret->Add(object);
  }
  return ret;
}

std::shared_ptr<Component> component(std::string name, ComponentPool *component_pool) {
  return component(std::move(name), {}, component_pool);
}

std::vector<const Component *> Component::GetAllInstanceComponents() const {
  std::vector<const Component *> ret;
  for (const auto &child : children_) {
    const Component *comp = nullptr;
    if (child->IsComponent()) {
      // Graph itself is the component to potentially insert
      auto child_as_comp = dynamic_cast<Component *>(child.get());
      comp = child_as_comp;
    } else if (child->IsInstance()) {
      auto child_as_inst = dynamic_cast<Instance *>(child.get());
      // Graph is an instance, its component definition should be inserted.
      comp = child_as_inst->component();
    }
    if (comp != nullptr) {
      if (!Contains(ret, comp)) {
        // If so, push it onto the vector
        ret.push_back(comp);
      }
    }
  }
  return ret;
}

Instance *Component::AddInstanceOf(std::shared_ptr<Component> comp, const std::string &name) {
  return AddInstanceOf(comp.get(), name);
}

std::optional<Node *> Graph::FindNode(const std::string &node_name) const {
  for (const auto &n : GetAll<Node>()) {
    if (n->name() == node_name) {
      return n;
    }
  }
  return std::nullopt;
}

Node *Graph::GetNode(const std::string &node_name) const {
  for (const auto &n : GetAll<Node>()) {
    if (n->name() == node_name) {
      return n;
    }
  }
  CERATA_LOG(FATAL, "Node with name " + node_name + " does not exist on Graph " + this->name());
}

size_t Graph::CountNodes(Node::NodeID id) const {
  size_t count = 0;
  for (const auto &n : GetAll<Node>()) {
    if (n->Is(id)) {
      count++;
    }
  }
  return count;
}

size_t Graph::CountArrays(Node::NodeID id) const {
  size_t count = 0;
  for (const auto &n : GetAll<NodeArray>()) {
    if (n->node_id() == id) {
      count++;
    }
  }
  return count;
}

std::vector<Node *> Graph::GetNodesOfType(Node::NodeID id) const {
  std::vector<Node *> result;
  for (const auto &n : GetAll<Node>()) {
    if (n->Is(id)) {
      result.push_back(n);
    }
  }
  return result;
}

std::vector<NodeArray *> Graph::GetArraysOfType(Node::NodeID id) const {
  std::vector<NodeArray *> result;
  auto arrays = GetAll<NodeArray>();
  for (const auto &a : arrays) {
    if (a->node_id() == id) {
      result.push_back(a);
    }
  }
  return result;
}

Port *Graph::prt(const std::string &name) const {
  return Get<Port>(name);
}

Signal *Graph::sig(const std::string &name) const {
  return Get<Signal>(name);
}

Parameter *Graph::par(const std::string &name) const {
  return Get<Parameter>(name);
}

Parameter *Graph::par(const Parameter &param) const {
  return Get<Parameter>(param.name());
}

Parameter *Graph::par(const std::shared_ptr<Parameter> &param) const {
  return Get<Parameter>(param->name());
}

PortArray *Graph::prt_arr(const std::string &name) const {
  return Get<PortArray>(name);
}

SignalArray *Graph::sig_arr(const std::string &name) const {
  return Get<SignalArray>(name);
}

std::vector<Node *> Graph::GetImplicitNodes() const {
  std::vector<Node *> result;
  for (const auto &n : GetAll<Node>()) {
    for (const auto &i : n->sources()) {
      if (i->src()) {
        if (!i->src()->parent()) {
          result.push_back(i->src());
        }
      }
    }
  }
  auto last = std::unique(result.begin(), result.end());
  result.erase(last, result.end());
  return result;
}

std::vector<Node *> Graph::GetNodesOfTypes(std::initializer_list<Node::NodeID> ids) const {
  std::vector<Node *> result;
  for (const auto &n : GetAll<Node>()) {
    for (const auto &id : ids) {
      if (n->node_id() == id) {
        result.push_back(n);
        break;
      }
    }
  }
  return result;
}

Graph &Graph::SetMeta(const std::string &key, std::string value) {
  meta_[key] = std::move(value);
  return *this;
}

bool Graph::Has(const std::string &name) {
  for (const auto &o : objects_) {
    if (o->name() == name) {
      return true;
    }
  }
  return false;
}

std::string Graph::ToStringAllOjects() const {
  std::stringstream ss;
  for (const auto &o : objects_) {
    ss << o->name();
    if (o != objects_.back()) {
      ss << ", ";
    }
  }
  return ss.str();
}

std::unique_ptr<Instance> instance(Component *component, const std::string &name) {
  auto n = name;
  if (name.empty()) {
    n = component->name() + "_inst";
  }
  auto inst = new Instance(component, n);
  return std::unique_ptr<Instance>(inst);
}

Instance::Instance(Component *comp, std::string
name) : Graph(std::move(name), INSTANCE), component_(comp) {
  // Create a map that maps a component node to an instance node for type generic rebinding.
  std::unordered_map<Node *, Node *> rebind_map;

  // Copy over all parameters.
  for (const auto &param : component_->GetAll<Parameter>()) {
    auto inst_param = std::dynamic_pointer_cast<Parameter>(param->Copy());
    Add(inst_param);
    rebind_map[param] = inst_param.get();
  }

  // Copy over all ports.
  for (const auto &port : component_->GetAll<Port>()) {
    auto inst_port = std::dynamic_pointer_cast<Port>(port->Copy());
    // If the port type is a generic type, we need to bind the generic nodes to copies of those nodes on this instance.
    // If the component was properly constructed, those nodes should have been copied over in the previous part of this
    // function. We can now provide the mapping from component node to instance node and rebind a copy of the type
    // to the new generic nodes.
    if (port->type()->IsGeneric()) {
      // Copy the generic type and bind it to the copied parameters.
      auto new_type = port->type()->Copy(rebind_map);
      // Set the instance port to the newly bound generic type.
      inst_port->SetType(new_type);
    }
    Add(inst_port);
    rebind_map[port] = inst_port.get();
  }

  // Make copies of port arrays
  for (const auto &pa : component_->GetAll<PortArray>()) {
    // Make a copy of the port array.
    auto inst_pa = std::dynamic_pointer_cast<PortArray>(pa->Copy());

    // Same story here. Check if the base type is a generic type. If so, rebind it to the instance nodes and change the
    // type of the instance port array to the newly bound type.
    if (inst_pa->type()->IsGeneric()) {
      auto type = inst_pa->type()->Copy(rebind_map);
      inst_pa->SetType(type);
    }

    // We also need to rebind the port array size to a copy of the size node.
    std::shared_ptr<Node> inst_size;
    // Look up if we've already copied the size node.
    if (rebind_map.count(pa->size()) > 0) {
      inst_size = dynamic_cast<Node *>(rebind_map.at(pa->size()))->shared_from_this();
    } else {
      // There is no copy of the size node yet. Make a copy, add it to the Instance, and remember we made this copy.
      inst_size = std::dynamic_pointer_cast<Node>(pa->size()->Copy());
      Add(inst_size);
      rebind_map[pa->size()] = inst_size.get();
    }
    // We now know which size node we're going to use.
    inst_pa->SetSize(inst_size);
    // Now that the size node is on the instance graph, we can copy over the PortArray.
    Add(inst_pa);
  }
}

Graph &Instance::Add(const std::shared_ptr<Object> &object) {
  if (object->IsNode()) {
    auto node = std::dynamic_pointer_cast<Node>(object);
    if (node->IsSignal()) {
      CERATA_LOG(FATAL, "Instance Graph cannot own Signal nodes. " + node->ToString());
    }
  }
  Graph::Add(object);
  object->SetParent(this);
  return *this;
}

Graph &Instance::SetParent(Graph *parent) {
  parent_ = parent;
  return *this;
}

void RebindGeneric(Component *comp, Node *generic, std::unordered_map<Node *, Node *> *rebinding) {
  // Check if the generic is not already on the rebind map. If it is, we don't have to do anything.
  // Any copies of a type with the intend to rebind its generics will be solved already.
  if (((*rebinding).count(generic) == 0)) {
    // If the type generic is a parameter, its value could be present on this graph already.
    // It could also be a literal. We can just rebind to that value or literal.
    // Go over all sources of this parameter and check if any of them is on the parent.
    bool rebound = false;
    if (generic->IsParameter()) {
      std::vector<Node *> param_sources;
      generic->AsParameter()->Trace(&param_sources);
      for (const auto &ps : param_sources) {
        if (ps->parent() == comp || ps->IsLiteral()) {
          (*rebinding)[generic] = ps;
          rebound = true;
          break;
        }
      }
    }
    // If going through the parameter trace didn't deliver us with a rebinding, make a new copy of the type generic
    // node and add it tot he rebind map.
    if (!rebound) {
      auto new_g = std::dynamic_pointer_cast<Node>(generic->Copy());
      // Prefix the generic node with the name of its original parent.
      auto new_name = generic->name();
      if (generic->parent()) {
        new_name = generic->parent().value()->name() + "_" + new_name;
      }
      new_g->SetName(new_name);
      comp->Add(new_g);
      (*rebinding)[generic] = new_g.get();
    }
  }
}

}  // namespace cerata
