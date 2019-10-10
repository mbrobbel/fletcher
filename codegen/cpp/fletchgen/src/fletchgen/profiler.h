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

#pragma once

#include <cerata/api.h>
#include <utility>
#include <string>
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>

#include "fletchgen/basic_types.h"
#include "fletchgen/nucleus.h"

namespace fletchgen {

/// @brief Obtain the registers that should be reserved in the mmio component for profiling.
std::vector<MmioReg> GetProfilingRegs(const std::vector<std::shared_ptr<RecordBatch>> &recordbatches);

/// @brief Returns a stream probe type.
std::shared_ptr<cerata::Type> stream_probe();

/// @brief Returns an instance of a StreamProfiler.
std::unique_ptr<cerata::Instance> ProfilerInstance(const std::string &name,
                                                   const std::shared_ptr<cerata::ClockDomain> &domain = kernel_cd());

/**
 * @brief Transforms a Cerata component graph to include stream profilers for selected nodes.
 *
 * To select a node for profiling, the node must be of type cerata::Stream and it must have the PROFILE_KEY set to true
 * in its kv-metadata.
 *
 * Currently doesn't make a deep copy, so it modifies the existing structure irreversibly.
 *
 * @param top           The top-level component to apply this to.
 * @param profile_nodes The nodes that should be profiled.
 * @return              A mapping from each input node to the instantiated port nodes of the profilers.
 */
std::unordered_map<Node *, std::vector<Port *>> EnableStreamProfiling(cerata::Component *comp,
                                                                      const std::vector<Node *> &profile_nodes);

}  // namespace fletchgen
