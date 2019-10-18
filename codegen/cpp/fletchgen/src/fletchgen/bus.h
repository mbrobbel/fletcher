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

#include <cerata/api.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fletchgen/basic_types.h"

namespace fletchgen {

using cerata::Instance;
using cerata::Component;
using cerata::Port;
using cerata::intl;
using cerata::Object;

// Bus channel constructs.
PARAM_DECL_FACTORY(bus_addr_width, 64)
PARAM_DECL_FACTORY(bus_data_width, 512)
PARAM_DECL_FACTORY(bus_strobe_width, 64)
PARAM_DECL_FACTORY(bus_len_width, 8)
PARAM_DECL_FACTORY(bus_burst_step_len, 4)
PARAM_DECL_FACTORY(bus_burst_max_len, 16)

/// Defines function of a bus interface.
enum class BusFunction {
  READ,  ///< Interface reads from memory.
  WRITE  ///< Interface writes to memory.
};

struct BusSpec {
  int aw = 64;   // Address width
  int dw = 512;  // Data width
  int sw = 64;   // Strobe width
  int lw = 8;    // Len width
  int bs = 1;    // Burst step length
  int bm = 16;   // Burst max length

  /// @brief Return a bus specification from a string. See [common/cpp/include/fletcher/arrow-utils.h] for more info.
  static BusSpec FromString(std::string str, BusSpec default_to);

  /// @brief Return a human-readable version of the bus specification.
  [[nodiscard]] std::string ToString() const;
  /// @brief Return a type name for a Cerata type based on this bus specification.
  [[nodiscard]] std::string ToBusTypeName() const;
};

/// @brief Bus specification using parameters.
struct BusParam {
  /// @brief Construct a new bunch of bus parameters based on a bus spec and function, and add them to a graph.
  BusParam(cerata::Graph *parent, BusFunction func, BusSpec spec = BusSpec{}, const std::string &prefix = "");
  /// @brief Construct a new bunch of bus parameters based on a bus spec and function, and add them to a graph.
  explicit BusParam(const std::shared_ptr<cerata::Graph> &parent,
                    BusFunction func = BusFunction::READ,
                    BusSpec spec = BusSpec{},
                    const std::string &prefix = "")
      : BusParam(parent.get(), func, spec, prefix) {}

  /// Function of this bus parameter struct.
  BusFunction func_;
  /// Integer value specification of bus parameters.
  BusSpec spec_;

  // Value nodes.
  std::shared_ptr<Node> aw; ///< Address width node
  std::shared_ptr<Node> dw; ///< Data width node
  std::shared_ptr<Node> sw; ///< Strobe width node
  std::shared_ptr<Node> lw; ///< Len width node
  std::shared_ptr<Node> bs; ///< Burst step length node
  std::shared_ptr<Node> bm; ///< Burst max length node

  /// @brief Return all parameters as an object vector.
  [[nodiscard]] std::vector<std::shared_ptr<Object>> all(BusFunction func) const;
};

/// @brief Returns true if bus specifications are equal, false otherwise.
bool operator==(const BusParam &lhs, const BusParam &rhs);

/// @brief Return a Cerata type for a Fletcher bus read interface.
std::shared_ptr<Type> bus_read(const std::shared_ptr<Node> &addr_width,
                               const std::shared_ptr<Node> &data_width,
                               const std::shared_ptr<Node> &len_width);

/// @brief Return a Cerata type for a Fletcher bus write interface.
std::shared_ptr<Type> bus_write(const std::shared_ptr<Node> &addr_width,
                                const std::shared_ptr<Node> &strobe_width,
                                const std::shared_ptr<Node> &data_width,
                                const std::shared_ptr<Node> &len_width);

/// @brief Fletcher bus type with access mode conveyed through spec of params.
std::shared_ptr<Type> bus(const BusParam &param);

/// A port derived from bus parameters.
struct BusPort : public Port {
  /// @brief Construct a new port based on a bus parameters..
  BusPort(const std::string &name,
          Port::Dir dir,
          const BusParam &params,
          std::shared_ptr<ClockDomain> domain = bus_cd())
      : Port(name, bus(params), dir, std::move(domain)), params_(params) {}

  /// The bus parameters to which the type generics of the bus port are bound.
  BusParam params_;

  /// @brief Deep-copy the BusPort.
  std::shared_ptr<Object> Copy() const override;
};
/// @brief Make a new port and return a shared pointer to it.
std::shared_ptr<BusPort> bus_port(const std::string &name, Port::Dir dir, const BusParam &params);
/// @brief Make a new port, name it automatically based on the bus specification, and return a shared pointer to it.
std::shared_ptr<BusPort> bus_port(Port::Dir dir, const BusParam &params);

// TODO(johanpel): Perhaps generalize and build some sort of parameter struct/bundle in cerata for fast connection.
/// @brief Connect all bus params on a graph to the supplied source params.
void ConnectBusParam(cerata::Graph *dst, const BusParam &src);

/**
 * @brief Return a Cerata model of a BusArbiter.
 * @param function  The function of the bus; either read or write.
 * @return          A Bus(Read/Write)Arbiter Cerata component model.
 *
 * This model corresponds to either:
 *    [`hardware/interconnect/BusReadArbiterVec.vhd`](https://github.com/johanpel/fletcher/blob/develop/hardware/interconnect/BusReadArbiterVec.vhd)
 * or [`hardware/interconnect/BusWriteArbiterVec.vhd`](https://github.com/johanpel/fletcher/blob/develop/hardware/interconnect/BusWriteArbiterVec.vhd)
 * depending on the function parameter.
 *
 * Changes to the implementation of this component in the HDL source must be reflected in the implementation of this
 * function.
 */
Component *bus_arbiter(BusFunction function);

/// @brief Return a BusReadSerializer component
std::shared_ptr<Component> BusReadSerializer();

/// @brief Return a BusWriteSerializer component
std::shared_ptr<Component> BusWriteSerializer();

}  // namespace fletchgen


///  Specialization of std::hash for BusSpec
template<>
struct std::hash<fletchgen::BusParam> {
  /// @brief Hash a BusSpec.
  size_t operator()(fletchgen::BusParam const &param) const noexcept {
    auto str = param.spec_.ToBusTypeName();
    return std::hash<std::string>()(str);
  }
};
