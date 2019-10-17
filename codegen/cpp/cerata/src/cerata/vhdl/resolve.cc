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

#include "cerata/vhdl/resolve.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "cerata/logging.h"
#include "cerata/type.h"
#include "cerata/graph.h"
#include "cerata/edge.h"
#include "cerata/transform.h"
#include "cerata/vhdl/vhdl_types.h"
#include "cerata/vhdl/vhdl.h"

namespace cerata::vhdl {

static std::shared_ptr<ClockDomain> DomainOf(NodeArray *node_array) {
  std::shared_ptr<ClockDomain> result;
  auto base = node_array->base();
  if (base->IsSignal()) {
    result = std::dynamic_pointer_cast<Signal>(base)->domain();
  } else if (base->IsPort()) {
    result = std::dynamic_pointer_cast<Port>(base)->domain();
  } else {
    throw std::runtime_error("Base node is not a signal or port.");
  }
  return result;
}

/**
 * @brief Insert a signal based on a node and reconnect every edge.
 */
static void AttachSignalToNode(Component *comp,
                               NormalNode *node,
                               std::vector<Object *> *resolved,
                               std::unordered_map<Node *, Node *> *rebinding) {
  std::shared_ptr<Type> type = node->type()->shared_from_this();
  // Figure out if the type is a generic type.
  if (type->IsGeneric()) {
    // In that case, obtain the type generic nodes.
    auto generics = type->GetGenerics();
    // We need to copy over any type generic nodes that are not on the component yet.
    // Potentially produce new generics in the rebind map.
    for (const auto &g : generics) {
      RebindGeneric(comp, g, rebinding);
    }
    // Now we must rebind the type generics to the nodes on the component graph.
    // Copy the type and provide the rebind map.
    type = type->Copy(*rebinding);
  }

  // Get the clock domain of the node.
  auto domain = default_domain();
  if (node->IsSignal()) domain = node->AsSignal().domain();
  if (node->IsPort()) domain = node->AsPort().domain();

  // Get the name of the node.
  auto name = node->name();
  // Check if the node is on an instance, and prepend that.
  if (node->parent()) {
    if (node->parent().value()->IsInstance()) {
      name = node->parent().value()->name() + "_" + name;
    }
  }
  auto new_name = name;
  // Check if a node with this name already exists, generate a new name suffixed with a number if it does.
  int i = 0;
  while (comp->Has(new_name)) {
    i++;
    new_name = name + "_" + std::to_string(i);
  }

  // Create the new signal.
  auto new_signal = signal(new_name, type, domain);
  comp->Add(new_signal);
  resolved->push_back(new_signal.get());

  // Iterate over any existing edges that are sinked by the original node.
  for (auto &e : node->sinks()) {
    // Figure out the original destination of this edge.
    auto dst = e->dst();
    // Remove the edge from the original node and the destination node.
    node->RemoveEdge(e);
    dst->RemoveEdge(e);
    // Create a new edge, driving the destination with the new signal.
    Connect(dst, new_signal);
    Connect(new_signal, node);
  }

  // Iterate over any existing edges that source the original node.
  for (auto &e : node->sources()) {
    // Get the destination node.
    auto src = e->src();
    // Remove the original edge from the port array child node and source node.
    node->RemoveEdge(e);
    src->RemoveEdge(e);
    // Create a new edge, driving the new signal with the original sourc.
    Connect(new_signal, src);
    Connect(node, new_signal);
  }
}

/**
 * @brief Insert a signal array based on a node array and connect every node.
 */
static void AttachSignalArrayToNodeArray(Component *comp,
                                         NodeArray *array,
                                         std::vector<Object *> *resolved,
                                         std::unordered_map<Node *, Node *> *rebinding) {
  // Get the base node.
  auto base = array->base();

  std::shared_ptr<Type> type = base->type()->shared_from_this();
  // Figure out if the type is a generic type.
  if (type->IsGeneric()) {
    // In that case, obtain the type generic nodes.
    auto generics = base->type()->GetGenerics();
    // We need to copy over any type generic nodes that are not on the component yet.
    // Potentially produce new generics in the rebind map.
    for (const auto &g : generics) {
      RebindGeneric(comp, g, rebinding);
    }
    // Now we must rebind the type generics to the nodes on the component graph.
    // Copy the type and provide the rebind map.
    type = array->type()->Copy(*rebinding);
  }

  // The size parameter must potentially be "rebound" as well.
  RebindGeneric(comp, array->size(), rebinding);
  auto size = rebinding->at(array->size())->shared_from_this();

  // Get the clock domain of the array node
  auto domain = DomainOf(array);
  auto name = array->name();
  // Check if the array is on an instance, and prepend that.
  if (array->parent()) {
    if (array->parent().value()->IsInstance()) {
      name = array->parent().value()->name() + "_" + name;
    }
  }
  auto new_name = name;
  // Check if a node with this name already exists, generate a new name suffixed with a number if it does.
  int i = 0;
  while (comp->Has(new_name)) {
    i++;
    new_name = name + "_" + std::to_string(i);
  }

  // Create the new array.
  auto new_array = signal_array(new_name, type, size, domain);
  comp->Add(new_array);
  resolved->push_back(new_array.get());

  // Now iterate over all original nodes on the NodeArray and reconnect them.
  for (size_t n = 0; n < array->num_nodes(); n++) {
    // Create a new child node inside the array, but don't increment the size.
    // We've already (potentially) rebound the size from some other source and it should be set properly already.
    auto new_sig = new_array->Append(false);
    resolved->push_back(new_sig);

    bool has_sinks = false;
    bool has_sources = false;

    // Iterate over any existing edges that are sinked by the original array node.
    for (auto &e : array->node(n)->sinks()) {
      // Figure out the original destination.
      auto dst = e->dst();
      // Source the destination with the new signal.
      Connect(dst, new_sig);
      // Remove the original edge from the port array child node and destination node.
      array->node(n)->RemoveEdge(e);
      dst->RemoveEdge(e);
      has_sinks = true;
    }

    // Iterate over any existing edges that source the original array node.
    for (auto &e : array->node(n)->sources()) {
      // Get the destination node.
      auto src = e->src();
      Connect(new_sig, src);
      // Remove the original edge from the port array child node and source node.
      array->node(n)->RemoveEdge(e);
      src->RemoveEdge(e);
      has_sources = true;
    }

    // Source the new signal node with the original node, depending on whether it had any sinks or sources.
    if (has_sinks) {
      Connect(new_sig, array->node(n));
    }
    if (has_sources) {
      Connect(array->node(n), new_sig);
    }
  }
}

static void ResolvePorts(Component *comp,
                         Instance *inst,
                         std::vector<Object *> *resolved,
                         std::unordered_map<Node *, Node *> *rebinding) {
  for (const auto &port : inst->GetAll<Port>()) {
    AttachSignalToNode(comp, port, resolved, rebinding);
  }
}

/**
 * @brief Resolve VHDL-specific limitations in a Cerata graph related to ports.
 * @param comp      The component to resolve the limitations for.
 * @param inst      The instance to resolve it for.
 * @param resolved  A vector to append resolved objects to.
 * @param rebinding A map with type generic node rebindings.
 */
static void ResolvePortArrays(Component *comp,
                              Instance *inst,
                              std::vector<Object *> *resolved,
                              std::unordered_map<Node *, Node *> *rebinding) {
  // There is something utterly annoying in VHDL; range expressions must be "locally static" in port map
  // associativity lists on the left hand side. This means we can't use any type generic nodes.
  // Thanks, VHDL.

  // To solve this, we're just going to insert a signal array for every port array.
  for (const auto &pa : inst->GetAll<PortArray>()) {
    AttachSignalArrayToNodeArray(comp, pa, resolved, rebinding);
  }
}

Component *Resolve::SignalizePorts(Component *comp) {
  // Remember which nodes we've already resolved.
  std::vector<Object *> resolved;
  // We are potentially going to make a bunch of copies of type generic nodes and array sizes.
  // Remember which ones we did already and where we left their copy through a map.
  std::unordered_map<Node *, Node *> rebinding;

  CERATA_LOG(DEBUG, "VHDL: Resolving a whole bunch of ridiculous VHDL restrictions.");
  auto children = comp->children();
  for (const auto &inst : children) {
    ResolvePorts(comp, inst, &resolved, &rebinding);
    ResolvePortArrays(comp, inst, &resolved, &rebinding);
  }
  CERATA_LOG(DEBUG, "VHDL: Resolved " + std::to_string(resolved.size()) + " port-to-port connections ...");
  CERATA_LOG(DEBUG, "VHDL: Rebound " + std::to_string(rebinding.size()) + " nodes ...");
  return comp;
}

static bool IsExpandType(const Type *t, const std::string &str = "") {
  if (t->meta.count(meta::EXPAND_TYPE) > 0) {
    if (str.empty()) {
      return true;
    } else if (t->meta.at(meta::EXPAND_TYPE) == str) {
      return true;
    }
  }
  return false;
}

static bool HasStream(const std::vector<FlatType> &fts) {
  for (const auto &ft : fts) {
    if (ft.type_->Is(Type::STREAM)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Convert a type into a record type with valid and ready fields.
 *
 * This function may cause old stream types to be deleted. Any pointers to types might be invalidated.
 *
 * @param flattened_type  The flattened type to add valid and ready signals to.
 */
static void ExpandStreamType(Type *type) {
  // If the type was not expanded yet.
  if (type->meta.count(meta::WAS_EXPANDED) == 0) {
    // Expand it. First remember that it was expanded.
    type->meta[meta::WAS_EXPANDED] = "true";
    // Flatten the type.
    auto flattened_types = Flatten(type);
    // Check if it has any streams.
    if (HasStream(flattened_types)) {
      CERATA_LOG(DEBUG, "VHDL:   Expanding type " + type->ToString());
      // Iterate over the flattened type
      for (auto &flat : flattened_types) {
        // If we encounter a stream type
        if (flat.type_->Is(Type::STREAM)) {
          auto *stream_type = dynamic_cast<Stream *>(flat.type_);
          // Check if we didn't expand the type already.
          if (stream_type->meta.count(meta::EXPAND_TYPE) == 0) {
            // Create a new record to hold valid, ready and the original element type
            auto new_elem_type = record(stream_type->name() + "_vr");
            // Mark the record
            new_elem_type->meta[meta::EXPAND_TYPE] = "record";
            // Add valid, ready and original type to the record and set the new element type
            new_elem_type->AddField(field("valid", valid()));
            new_elem_type->AddField(field("ready", ready(), true));
            new_elem_type->AddField(field(stream_type->element_name(), stream_type->element_type()));
            stream_type->SetElementType(new_elem_type);
            // Mark the stream itself to remember we've expanded it.
            stream_type->meta[meta::EXPAND_TYPE] = "stream";
          }
        }
      }
    }
  }
}

static void ExpandMappers(Type *type, const std::vector<std::shared_ptr<TypeMapper>> &mappers) {
  // TODO(johanpel): Generalize this type expansion functionality and add it to Cerata.
  // Iterate over all previously existing mappers.
  for (const auto &mapper : mappers) {
    // Skip mappers that don't contain streams.
    if (!HasStream(mapper->flat_a()) && !HasStream(mapper->flat_b())) {
      continue;
    }
    // Skip mappers that were already expanded.
    if (mapper->meta.count(meta::WAS_EXPANDED) > 0) {
      continue;
    }

    // Get a copy of the old matrix.
    auto old_matrix = mapper->map_matrix();
    // Create a new mapper
    auto new_mapper = TypeMapper::Make(type, mapper->b());
    // Get the properly sized matrix
    auto new_matrix = new_mapper->map_matrix();
    // Get the flattened type
    auto flat_a = new_mapper->flat_a();
    auto flat_b = new_mapper->flat_b();

    int64_t old_row = 0;
    int64_t old_col = 0;
    int64_t new_row = 0;
    int64_t new_col = 0;

    while (new_row < new_matrix.height()) {
      auto at = flat_a[new_row].type_;
      while (new_col < new_matrix.width()) {
        auto bt = flat_b[new_col].type_;
        // Figure out if we're dealing with a matching, expanded type on both sides.
        if (IsExpandType(at, "stream") && IsExpandType(bt, "stream")) {
          new_matrix(new_row, new_col) = old_matrix(old_row, old_col);
          new_col += 4;  // Skip over record, valid and ready
          old_col += 1;
        } else if (IsExpandType(at, "record") && IsExpandType(bt, "record")) {
          new_matrix(new_row, new_col) = old_matrix(old_row, old_col);
          new_col += 3;  // Skip over valid and ready
          old_col += 1;
        } else if (IsExpandType(at, "valid") && IsExpandType(bt, "valid")) {
          new_matrix(new_row, new_col) = old_matrix(old_row, old_col);
          new_col += 2;  // Skip over ready
          old_col += 1;
        } else if (IsExpandType(at, "ready") && IsExpandType(bt, "ready")) {
          new_matrix(new_row, new_col) = old_matrix(old_row, old_col);
          new_col += 1;
        } else {
          // We're not dealing with a *matching* expanded type. However, if the A side was expanded and the type is
          // not matching, we shouldn't make a copy. That will happen later on, on another row.
          if (!IsExpandType(at)) {
            new_matrix(new_row, new_col) = old_matrix(old_row, old_col);
          }
          new_col += 1;
        }
        // If this column was not expanded, or if it was the last expanded column, move to the next column in the old
        // matrix.
        if (!IsExpandType(bt) || IsExpandType(bt, "ready")) {
          old_col += 1;
        }
      }
      old_col = 0;
      new_col = 0;

      // Only move to the next row of the old matrix when we see the last expanded type or if it's not an expanded
      // type at all.
      if (!IsExpandType(at) || IsExpandType(at, "ready")) {
        old_row += 1;
      }
      new_row += 1;
    }

    // Set the mapping matrix of the new mapper to the new matrix
    new_mapper->SetMappingMatrix(new_matrix);
    new_mapper->meta[meta::WAS_EXPANDED] = "true";

    // Add the mapper to the type
    type->AddMapper(new_mapper);
  }
}

Component *Resolve::ExpandStreams(Component *comp) {
  CERATA_LOG(DEBUG, "VHDL: Materialize stream abstraction...");
  // List of types.
  std::vector<Type *> types;
  // List of mappers. Takes ownership of existing mappers before expansion.
  std::unordered_map<Type *, std::vector<std::shared_ptr<TypeMapper>>> mappers;

  // Populate the list.
  GetAllTypes(comp, &types, true);

  // First expand all the types.
  for (const auto &type : types) {
    auto type_mappers = type->mappers();
    if (!type_mappers.empty()) {
      mappers[type] = type_mappers;  // Remember and keep the old mappers for ExpandMappers
    }
    ExpandStreamType(type);
  }
  // Next, expand the mappers that were present for all types.
  for (const auto &type : types) {
    if (mappers.count(type) > 0) {
      ExpandMappers(type, mappers[type]);
    }
  }

  return comp;
}

}  // namespace cerata::vhdl
