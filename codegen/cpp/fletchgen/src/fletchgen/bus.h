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

// Bus channel classes:

/// Defines function of a bus interface.
enum class BusFunction {
  READ,  ///< Interface reads from memory.
  WRITE  ///< Interface writes to memory.
};

/// @brief Bus specification using parameters.
struct BusParam {
  /// Width of address.
  std::shared_ptr<Node> addr_width = bus_addr_width();
  /// Width of data.
  std::shared_ptr<Node> data_width = bus_data_width();
  /// Width of strobe.
  std::shared_ptr<Node> strobe_width = bus_strobe_width();
  /// Width of burst length field.
  std::shared_ptr<Node> len_width = bus_len_width();
  /// Minimum burst size.
  std::shared_ptr<Node> burst_step = bus_burst_step_len();
  /// Maximum burst size.
  std::shared_ptr<Node> burst_max = bus_burst_max_len();
  /// @brief Return all parameters as an object vector.
  [[nodiscard]] std::vector<std::shared_ptr<Object>> all(std::optional<BusFunction> function = std::nullopt) const;
  /// @brief Return a human-readable version of the bus specification.
  [[nodiscard]] std::string ToString() const;
  /// @brief Return a type name for a Cerata type based on this bus specification.
  [[nodiscard]] std::string ToBusTypeName() const;
};

/// @brief Returns true if bus specifications are equal, false otherwise.
bool operator==(const BusParam &lhs, const BusParam &rhs);

/// @brief Fletcher bus type with access mode conveyed through parameters.
std::shared_ptr<Type> bus(BusFunction function);

/// @brief Fletcher bus type with access mode conveyed through spec of params.
std::shared_ptr<Type> bus(const BusParam &spec, BusFunction function);

/// @brief Return a Cerata type for a Fletcher bus read interface.
std::shared_ptr<Type> bus_read(const std::shared_ptr<Node> &addr_width,
                               const std::shared_ptr<Node> &len_width,
                               const std::shared_ptr<Node> &data_width);

/// @brief Return a Cerata type for a Fletcher bus write interface.
std::shared_ptr<Type> bus_write(const std::shared_ptr<Node> &addr_width,
                                const std::shared_ptr<Node> &len_width,
                                const std::shared_ptr<Node> &data_width);

/// A port derived from bus parameters.
struct BusPort : public Port {
  /// Bus specification.
  BusParam params_;
  /// Bus function.
  BusFunction function_;
  /// @brief Construct a new port based on a bus parameters..
  BusPort(const std::string &name,
          Port::Dir dir,
          const BusParam &params,
          BusFunction function,
          std::shared_ptr<ClockDomain> domain = bus_cd())
      : Port(name.empty() ? "bus" : name, bus(params, function), dir, std::move(domain)),
        params_(params),
        function_(function) {}
  /// @brief Deep-copy the BusPort.
  std::shared_ptr<Object> Copy() const override;
};

/// @brief Make a new port and return a shared pointer to it.
std::shared_ptr<BusPort> bus_port(const std::string &name, Port::Dir dir, const BusParam &params, BusFunction func);
/// @brief Make a new port, name it automatically based on the bus specification, and return a shared pointer to it.
std::shared_ptr<BusPort> bus_port(Port::Dir dir, const BusParam &spec, BusFunction func);

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
  size_t operator()(fletchgen::BusParam const &s) const noexcept {
    auto str = s.data_width->ToString() + s.addr_width->ToString() + s.len_width->ToString()
        + s.burst_step->ToString() + s.burst_max->ToString();
    return std::hash<std::string>()(str);
  }
};
