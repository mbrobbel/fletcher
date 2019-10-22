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

#include "cerata/array.h"

#include <optional>
#include <utility>
#include <vector>
#include <memory>
#include <string>

#include "cerata/edge.h"
#include "cerata/node.h"
#include "cerata/expression.h"
#include "cerata/parameter.h"
#include "cerata/pool.h"
#include "cerata/graph.h"

namespace cerata {

/*
static std::shared_ptr<Node> IncrementNode(Node *node) {
  if (node->IsLiteral() || node->IsExpression()) {
    return node->shared_from_this() + 1;
  } else if (node->IsParameter()) {
    // If the node is a parameter, we should be able to trace its source back to a literal node.
    // We then replace the last parameter node in the trace by a copy and source the copy from an incremented literal.
    auto param = dynamic_cast<Parameter *>(node);
    // Initialize the trace with the parameter node.
    std::vector<Object *> obj_trace{param};
    param->AppendReferences(&obj_trace);
    // Parameter nodes can only reference nodes.
    auto node_trace = As<Node>(obj_trace);
    // Sanity check the trace.
    if (!node_trace.back()->IsLiteral()) {
      CERATA_LOG(FATAL, "Parameter node " + param->ToString() + " not (indirectly) sourced by literal.");
    }
    // The second-last node is of importance, because this is the final parameter node.
    auto second_last = node_trace[node_trace.size() - 2];
    auto incremented = node_trace.back()->shared_from_this() + 1;
    // Source the second last node with whatever literal was at the end of the trace, plus one.
    Connect(second_last, incremented);
    return node->shared_from_this();
  } else {
    CERATA_LOG(FATAL, "Can only increment literal, expression or parameter size node " + node->ToString());
  }
}
 */

void NodeArray::SetSize(const std::shared_ptr<Node> &size) {
  if (!(size->IsLiteral() || size->IsParameter() || size->IsExpression())) {
    CERATA_LOG(FATAL, "NodeArray size node must be literal, parameter or expression.");
  }
  if (size->IsParameter()) {
    auto param = size->AsParameter();
    if (param->node_array_parent) {
      auto na = size->AsParameter()->node_array_parent.value();
      if (na != this) {
        CERATA_LOG(FATAL, "NodeArray size can only be used by a single NodeArray.");
      }
    }
    param->node_array_parent = this;
  }
  size_ = size;
}

void NodeArray::IncrementSize() {
  SetSize(size_ + 1);
}

Node *NodeArray::Append(bool increment_size) {
  // Create a new copy of the base node.
  auto new_node = std::dynamic_pointer_cast<Node>(base_->Copy());
  if (parent()) {
    new_node->SetParent(*parent());
  }
  new_node->SetArray(this);
  nodes_.push_back(new_node);

  // Increment this NodeArray's size node.
  if (increment_size) {
    IncrementSize();
  }

  // Return the new node.
  return new_node.get();
}

Node *NodeArray::node(size_t i) const {
  if (i < nodes_.size()) {
    return nodes_[i].get();
  } else {
    CERATA_LOG(FATAL, "Index " + std::to_string(i) + " out of bounds for node " + ToString());
  }
}

std::shared_ptr<Object> NodeArray::Copy() const {
  auto p = parent();
  auto ret = std::make_shared<NodeArray>(name(), node_id_, base_, std::dynamic_pointer_cast<Node>(size()->Copy()));
  if (p) {
    ret->SetParent(*p);
  }
  return ret;
}

void NodeArray::SetParent(Graph *parent) {
  Object::SetParent(parent);
  base_->SetParent(parent);
  for (const auto &e : nodes_) {
    e->SetParent(parent);
  }
}

size_t NodeArray::IndexOf(const Node &n) const {
  for (size_t i = 0; i < nodes_.size(); i++) {
    if (nodes_[i].get() == &n) {
      return i;
    }
  }
  CERATA_LOG(FATAL, "Node " + n.ToString() + " is not element of " + this->ToString());
}

void NodeArray::SetType(const std::shared_ptr<Type> &type) {
  base_->SetType(type);
  for (auto &n : nodes_) {
    n->SetType(type);
  }
}

NodeArray::NodeArray(std::string name, Node::NodeID id, std::shared_ptr<Node> base, const std::shared_ptr<Node> &size)
    : Object(std::move(name), Object::ARRAY), node_id_(id), base_(std::move(base)) {
  base_->SetArray(this);
  SetSize(size);
}

NodeArray *NodeArray::CopyOnto(Graph *dst, const std::string &name, NodeMap *rebinding) {
  // Make a normal copy (that does not rebind the type generics).
  auto result = std::dynamic_pointer_cast<NodeArray>(Copy());
  // Set the name.
  result->SetName(name);

  // Get the references of the base node.
  std::vector<Object *> obj_refs;
  obj_refs.push_back(base_.get());
  base_->AppendReferences(&obj_refs);

  // A node array can only reference nodes, so we should be able to safely cast to a vec of node ptrs.
  auto refs = As<Node>(obj_refs);

  // Check which references we need to rebind.
  for (const auto &r : refs) {
    if (rebinding->count(r) > 0) {
      continue;
    } else if (dst->Has(g->name())) {
      // There might already be a node on the graph with that name. Implicitly use that node.
      (*rebinding)[g] = dst->Get<Node>(g->name());
    } else if (!g->IsLiteral()) {
      // Otherwise, if the node is not a literal, which doesn't have to be on the graph, make a copy of the generic
      // onto the graph.
      g->CopyOnto(dst, g->name(), rebinding);
    }
  }
  // Make a copy of the type, rebinding the generic nodes.
  auto rebound_type = result->type()->Copy(*rebinding);
  // Set the type of the new node to this new type.
  result->SetType(rebound_type);


  // Append this node to the rebind map.
  (*rebinding)[this] = result.get();

  // It should now be possible to add the copy onto the graph.
  dst->Add(result);

  return result.get();
}

PortArray::PortArray(const std::shared_ptr<Port> &base,
                     const std::shared_ptr<Node> &size,
                     Term::Dir dir) :
    NodeArray(base->name(), Node::NodeID::PORT, base, size), Term(base->dir()) {}

std::shared_ptr<PortArray> port_array(const std::string &name,
                                      const std::shared_ptr<Type> &type,
                                      const std::shared_ptr<Node> &size,
                                      Port::Dir dir,
                                      const std::shared_ptr<ClockDomain> &domain) {
  auto base_node = port(name, type, dir, domain);
  auto *port_array = new PortArray(base_node, size, dir);
  return std::shared_ptr<PortArray>(port_array);
}

std::shared_ptr<PortArray> port_array(const std::shared_ptr<Port> &base_node,
                                      const std::shared_ptr<Node> &size) {
  auto *port_array = new PortArray(base_node, size, base_node->dir());
  return std::shared_ptr<PortArray>(port_array);
}

std::shared_ptr<Object> PortArray::Copy() const {
  // Make a copy of the size node.
  auto size_copy = std::dynamic_pointer_cast<Node>(size()->Copy());
  // Cast the base node pointer to a port pointer
  auto base_as_port = std::dynamic_pointer_cast<Port>(base_);
  // Create the new PortArray using the new nodes.
  auto *port_array = new PortArray(base_as_port, size_copy, dir());
  // Return the resulting object.
  return std::shared_ptr<PortArray>(port_array);
}

std::shared_ptr<SignalArray> signal_array(const std::string &name,
                                          const std::shared_ptr<Type> &type,
                                          std::shared_ptr<Node> size,
                                          const std::shared_ptr<ClockDomain> &domain) {
  auto base_node = signal(name, type, domain);
  auto *sig_array = new SignalArray(base_node, std::move(size));
  return std::shared_ptr<SignalArray>(sig_array);
}

}  // namespace cerata
