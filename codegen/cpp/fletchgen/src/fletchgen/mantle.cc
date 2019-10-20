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

#include "fletchgen/mantle.h"

#include <cerata/api.h>
#include <fletcher/common.h>

#include <memory>
#include <vector>
#include <utility>
#include <string>
#include <vector>

#include "fletchgen/basic_types.h"
#include "fletchgen/bus.h"
#include "fletchgen/nucleus.h"
#include "fletchgen/axi4_lite.h"

namespace fletchgen {

using cerata::intl;

static std::string ArbiterMasterName(BusFunction function) {
  return std::string(function == BusFunction::READ ? "rd" : "wr") + "_mst";
}

Mantle::Mantle(std::string name,
               const std::vector<std::shared_ptr<RecordBatch>> &recordbatches,
               const std::shared_ptr<Nucleus> &nucleus,
               BusSpec bus_spec)
    : Component(std::move(name)), bus_spec_(bus_spec) {

  // Add some default parameters.
  auto iw = index_width();
  auto tw = tag_width();
  Add({iw, tw});
  
  // Top level bus parameters.
  auto bus_params = BusParam(this, BusFunction::WRITE, bus_spec);

  // Add default ports; bus clock/reset, kernel clock/reset and AXI4-lite port.
  auto bcr = port("bcd", cr(), Port::Dir::IN, bus_cd());
  auto kcr = port("kcd", cr(), Port::Dir::IN, kernel_cd());
  auto axi = axi4_lite(Port::Dir::IN, bus_cd());
  Add({bcr, kcr, axi});

  // Handle the Nucleus.
  // Instantiate the nucleus and connect some default parameters.
  nucleus_inst_ = AddInstanceOf(nucleus.get());
  nucleus_inst_->prt("kcd") <<= kcr;
  nucleus_inst_->prt("mmio") <<= axi;

  Connect(nucleus_inst_->par(bus_addr_width()), par(bus_addr_width()));
  Connect(nucleus_inst_->par(tag_width()), par(tag_width()));
  Connect(nucleus_inst_->par(index_width()), par(index_width()));

  // Handle RecordBatches.
  // We've instantiated the Nucleus, and now we should feed it with data from the RecordBatch components.
  // We're going to do the following while iterating over the RecordBatch components.
  // 1. Instantiate every RecordBatch component.
  // 2. Remember the memory interface ports (bus_ports) for bus infrastructure generation later on.
  // 3. Connect all field-derived ports between RecordBatches and Nucleus.

  std::vector<BusPort *> rb_bus_ports;

  for (const auto &rb : recordbatches) {
    // Instantiate the RecordBatch
    auto rbi = AddInstanceOf(rb.get());
    recordbatch_instances_.push_back(rbi);

    // Connect bus clock/reset and kernel clock/reset.
    rbi->prt("bcd") <<= bcr;
    rbi->prt("kcd") <<= kcr;

    rbi->par(index_width()) <<= iw;
    rbi->par(tag_width()) <<= tw;

    // Look up its bus ports and remember them.
    auto rbi_bus_ports = rbi->GetAll<BusPort>();
    rb_bus_ports.insert(rb_bus_ports.end(), rbi_bus_ports.begin(), rbi_bus_ports.end());

    // Obtain all the field-derived ports from the RecordBatch Instance.
    auto field_ports = rbi->GetAll<FieldPort>();
    // Depending on their function (and direction), connect all of them to the nucleus.
    for (const auto &fp : field_ports) {
      if (fp->function_ == FieldPort::Function::ARROW) {
        if (fp->dir() == cerata::Term::Dir::OUT) {
          Connect(nucleus_inst_->prt(fp->name()), fp);
        } else {
          Connect(fp, nucleus_inst_->prt(fp->name()));
        }
      } else if (fp->function_ == FieldPort::Function::COMMAND) {
        Connect(fp, nucleus_inst_->prt(fp->name()));
      } else if (fp->function_ == FieldPort::Function::UNLOCK) {
        Connect(nucleus_inst_->prt(fp->name()), fp);
      }
    }
  }

  // Handle the bus infrastructure.
  // Now that we've instantiated and connected all RecordBatches on the Nucleus side, we need to connect their bus
  // ports to bus arbiters. We don't have an elaborate interconnection generation step yet, so we just discern between
  // read and write ports, assuming they will all get the same bus parametrization.
  //
  // Therefore, we only need a read and/or write arbiter, whatever mode RecordBatch is instantiated.
  // We take the following steps.
  // 1. instantiate them and connect them to the top level ports.
  // 2. Connect every RecordBatch bus port to the corresponding arbiter.

  // Instantiate and connect arbiters to top level.
  //Instance *arb_read = nullptr;
  //Instance *arb_write = nullptr;

  std::shared_ptr<Port> bus_read;
  std::shared_ptr<Port> bus_write;

  ArbiterMasterName(BusFunction::WRITE);
  for (const auto &rb_bus_port : rb_bus_ports) {
    FLETCHER_LOG(INFO, rb_bus_port->params_.spec_.ToString());
  }

  /*
  if (require_read_arb) {
    // Create a master bus port on the mantle.
    // bus_read = bus_port(ArbiterMasterName(BusFunction::READ), Port::Dir::OUT, bus_params, BusFunction::READ);
    Add(bus_read);
    // Instantiate and parametrize the arbiter.
    arb_read = AddInstanceOf(bus_arbiter(BusFunction::READ));
    Connect(arb_read->par(bus_addr_width()), params.addr_width);
    Connect(arb_read->par(bus_data_width()), params.data_width);
    Connect(arb_read->par(bus_len_width()), params.len_width);
    Connect(arb_read->prt("bcd"), bcr.get());
    // Connect the top level bus port to the arbiter.
    Connect(bus_read.get(), arb_read->prt("mst"));
  }
  if (require_write_arb) {
    // Create a bus port on the mantle.
    bus_write = bus_port(ArbiterMasterName(BusFunction::WRITE), Port::Dir::OUT, params, BusFunction::WRITE);
    Add(bus_write);
    // Instantiate and parametrize the arbiter.
    arb_write = AddInstanceOf(bus_arbiter(BusFunction::WRITE));
    Connect(arb_write->par(bus_addr_width()), params.addr_width);
    Connect(arb_write->par(bus_data_width()), params.data_width);
    Connect(arb_write->par(bus_strobe_width()), params.strobe_width);
    Connect(arb_write->par(bus_len_width()), params.len_width);
    Connect(arb_write->prt("bcd"), bcr.get());
    // Connect the top level bus port to the arbiter.
    Connect(bus_write.get(), arb_write->prt("mst"));
  }

  // Connect recordbatch bus ports to the appropriate arbiter.
  for (const auto &bp : rb_bus_ports) {
    // Select the corresponding arbiter.
    auto arb = bp->function_ == BusFunction::READ ? arb_read : arb_write;
    // Get the PortArray.
    auto array = arb->prt_arr("bsv");
    // Extend the recordbatch bus port because VHDL sucks.
    auto bp_sig = cerata::signal("int_" + bp->name(), bp->type()->shared_from_this(), bp->domain());
    Add(bp_sig);
    Connect(bp_sig.get(), bp);
    // Append the PortArray and connect.
    Connect(array->Append(), bp_sig.get());
  }
   */
}

/// @brief Construct a Mantle and return a shared pointer to it.
std::shared_ptr<Mantle> mantle(const std::string &name,
                               const std::vector<std::shared_ptr<RecordBatch>> &recordbatches,
                               const std::shared_ptr<Nucleus> &nucleus,
                               BusSpec bus_spec) {
  return std::make_shared<Mantle>(name, recordbatches, nucleus, bus_spec);
}

}  // namespace fletchgen
