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

#include "fletchgen/recordbatch.h"

#include <cerata/api.h>
#include <memory>
#include <vector>
#include <utility>

#include "fletchgen/array.h"
#include "fletchgen/bus.h"
#include "fletchgen/schema.h"

namespace fletchgen {

using cerata::Instance;
using cerata::Component;
using cerata::Port;
using cerata::Literal;
using cerata::intl;
using cerata::Term;

RecordBatch::RecordBatch(const std::string &name,
                         const std::shared_ptr<FletcherSchema> &fletcher_schema,
                         fletcher::RecordBatchDescription batch_desc)
    : Component(name),
      fletcher_schema_(fletcher_schema),
      mode_(fletcher_schema->mode()),
      batch_desc_(std::move(batch_desc)) {
  // Get Arrow Schema
  auto as = fletcher_schema_->arrow_schema();

  // Add default parameters
  Add(bus_addr_width());
  Add(bus_data_width());
  Add(bus_burst_step_len());
  Add(bus_burst_max_len());
  if (mode_ == Mode::WRITE) {
    Add(bus_strobe_width());
  }
  Add(bus_len_width());

  // Add default port nodes
  Add(port("bcd", cr(), Port::Dir::IN, bus_cd()));
  Add(port("kcd", cr(), Port::Dir::IN, kernel_cd()));

  // Add and connect all array readers and resulting ports
  AddArrays(fletcher_schema);
}

void RecordBatch::AddArrays(const std::shared_ptr<FletcherSchema> &fletcher_schema) {
  // Iterate over all fields and add ArrayReader/Writer data and control ports.
  for (const auto &field : fletcher_schema->arrow_schema()->fields()) {
    // Check if we must ignore the field
    if (fletcher::GetBoolMeta(*field, fletcher::meta::IGNORE, false)) {
      FLETCHER_LOG(DEBUG, "Ignoring field " + field->name());
    } else {
      FLETCHER_LOG(DEBUG, "Instantiating Array" << (mode_ == Mode::READ ? "Reader" : "Writer")
                                                << " for schema: " << fletcher_schema->name()
                                                << " : " << field->name());
      // Generate a warning for Writers as they are still experimental.
      if (mode_ == Mode::WRITE) {
        FLETCHER_LOG(WARNING, "ArrayWriter implementation is highly experimental. Use with caution! Features that are "
                              "not implemented include:\n"
                              "  - dvalid bit is ignored (so you cannot supply handshakes on the values stream for "
                              "empty lists or use empty handshakes to close streams)\n"
                              "  - lists of primitives (e.g. strings) values stream last signal must signal the last "
                              "value for all lists, not single lists in the Arrow Array).\n"
                              "  - clock domain crossings.");
      }

      // Convert to and add an Arrow port. We must invert it because it is an output of the RecordBatch.
      auto kernel_arrow_port = arrow_port(fletcher_schema, field, true, kernel_cd());
      auto kernel_arrow_type = kernel_arrow_port->type();
      Add(kernel_arrow_port);

      // Add a command port for the ArrayReader/Writer
      auto cmd = command_port(fletcher_schema, field, true, kernel_cd());
      Add(cmd);

      // Add an unlock port for the ArrayReader/Writer
      auto unl = unlock_port(fletcher_schema, field, kernel_cd());
      Add(unl);

      // Instantiate an ArrayReader or Writer
      auto a = AddInstanceOf(array(mode_), field->name() + "_inst");
      array_instances_.push_back(a);

      // Generate a configuration string for the ArrayReader
      auto cfg_node = a->GetNode(Node::NodeID::PARAMETER, "CFG");
      cfg_node <<= cerata::strl(GenerateConfigString(*field));  // Set the configuration string for this field

      // Set the parameters.
      Connect(a->par(bus_addr_width()), par(bus_addr_width()));
      Connect(a->par(bus_data_width()), par(bus_data_width()));
      if (mode_ == Mode::WRITE) {
        Connect(a->par(bus_strobe_width()), par(bus_strobe_width()));
      }
      Connect(a->par(bus_len_width()), par(bus_len_width()));
      Connect(a->par(bus_burst_step_len()), par(bus_burst_step_len()));
      Connect(a->par(bus_burst_max_len()), par(bus_burst_max_len()));

      // Drive the clocks and resets
      Connect(a->prt("kcd"), prt("kcd"));
      Connect(a->prt("bcd"), prt("bcd"));

      // Drive the RecordBatch Arrow data port with the ArrayReader/Writer data port, or vice versa
      if (mode_ == Mode::READ) {
        auto array_data_port = a->prt("out");
        auto array_data_type = array_data_port->type();
        // Create a mapper between the Arrow port and the Array data port
        auto mapper = GetStreamTypeMapper(kernel_arrow_type, array_data_type);
        kernel_arrow_type->AddMapper(mapper);
        // Connect the ports
        kernel_arrow_port <<= array_data_port;
      } else {
        auto array_data_port = a->prt("in");
        auto array_data_type = array_data_port->type();
        // Create a mapper between the Arrow port and the Array data port
        auto mapper = GetStreamTypeMapper(kernel_arrow_type, array_data_type);
        kernel_arrow_type->AddMapper(mapper);
        // Connect the ports
        array_data_port <<= kernel_arrow_port;
      }

      // Drive the ArrayReader command port from the top-level command port
      a->prt("cmd") <<= cmd;
      // Drive the unlock port with the ArrayReader/Writer
      unl <<= a->prt("unl");

      // Copy over the ArrayReader/Writer's bus channels
      auto bus = std::dynamic_pointer_cast<BusPort>(a->prt("bus")->Copy());
      // Give the new bus port a unique name
      // TODO(johanpel): move the bus renaming to the Mantle level
      bus->SetName(fletcher_schema->name() + "_" + field->name() + "_" + bus->name());
      Add(bus);  // Add them to the RecordBatch
      bus_ports_.push_back(bus);  // Remember the port
      bus <<= a->prt("bus");  // Connect them to the ArrayReader/Writer
    }
  }
}

std::shared_ptr<FieldPort> RecordBatch::GetArrowPort(const arrow::Field &field) const {
  for (const auto &n : objects_) {
    auto ap = std::dynamic_pointer_cast<FieldPort>(n);
    if (ap != nullptr) {
      if ((ap->function_ == FieldPort::ARROW) && (ap->field_->Equals(field))) {
        return ap;
      }
    }
  }
  throw std::runtime_error("Field " + field.name() + " did not generate an ArrowPort for Core " + name() + ".");
}

std::vector<std::shared_ptr<FieldPort>>
RecordBatch::GetFieldPorts(const std::optional<FieldPort::Function> &function) const {
  std::vector<std::shared_ptr<FieldPort>> result;
  for (const auto &n : objects_) {
    auto ap = std::dynamic_pointer_cast<FieldPort>(n);
    if (ap != nullptr) {
      if ((function && (ap->function_ == *function)) || !function) {
        result.push_back(ap);
      }
    }
  }
  return result;
}

std::shared_ptr<RecordBatch> recordbatch(const std::string &name,
                                         const std::shared_ptr<FletcherSchema> &fletcher_schema,
                                         const fletcher::RecordBatchDescription &batch_desc) {
  auto rb = new RecordBatch(name, fletcher_schema, batch_desc);
  auto shared_rb = std::shared_ptr<RecordBatch>(rb);
  cerata::default_component_pool()->Add(shared_rb);
  return shared_rb;
}

std::shared_ptr<FieldPort> arrow_port(const std::shared_ptr<FletcherSchema> &fletcher_schema,
                                      const std::shared_ptr<arrow::Field> &field,
                                      bool invert,
                                      const std::shared_ptr<ClockDomain> &domain) {
  Port::Dir dir;
  if (invert) {
    dir = Term::Invert(mode2dir(fletcher_schema->mode()));
  } else {
    dir = mode2dir(fletcher_schema->mode());
  }
  // Check if the Arrow data stream should be profiled. This is disabled by default but can be conveyed through
  // the schema.
  bool profile = fletcher::GetBoolMeta(*field, fletcher::meta::PROFILE, false);
  return std::make_shared<FieldPort>(fletcher_schema->name() + "_" + field->name(),
                                     FieldPort::ARROW,
                                     field,
                                     fletcher_schema,
                                     GetStreamType(*field, fletcher_schema->mode()),
                                     dir,
                                     domain,
                                     profile);
}

std::shared_ptr<FieldPort> command_port(const std::shared_ptr<FletcherSchema> &fletcher_schema,
                                        const std::shared_ptr<arrow::Field> &field,
                                        bool ctrl,
                                        const std::shared_ptr<ClockDomain> &domain) {
  std::shared_ptr<cerata::Type> type;
  if (ctrl) {
    type = cmd(tag_width(*field), ctrl_width(*field));
  } else {
    type = cmd(tag_width(*field));
  }
  return std::make_shared<FieldPort>(fletcher_schema->name() + "_" + field->name() + "_cmd",
                                     FieldPort::COMMAND, field, fletcher_schema, type, Port::Dir::IN, domain, false);
}

std::shared_ptr<FieldPort> unlock_port(const std::shared_ptr<FletcherSchema> &fletcher_schema,
                                       const std::shared_ptr<arrow::Field> &field,
                                       const std::shared_ptr<ClockDomain> &domain) {
  return std::make_shared<FieldPort>(fletcher_schema->name() + "_" + field->name() + "_unl",
                                     FieldPort::UNLOCK,
                                     field,
                                     fletcher_schema,
                                     unlock(tag_width(*field)),
                                     Port::Dir::OUT,
                                     domain,
                                     false);
}

std::shared_ptr<Node> FieldPort::data_width() {
  std::shared_ptr<Node> width = intl(0);
  // Flatten the type
  auto flat_type = Flatten(type());
  for (const auto &ft : flat_type) {
    if (ft.type_->meta.count("array_data") > 0) {
      if (ft.type_->width()) {
        width = width + ft.type_->width().value();
      }
    }
  }
  return width;
}

std::shared_ptr<cerata::Object> FieldPort::Copy() const {
  // Take shared ownership of the type.
  auto typ = type()->shared_from_this();
  auto result = std::make_shared<FieldPort>(name(), function_, field_, fletcher_schema_, typ, dir(), domain_, profile_);
  result->meta = meta;
  return result;
}

}  // namespace fletchgen
