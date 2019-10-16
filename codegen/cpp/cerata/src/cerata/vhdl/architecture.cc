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

#include "cerata/vhdl/architecture.h"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

#include "cerata/node.h"
#include "cerata/expression.h"
#include "cerata/graph.h"
#include "cerata/vhdl/block.h"
#include "cerata/vhdl/declaration.h"
#include "cerata/vhdl/instantiation.h"
#include "cerata/vhdl/vhdl.h"

namespace cerata::vhdl {

MultiBlock Arch::Generate(const Component &comp) {
  MultiBlock ret;
  Line header_line;
  header_line << "architecture Implementation of " + comp.name() + " is";
  ret << header_line;

  // Component declarations
  auto components_used = comp.GetAllUniqueComponents();
  for (const auto &c : components_used) {
    // Check for metadata that this component is not marked primitive
    // In this case, a library package has been added at the top of the design file.
    if ((c->meta().count(metakeys::PRIMITIVE) == 0) || (c->meta().at(metakeys::PRIMITIVE) != "true")) {
      // TODO(johanpel): generate packages with component declarations
      auto comp_decl = Decl::Generate(*c);
      ret << comp_decl;
      ret << Line();
    }
  }

  // Signal declarations
  auto signals = comp.GetAll<Signal>();
  for (const auto &s : signals) {
    auto signal_decl = Decl::Generate(*s, 1);
    ret << signal_decl;
    if (signal_decl.lines.size() > 1) {
      ret << Line();
    }
    if (s == signals.back()) {
      ret << Line();
    }
  }

  // Signal array declarations
  auto signal_arrays = comp.GetAll<SignalArray>();
  for (const auto &s : signal_arrays) {
    auto signal_array_decl = Decl::Generate(*s, 1);
    ret << signal_array_decl;
    if (signal_array_decl.lines.size() > 1) {
      ret << Line();
    }
    if (s == signal_arrays.back()) {
      ret << Line();
    }
  }

  Line header_end;
  header_end << "begin";
  ret << header_end;

  // Port connections
  auto ports = comp.GetAll<Port>();
  for (const auto &p : ports) {
    auto port_assign = Generate(comp, *p, 1);
    ret << port_assign;
    if (port_assign.lines.size() > 1) {
      ret << Line();
    }
    if (p == ports.back()) {
      ret << Line();
    }
  }

  // Signal connections
  for (const auto &s : signals) {
    auto signal_assign = Generate(comp, *s, 1);
    ret << signal_assign;
    if (signal_assign.lines.size() > 1) {
      ret << Line();
    }
    if (s == signals.back()) {
      ret << Line();
    }
  }

  // Signal array connections
  for (const auto &s : signal_arrays) {
    auto signal_assign = Generate(comp, *s, 1);
    ret << signal_assign;
    if (signal_assign.lines.size() > 1) {
      ret << Line();
    }
    if (s == signal_arrays.back()) {
      ret << Line();
    }
  }

  // Component instantiations
  auto instances = comp.children();
  for (const auto &i : instances) {
    auto inst_decl = Inst::Generate(*i);
    ret << inst_decl;
    ret << Line();
  }

  Line footer;
  footer << "end architecture;";
  ret << footer;

  return ret;
}

// TODO(johanpel): make this something less arcane
static Block GenerateMappingPair(const MappingPair &p,
                                 size_t ia,
                                 const std::shared_ptr<Node> &offset_a,
                                 size_t ib,
                                 const std::shared_ptr<Node> &offset_b,
                                 const std::string &lh_prefix,
                                 const std::string &rh_prefix,
                                 bool a_is_array,
                                 bool b_is_array) {
  Block ret;

  std::shared_ptr<Node> next_offset_a;
  std::shared_ptr<Node> next_offset_b;

  auto a_width = p.flat_type_a(ia).type_->width();
  auto b_width = p.flat_type_b(ib).type_->width();

  next_offset_a = (offset_a + (b_width ? b_width.value() : rintl(0)));
  next_offset_b = (offset_b + (a_width ? a_width.value() : rintl(0)));

  if (p.flat_type_a(0).type_->Is(Type::STREAM)) {
    // Don't output anything for the abstract stream type.
  } else if (p.flat_type_a(0).type_->Is(Type::RECORD)) {
    // Don't output anything for the abstract record type.
  } else {
    std::string a;
    std::string b;
    a = p.flat_type_a(ia).name(NamePart(lh_prefix, true));
    // if right side is concatenated onto the left side
    // or the left side is an array (right is also concatenated onto the left side)
    if ((p.num_b() > 1) || a_is_array) {
      if (p.flat_type_a(ia).type_->Is(Type::BIT)) {
        a += "(" + offset_a->ToString() + ")";
      } else {
        a += "(" + (next_offset_a - 1)->ToString();
        a += " downto " + offset_a->ToString() + ")";
      }
    }
    b = p.flat_type_b(ib).name(NamePart(rh_prefix, true));
    if ((p.num_a() > 1) || b_is_array) {
      if (p.flat_type_b(ib).type_->Is(Type::BIT)) {
        b += "(" + offset_b->ToString() + ")";
      } else {
        b += "(" + (next_offset_b - 1)->ToString();
        b += " downto " + offset_b->ToString() + ")";
      }
    }
    Line l;
    if (p.flat_type_a(ia).invert_) {
      l << b << " <= " << a;
    } else {
      l << a << " <= " << b;
    }
    ret << l;
  }

  return ret;
}

static Block GenerateAssignmentPair(std::vector<MappingPair> pairs, const Node &a, const Node &b) {
  Block ret;
  // Sort the pair in order of appearance on the flatmap
  std::sort(pairs.begin(), pairs.end(), [](const MappingPair &x, const MappingPair &y) -> bool {
    return x.index_a(0) < y.index_a(0);
  });
  bool a_array = false;
  bool b_array = false;
  size_t a_idx = 0;
  size_t b_idx = 0;
  // Figure out if these nodes are on NodeArrays and what their index is
  if (a.array()) {
    a_array = true;
    a_idx = a.array().value()->IndexOf(a);
  }
  if (b.array()) {
    b_array = true;
    b_idx = b.array().value()->IndexOf(b);
  }
  if (a.type()->meta.count(metakeys::FORCE_VECTOR) > 0) {
    a_array = true;
  }
  if (b.type()->meta.count(metakeys::FORCE_VECTOR) > 0) {
    b_array = true;
  }
  // Loop over all pairs
  for (const auto &pair : pairs) {
    // Offset on the right side
    std::shared_ptr<Node> b_offset = pair.width_a(intl(1)) * intl(b_idx);
    // Loop over everything on the left side
    for (int64_t ia = 0; ia < pair.num_a(); ia++) {
      // Get the width of the left side.
      auto a_width = pair.flat_type_a(ia).type_->width();
      // Offset on the left side.
      std::shared_ptr<Node> a_offset = pair.width_b(intl(1)) * intl(a_idx);
      for (int64_t ib = 0; ib < pair.num_b(); ib++) {
        // Get the width of the right side.
        auto b_width = pair.flat_type_b(ib).type_->width();
        // Generate the mapping pair with given offsets
        auto mpblock = GenerateMappingPair(pair, ia, a_offset, ib, b_offset, a.name(), b.name(), a_array, b_array);
        ret << mpblock;
        // Increase the offset on the left side.
        a_offset = a_offset + (b_width ? b_width.value() : rintl(1));
      }
      // Increase the offset on the right side.
      b_offset = b_offset + (a_width ? a_width.value() : rintl(1));
    }
  }
  return ret;
}

static Block GenerateNodeAssignment(const Node &dst, const Node &src) {
  Block result;
  // Check if a type mapping exists
  auto optional_type_mapper = dst.type()->GetMapper(src.type());
  if (optional_type_mapper) {
    auto type_mapper = optional_type_mapper.value();
    // Obtain the unique mapping pairs for this mapping
    auto pairs = type_mapper->GetUniqueMappingPairs();
    // Generate the mapping for this port-node pair.
    result << GenerateAssignmentPair(pairs, dst, src);
    result << ";";
  } else {
    CERATA_LOG(FATAL, "No type mapping available for: Node[" + dst.name() + ": " + dst.type()->name()
        + "] from Other[" + src.name() + " : " + src.type()->name() + "]");
  }
  return result;
}

Block Arch::Generate(const Component &comp, const Port &port, int indent) {
  Block ret(indent);
  // We need to assign this port. If we properly resolved VHDL issues, and didn't do weird stuff like loop a
  // port back up, the only thing that could drive this port is a signal. We're going to sanity check this anyway.

  // Check if the port node is sourced at all.
  if (port.sources().empty()) {
    return ret;
  }

  // Generate the assignments for every source.
  for (const auto &edge : port.sources()) {
    if (edge->src()->IsPort()) {
      CERATA_LOG(FATAL, "Component port is unexpectedly sourced by another port.");
    }
    ret << GenerateNodeAssignment(*edge->dst(), *edge->src());
  }
  return ret;
}

Block Arch::Generate(const Component &comp, const Signal &sig, int indent) {
  Block ret(indent);
  // We need to assign this signal when it is sourced by a node that is not an instance port.
  // If the source is, then this is done in the associativity list of the port map of the component instantiation.

  // Check if the signal node is sourced at all.
  if (sig.sources().empty()) {
    return ret;
  }

  for (const auto &edge : sig.sources()) {
    Block b;
    Node *src = edge->src();
    Node *dst = edge->dst();
    // Check if the source is a port, has a parent and if its parent is an instance.
    // Then, we can skip the assignment, it is done elsewhere.
    if (src->IsPort() && src->parent() && src->parent().value()->IsInstance()) {
      continue;
    }
    // Get the node on the other side of the connection
    auto other = src;
    // Get the other type.
    auto other_type = other->type();
    // Check if a type mapping exists
    auto optional_type_mapper = dst->type()->GetMapper(other_type);
    if (optional_type_mapper) {
      auto type_mapper = optional_type_mapper.value();
      // Obtain the unique mapping pairs for this mapping
      auto pairs = type_mapper->GetUniqueMappingPairs();
      // Generate the mapping for this port-node pair.
      b << GenerateAssignmentPair(pairs, *dst, *other);
      b << ";";
      ret << b;
    } else {
      CERATA_LOG(FATAL, "No type mapping available for: Signal[" + dst->name() + ": " + dst->type()->name()
          + "] to Other[" + other->name() + " : " + other->type()->name() + "]");
    }
  }
  return ret;
}

Block Arch::Generate(const Component &comp, const SignalArray &sig_array, int indent) {
  Block ret(indent);
  // Go over each node in the array
  for (const auto &node : sig_array.nodes()) {
    if (node->IsSignal()) {
      const auto &sig = dynamic_cast<const Signal &>(*node);
      ret << Generate(comp, sig, indent);
    } else {
      throw std::runtime_error("Signal Array contains non-signal node.");
    }
  }
  return ret.Sort('(');
}

}  // namespace cerata::vhdl
