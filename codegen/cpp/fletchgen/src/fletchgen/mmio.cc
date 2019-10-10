// Copyright 2018 Delft University of Technology
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

#include "fletchgen/mmio.h"

#include <fletcher/fletcher.h>
#include <fletcher/common.h>
#include <cerata/api.h>
#include <memory>
#include <string>
#include <fstream>
#include <algorithm>

#include "fletchgen/axi4_lite.h"
#include "fletchgen/basic_types.h"

namespace fletchgen {

using cerata::Parameter;
using cerata::Port;
using cerata::PortArray;
using cerata::intl;
using cerata::integer;
using cerata::string;
using cerata::strl;
using cerata::boolean;
using cerata::integer;
using cerata::bit;
using cerata::RecField;
using cerata::Vector;
using cerata::Record;
using cerata::Stream;

static std::string ToString(MmioReg::Behavior behavior) {
  return behavior == MmioReg::Behavior::CONTROL ? "control" : "status";
}

MmioPort::MmioPort(const std::string &name,
                   Port::Dir dir,
                   const MmioReg &reg,
                   const std::shared_ptr<ClockDomain> &domain) :
    Port(name, reg.width == 1 ? cerata::bit() : Vector::Make(reg.width), dir, domain), reg(reg) {}

std::shared_ptr<MmioPort> mmio_port(Port::Dir dir, const MmioReg &reg, const std::shared_ptr<ClockDomain> &domain) {
  return std::make_shared<MmioPort>(reg.name, dir, reg, domain);
}

std::shared_ptr<Component> mmio(const std::vector<fletcher::RecordBatchDescription> &batches,
                                const std::vector<MmioReg> &regs) {
  // Clock/reset port.
  auto kcd = Port::Make("kcd", cr(), Port::Dir::IN, kernel_cd());
  // Create the component.
  auto comp = Component::Make("mmio", {kcd});
  // Generate all ports and add to the component.
  for (const auto &reg : regs) {
    auto dir = reg.behavior == MmioReg::Behavior::CONTROL ? Port::Dir::OUT : Port::Dir::IN;
    auto port = mmio_port(dir, reg, kernel_cd());
    // Change the name to vhdmmio convention.
    port->SetName("f_" + reg.name + (std::string(dir == Port::Dir::IN ? "_write" : "") + "_data"));
    comp->Add(port);
  }
  // Add the bus interface.
  auto bus = axi4_lite(Port::Dir::IN, kernel_cd());
  comp->Add(bus);

  // This will be a primitive component generated by VHDMMIO.
  comp->SetMeta(cerata::vhdl::metakeys::PRIMITIVE, "true");
  comp->SetMeta(cerata::vhdl::metakeys::LIBRARY, "work");
  comp->SetMeta(cerata::vhdl::metakeys::PACKAGE, "mmio_pkg");

  return comp;
}

static size_t AddrSpaceUsed(uint32_t width) {
  return 4 * (width / 32 + (width % 32 != 0));
}

std::string GenerateVhdmmioYaml(std::vector<MmioReg> *regs, std::optional<size_t *> next_addr) {
  std::stringstream ss;
  // The next free byte address.
  size_t next_free_addr = 0;
  // Header:
  ss << "metadata:\n"
        "  name: mmio\n"
        "  doc: Fletchgen generated MMIO configuration.\n"
        "  \n"
        "entity:\n"
        "  bus-flatten:  yes\n"
        "  bus-prefix:   mmio_\n"
        "  clock-name:   kcd_clk\n"
        "  reset-name:   kcd_reset\n"
        "\n"
        "features:\n"
        "  bus-width:    32\n"
        "  optimize:     yes\n"
        "\n"
        "interface:\n"
        "  flatten:      yes\n"
        "\n"
        "fields: \n";

  // Iterate over the registers and generate the appropriate YAML lines.
  for (auto &r : *regs) {
    // Determine the address.
    if (r.addr) {
      // There is a fixed address.
      ss << "  - address: " << *r.addr << "\n";
      // Just take this address plus its space as the next address. This limits how the vector of MmioRegs can be
      // supplied (fixed addr. must be at the start of the vector and ordered), but we don't currently use this in
      // any other way.
      next_free_addr = *r.addr + AddrSpaceUsed(r.width);
    } else {
      // There is not a fixed address.
      ss << "  - address: " << next_free_addr << "\n";
      next_free_addr += AddrSpaceUsed(r.width);
      r.addr = next_free_addr;
    }
    // Set doc, name and other stuff.
    ss << "    name: " << r.name << "\n";
    if (!r.desc.empty()) {
      ss << "    doc: " << r.desc << "\n";
    }
    if (r.width > 1) {
      ss << "    bitrange: " << r.index + r.width - 1 << ".." << r.index << "\n";
    } else {
      ss << "    bitrange: " << r.index << "\n";
    }
    ss << "    behavior: " << ToString(r.behavior) << "\n";
    ss << "\n";
  }

  if (next_addr) {
    **next_addr = next_free_addr;
  }

  return ss.str();
}

bool ExposeToKernel(MmioReg::Function fun) {
  switch (fun) {
    case MmioReg::Function::DEFAULT: return true;
    case MmioReg::Function::KERNEL: return true;
    case MmioReg::Function::BATCH: return true;
    default:return false;
  }
}

}  // namespace fletchgen
