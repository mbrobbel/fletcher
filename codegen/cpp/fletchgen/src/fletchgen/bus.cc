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

#include "fletchgen/bus.h"

#include <cerata/api.h>

#include <memory>
#include <vector>

#include "fletchgen/basic_types.h"

namespace fletchgen {

using cerata::PortArray;
using cerata::record;
using cerata::integer;
using cerata::string;
using cerata::boolean;
using cerata::strl;
using cerata::intl;
using cerata::booll;
using cerata::component;

// Bus types
std::shared_ptr<Type> bus_read(const std::shared_ptr<Node> &addr_width,
                               const std::shared_ptr<Node> &data_width,
                               const std::shared_ptr<Node> &len_width) {
  auto raddr = field("addr", vector("addr", addr_width));
  auto rlen = field("len", vector("len", len_width));
  auto rreq_record = record("rreq:rec", {raddr, rlen});
  auto rreq = stream("rreq:str", rreq_record);
  auto rdata = field(vector("data", data_width));
  auto rlast = field("last", last());
  auto rdat_record = record("rdat:rec", {rdata, rlast});
  auto rdat = stream("rdat:str", rdat_record);
  auto result = record("BusRead", {field("rreq", rreq), field("rdat", rdat, true)});
  return result;
}
std::shared_ptr<Type> bus_write(const std::shared_ptr<Node> &addr_width,
                                const std::shared_ptr<Node> &data_width,
                                const std::shared_ptr<Node> &strobe_width,
                                const std::shared_ptr<Node> &len_width) {
  auto waddr = field(vector("addr", addr_width));
  auto wlen = field(vector("len", len_width));
  auto wreq_record = record("wreq:rec", {waddr, wlen});
  auto wreq = stream("wreq:stm", wreq_record);
  auto wdata = field(vector("data", data_width));
  auto wstrobe = field(vector("strobe", strobe_width));
  auto wlast = field("last", last());
  auto wdat_record = record("wdat:rec", {wdata, wstrobe, wlast});
  auto wdat = stream("wdat:stm", wdat_record);
  auto result = record("BusWrite", {field("wreq", wreq), field("wdat", wdat)});
  return result;
}

static std::string GetBusArbiterName(BusFunction function) {
  return std::string("Bus") + (function == BusFunction::READ ? "Read" : "Write") + "ArbiterVec";
}

Component *bus_arbiter(BusFunction function) {
  // This component model corresponds to a VHDL primitive. Any modifications should be reflected accordingly.
  auto name = GetBusArbiterName(function);

  // If it already exists, just return the existing component.
  auto optional_existing_comp = cerata::default_component_pool()->Get(name);
  if (optional_existing_comp) {
    return *optional_existing_comp;
  }

  // Create a new component.
  auto result = component(name);

  // Parameters.
  auto baw = bus_addr_width();
  auto bdw = bus_data_width();
  auto bsw = bus_strobe_width();
  auto blw = bus_len_width();
  auto num_slv = parameter("NUM_SLAVE_PORTS", integer(), intl(0));

  result->Add({baw, bdw});
  if (function == BusFunction::WRITE) { result->Add(bsw); }
  result->Add({blw, num_slv});

  auto empty_str = strl("");
  result->Add({parameter("ARB_METHOD", string(), strl("RR-STICKY")),
               parameter("MAX_OUTSTANDING", integer(), intl(4)),
               parameter("RAM_CONFIG", string(), empty_str),
               parameter("SLV_REQ_SLICES", boolean(), booll(true)),
               parameter("MST_REQ_SLICE", boolean(), booll(true)),
               parameter("MST_DAT_SLICE", boolean(), booll(true)),
               parameter("SLV_DAT_SLICES", boolean(), booll(true))
              });

  // Clock/reset
  auto clk_rst = port("bcd", cr(), Port::Dir::IN, bus_cd());
  // Master port
  auto mst = bus_port("mst", Port::Dir::OUT, BusParam({baw, bdw, bsw, blw}), function);
  // Slave ports
  auto slv_base = std::dynamic_pointer_cast<BusPort>(mst->Copy());
  slv_base->SetName("bsv");
  slv_base->InvertDirection();
  auto slv_arr = port_array(slv_base, num_slv);
  // Add all ports.
  result->Add({clk_rst, mst, slv_arr});

  // This component is a primitive as far as Cerata is concerned.
  result->SetMeta(cerata::vhdl::meta::PRIMITIVE, "true");
  result->SetMeta(cerata::vhdl::meta::LIBRARY, "work");
  result->SetMeta(cerata::vhdl::meta::PACKAGE, "Interconnect_pkg");

  return result.get();
}

std::shared_ptr<Component> BusReadSerializer() {
  auto aw = parameter("ADDR_WIDTH", integer());
  auto mdw = parameter("MASTER_DATA_WIDTH", integer());
  auto mlw = parameter("MASTER_LEN_WIDTH", integer());
  auto sdw = parameter("SLAVE_DATA_WIDTH", integer());
  auto slw = parameter("SLAVE_LEN_WIDTH", integer());
  static auto ret = component("BusReadSerializer", {
      aw, mdw, mlw, sdw, slw,
      parameter("SLAVE_MAX_BURST", integer()),
      parameter("ENABLE_FIFO", boolean(), booll(false)),
      parameter("SLV_REQ_SLICE_DEPTH", integer(), intl(2)),
      parameter("SLV_DAT_SLICE_DEPTH", integer(), intl(2)),
      parameter("MST_REQ_SLICE_DEPTH", integer(), intl(2)),
      parameter("MST_DAT_SLICE_DEPTH", integer(), intl(2)),
      port("bcd", cr(), Port::Dir::IN, bus_cd()),
      port("mst", bus_read(aw, mlw, mdw), Port::Dir::OUT),
      port("slv", bus_read(aw, slw, sdw), Port::Dir::OUT),
  });
  ret->SetMeta(cerata::vhdl::meta::PRIMITIVE, "true");
  ret->SetMeta(cerata::vhdl::meta::LIBRARY, "work");
  ret->SetMeta(cerata::vhdl::meta::PACKAGE, "Interconnect_pkg");
  return ret;
}

bool operator==(const BusParam &lhs, const BusParam &rhs) {
  return (lhs.data_width == rhs.data_width) && (lhs.addr_width == rhs.addr_width) && (lhs.len_width == rhs.len_width) &&
      (lhs.burst_step == rhs.burst_step) && (lhs.burst_max == rhs.burst_max);
}

std::shared_ptr<Type> bus(const BusParam &spec, BusFunction function) {
  std::shared_ptr<Type> result;
  switch (function) {
    case BusFunction::READ: return bus_read(spec.addr_width, spec.data_width, spec.len_width);
    case BusFunction::WRITE: return bus_write(spec.addr_width, spec.data_width, spec.strobe_width, spec.len_width);
  }
  return nullptr;
}

std::shared_ptr<BusPort> bus_port(const std::string &name,
                                  Port::Dir dir,
                                  const BusParam &params,
                                  BusFunction function) {
  return std::make_shared<BusPort>(name, dir, params, function);
}

std::shared_ptr<BusPort> bus_port(Port::Dir dir, const BusParam &params, BusFunction function) {
  return std::make_shared<BusPort>("", dir, params, function);
}

std::shared_ptr<cerata::Object> BusPort::Copy() const {
  auto result = bus_port(name(), dir_, params_, function_);
  // Take shared ownership of the type
  auto typ = type()->shared_from_this();
  result->SetType(typ);
  return result;
}

std::string BusParam::ToBusTypeName() const {
  std::stringstream str;
  str << "A" << addr_width->ToString();
  str << "L" << len_width->ToString();
  str << "D" << data_width->ToString();
  str << "S" << burst_step->ToString();
  str << "M" << burst_max->ToString();
  return str.str();
}

std::string BusParam::ToString() const {
  std::stringstream str;
  str << "BusSpec[";
  str << ", addr=" << addr_width->ToString();
  str << ", len=" << len_width->ToString();
  str << ", dat=" << data_width->ToString();
  str << ", step=" << burst_step->ToString();
  str << ", max=" << burst_max->ToString();
  str << "]";
  return str.str();
}

std::vector<std::shared_ptr<Object>> BusParam::all(std::optional<BusFunction> function) const {
  if (function == BusFunction::WRITE) {
    return std::vector<std::shared_ptr<Object>>({addr_width, data_width, strobe_width, len_width, burst_step,
                                                 burst_max});
  } else if (function == BusFunction::READ) {
    return std::vector<std::shared_ptr<Object>>({addr_width, data_width, len_width, burst_step,
                                                 burst_max});
  } else {
    return std::vector<std::shared_ptr<Object>>({addr_width, data_width, strobe_width, len_width, burst_step,
                                                 burst_max});
  }
}

}  // namespace fletchgen
