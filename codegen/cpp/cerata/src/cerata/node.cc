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

#include "cerata/node.h"

#include <optional>
#include <utility>
#include <vector>
#include <memory>
#include <string>

#include "cerata/utils.h"
#include "cerata/edge.h"
#include "cerata/pool.h"
#include "cerata/graph.h"
#include "cerata/expression.h"

namespace cerata {

Node::Node(std::string name, Node::NodeID id, std::shared_ptr<Type> type)
    : Object(std::move(name), Object::NODE), node_id_(id), type_(std::move(type)) {}

std::string Node::ToString() const {
  return name();
}

Node *Node::Replace(Node *replacement) {
  // Iterate over all sourcing edges of the original node.
  for (const auto &e : this->sources()) {
    auto src = e->src();
    src->RemoveEdge(e);
    this->RemoveEdge(e);
    Connect(replacement, src);
  }
  // Iterate over all sinking edges of the original node.
  for (const auto &e : this->sinks()) {
    auto dst = e->src();
    dst->RemoveEdge(e);
    this->RemoveEdge(e);
    Connect(dst, replacement);
  }
  // Remove the node from its parent, if it has one.
  if (this->parent()) {
    this->parent().value()->Remove(this);
    this->parent().value()->Add(this->shared_from_this());
  }
  // Set array size node, if this is one.
  if (this->IsParameter()) {
    auto param = this->AsParameter();
    if (param->node_array_parent) {
      param->node_array_parent.value()->SetSize(replacement->shared_from_this());
    }
  }
  return replacement;
}

std::vector<Edge *> Node::edges() const {
  auto snk = sinks();
  auto src = sources();
  std::vector<Edge *> edges;
  edges.insert(edges.end(), snk.begin(), snk.end());
  edges.insert(edges.end(), src.begin(), src.end());
  return edges;
}

// Generate node casting convenience functions.
#ifndef NODE_CAST_IMPL_FACTORY
#define NODE_CAST_IMPL_FACTORY(NODENAME)                                                        \
NODENAME* Node::As##NODENAME() { auto result = dynamic_cast<NODENAME*>(this);                   \
  if (result != nullptr) {                                                                      \
    return result;                                                                              \
  } else {                                                                                      \
    CERATA_LOG(FATAL, "Node is not " + std::string(#NODENAME));                                 \
}}                                                                                              \
const NODENAME* Node::As##NODENAME() const { auto result = dynamic_cast<const NODENAME*>(this); \
  if (result != nullptr) {                                                                      \
    return result;                                                                              \
  } else {                                                                                      \
    CERATA_LOG(FATAL, "Node is not " + std::string(#NODENAME));                                 \
}}
#endif

NODE_CAST_IMPL_FACTORY(Port)
NODE_CAST_IMPL_FACTORY(Signal)
NODE_CAST_IMPL_FACTORY(Parameter)
NODE_CAST_IMPL_FACTORY(Literal)
NODE_CAST_IMPL_FACTORY(Expression)

#ifndef TYPE_STRINGIFICATION_FACTORY
#define TYPE_STRINGIFICATION_FACTORY(NODE_TYPE)             \
  template<>                                                \
  std::string ToString<NODE_TYPE>() { return #NODE_TYPE; }
#endif
TYPE_STRINGIFICATION_FACTORY(Port)
TYPE_STRINGIFICATION_FACTORY(Signal)
TYPE_STRINGIFICATION_FACTORY(Literal)
TYPE_STRINGIFICATION_FACTORY(Parameter)
TYPE_STRINGIFICATION_FACTORY(Expression)


bool MultiOutputNode::AddEdge(const std::shared_ptr<Edge> &edge) {
  bool success = false;
  // Check if this edge has a source
  if (edge->src()) {
    // Check if the source is this node
    if (edge->src() == this) {
      // Check if this node does not already contain this edge
      if (!Contains(outputs_, edge)) {
        // Add the edge to this node
        outputs_.push_back(edge);
        success = true;
      }
    }
  }
  return success;
}

bool MultiOutputNode::RemoveEdge(Edge *edge) {
  if (edge->src()) {
    if (edge->src() == this) {
      // This node sources the edge.
      for (auto i = outputs_.begin(); i < outputs_.end(); i++) {
        if (i->get() == edge) {
          outputs_.erase(i);
          return true;
        }
      }
    }
  }
  return false;
}

std::shared_ptr<Edge> MultiOutputNode::AddSink(Node *sink) {
  return Connect(sink, this);
}

std::optional<Edge *> NormalNode::input() const {
  if (input_ != nullptr) {
    return input_.get();
  }
  return {};
}

std::vector<Edge *> NormalNode::sources() const {
  if (input_ != nullptr) {
    return {input_.get()};
  } else {
    return {};
  }
}

bool NormalNode::AddEdge(const std::shared_ptr<Edge> &edge) {
  // first attempt to add the edge as an output.
  bool success = MultiOutputNode::AddEdge(edge);
  // If we couldn't add the edge as output, it must be input.
  if (!success) {
    // Check if the edge has a destination.
    if (edge->dst()) {
      // Check if this is the destination.
      if (edge->dst() == this) {
        // Set this node source to the edge.
        input_ = edge;
        success = true;
      }
    }
  }
  return success;
}

bool NormalNode::RemoveEdge(Edge *edge) {
  // First remove the edge from any outputs
  bool success = MultiOutputNode::RemoveEdge(edge);
  // Check if the edge is an input to this node
  if ((edge->dst() != nullptr) && !success) {
    if ((edge->dst() == this) && (input_.get() == edge)) {
      input_.reset();
      success = true;
    }
  }
  return success;
}

std::shared_ptr<Edge> NormalNode::AddSource(Node *source) {
  return Connect(this, source);
}

std::string ToString(Node::NodeID id) {
  switch (id) {
    case Node::NodeID::PORT:return "Port";
    case Node::NodeID::SIGNAL:return "Signal";
    case Node::NodeID::LITERAL:return "Literal";
    case Node::NodeID::PARAMETER:return "Parameter";
    case Node::NodeID::EXPRESSION:return "Expression";
  }
  throw std::runtime_error("Corrupted node type.");
}

Parameter::Parameter(std::string name, const std::shared_ptr<Type> &type, const std::shared_ptr<Node> &default_value)
    : NormalNode(std::move(name), Node::NodeID::PARAMETER, type) {
  Connect(this, default_value);
}

Node *Parameter::value() const {
  return input().value()->src();
}

void Parameter::Trace(std::vector<Node *> *out) const {
  if (input()) {
    out->push_back(input().value()->src());
    if (input().value()->src()->IsParameter()) {
      input().value()->src()->AsParameter()->Trace(out);
    }
  }
}

std::shared_ptr<Object> Parameter::Copy() const {
  auto result = parameter(name(), type()->shared_from_this(), value()->shared_from_this());
  result->meta = this->meta;
  return result;
}

std::shared_ptr<Parameter> parameter(const std::string &name,
                                     const std::shared_ptr<Type> &type,
                                     const std::shared_ptr<Node> &default_value) {
  auto val = default_value;
  if (val == nullptr) {
    val = intl(0);
  }
  auto p = new Parameter(name, type, val);
  return std::shared_ptr<Parameter>(p);
}

bool Parameter::RemoveEdge(Edge *edge) {
  if ((edge == input_.get()) && (!outputs_.empty())) {
    CERATA_LOG(FATAL,
               "Attempting to remove incoming edge from parameter, while parameter is still sourcing other nodes.");
  }
  return NormalNode::RemoveEdge(edge);
}

Signal::Signal(std::string name, std::shared_ptr<Type> type, std::shared_ptr<ClockDomain> domain)
    : NormalNode(std::move(name), Node::NodeID::SIGNAL, std::move(type)), Synchronous(std::move(domain)) {}

std::shared_ptr<Object> Signal::Copy() const {
  auto result = signal(this->name(), this->type_, this->domain_);
  result->meta = this->meta;
  return result;
}

std::string Signal::ToString() const {
  return name() + ":" + type()->name();
}

std::shared_ptr<Signal> signal(const std::string &name,
                               const std::shared_ptr<Type> &type,
                               const std::shared_ptr<ClockDomain> &domain) {
  auto ret = std::make_shared<Signal>(name, type, domain);
  return ret;
}

std::shared_ptr<Signal> signal(const std::shared_ptr<Type> &type,
                               const std::shared_ptr<ClockDomain> &domain) {
  auto ret = std::make_shared<Signal>(type->name() + "_signal", type, domain);
  return ret;
}

std::string Term::str(Term::Dir dir) {
  switch (dir) {
    case IN: return "in";
    case OUT: return "out";
  }
  return "corrupt";
}

Term::Dir Term::Invert(Term::Dir dir) {
  switch (dir) {
    case IN: return OUT;
    case OUT: return IN;
  }
  CERATA_LOG(FATAL, "Corrupted terminator direction.");
}

}  // namespace cerata
