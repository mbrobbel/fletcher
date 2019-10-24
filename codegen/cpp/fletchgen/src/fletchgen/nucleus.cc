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

#include "fletchgen/nucleus.h"

#include <cerata/api.h>
#include <vector>
#include <string>
#include <cerata/parameter.h>

#include "fletchgen/basic_types.h"
#include "fletchgen/recordbatch.h"
#include "fletchgen/kernel.h"
#include "fletchgen/mmio.h"
#include "fletchgen/profiler.h"
#include "fletchgen/axi4_lite.h"

namespace fletchgen {

using cerata::port;
using cerata::vector;
using cerata::component;
using cerata::parameter;

Component *accm() {
  // Check if the Array component was already created.
  auto opt_comp = cerata::default_component_pool()->Get("ArrayCmdCtrlMerger");
  if (opt_comp) {
    return *opt_comp;
  }

  auto ba = bus_addr_width();
  auto iw = index_width();
  auto tw = tag_width();
  auto cw = parameter("num_addr", integer(), intl(0));
  auto nucleus_side_cmd = port("nucleus_cmd", cmd_type(iw, tw, cw), Port::Dir::OUT, kernel_cd());
  auto kernel_side_cmd = port("kernel_cmd", cmd_type(iw, tw), Port::Dir::IN, kernel_cd());
  auto ctrl = port_array("ctrl", vector(64), cw, Port::Dir::IN, kernel_cd());
  auto result = component("ArrayCmdCtrlMerger", {cw, ba, iw, tw, nucleus_side_cmd, kernel_side_cmd, ctrl});

  // This is a primitive component from the hardware lib
  result->SetMeta(cerata::vhdl::meta::PRIMITIVE, "true");
  result->SetMeta(cerata::vhdl::meta::LIBRARY, "work");
  result->SetMeta(cerata::vhdl::meta::PACKAGE, "Array_pkg");

  return result.get();
}

static void CopyFieldPorts(Component *nucleus, const RecordBatch &record_batch, FieldPort::Function fun) {
  // Add Arrow field derived ports with some function.
  auto field_ports = record_batch.GetFieldPorts(fun);
  cerata::NodeMap rebinding;
  for (const auto &fp : field_ports) {
    // Create a copy and invert for the Nucleus
    auto copied_port = dynamic_cast<FieldPort *>(fp->CopyOnto(nucleus, fp->name(), &rebinding));
    copied_port->InvertDirection();
  }
}

Nucleus::Nucleus(const std::string &name,
                 const std::vector<std::shared_ptr<RecordBatch>> &recordbatches,
                 const std::shared_ptr<Kernel> &kernel,
                 const std::shared_ptr<Component> &mmio)
    : Component(name) {
  cerata::NodeMap rebinding;

  Add(index_width());
  Add(tag_width());
  // Add clock/reset
  auto kcd = port("kcd", cr(), Port::Dir::IN, kernel_cd());
  Add(kcd);
  // Add AXI4-lite interface
  auto axi = axi4_lite(Port::Dir::IN, bus_cd());
  Add(axi);

  // Instantiate the kernel and connect the clock/reset.
  kernel_inst = Instantiate(kernel.get());
  Connect(kernel_inst->prt("kcd"), kcd.get());

  // Instantiate the MMIO component and connect the AXI4-lite port and clock/reset.
  auto mmio_inst = Instantiate(mmio.get());
  mmio_inst->prt("mmio") <<= axi;
  mmio_inst->prt("kcd") <<= kcd;
  // For the kernel user, we need to abstract the "ctrl" field of the command streams away.
  // We need to instantiate a little ArrayCommandCtrlMerger (accm) component that just adds the buffer addresses to
  // the cmd stream ctrl field. We will remember the instances of that component and we'll get the buffer address ports
  // for later on.
  std::vector<Instance *> accms;
  // Get all the buffer ports from the mmio instance.
  std::vector<MmioPort *> mmio_buffer_ports;
  for (const auto &p : mmio_inst->GetAll<MmioPort>()) {
    if (p->reg.function == MmioReg::Function::BUFFER) {
      mmio_buffer_ports.push_back(p);
    }
  }

  // Copy over the field-derived ports from the RecordBatches.
  for (const auto &rb : recordbatches) {
    CopyFieldPorts(this, *rb, FieldPort::Function::ARROW);
    CopyFieldPorts(this, *rb, FieldPort::Function::UNLOCK);

    // For each one of the command streams, make an inverted copy of the RecordBatch full command stream port.
    // This one will expose all command stream fields to the nucleus user.
    auto cmd_ports = rb->GetFieldPorts(FieldPort::Function::COMMAND);
    for (const auto &cmd : cmd_ports) {
      // The command stream port type references the bus address width.
      // Add that parameter to the nucleus.
      auto prefix = rb->schema()->name() + "_" + cmd->field_->name();
      auto ba = bus_addr_width(64, prefix);
      Add(ba);

      auto nucleus_cmd = dynamic_cast<FieldPort *>(cmd->CopyOnto(this, cmd->name(), &rebinding));
      nucleus_cmd->InvertDirection();

      // Now, instantiate an ACCM that will merge the buffer addresses onto the command stream at the nucleus level.
      auto accm_inst = Instantiate(accm(), cmd->name() + "_accm_inst");
      // Connect the parameters.
      accm_inst->par(bus_addr_width())->SetValue(ba);
      Connect(accm_inst->par(index_width()), par(index_width()));
      Connect(accm_inst->par(tag_width()), par(tag_width()));
      // Remember the instance.
      accms.push_back(accm_inst);
    }
  }

  // Add and connect all recordbatch ports
  size_t batch_idx = 0;
  size_t accm_idx = 0;
  size_t buf_idx = 0;
  for (const auto &r : recordbatches) {
    // Connect Arrow data stream
    for (const auto &ap : r->GetFieldPorts(FieldPort::Function::ARROW)) {
      auto kernel_data = kernel_inst->prt(ap->name());
      auto nucleus_data = prt(ap->name());
      std::shared_ptr<cerata::Edge> edge;
      if (ap->dir() == Port::OUT) {
        edge = Connect(kernel_data, nucleus_data);
      } else {
        edge = Connect(nucleus_data, kernel_data);
      }
    }

    // Connect unlock stream
    for (const auto &up : r->GetFieldPorts(FieldPort::Function::UNLOCK)) {
      auto kernel_unl = kernel_inst->prt(up->name());
      auto nucleus_unl = prt(up->name());
      Connect(kernel_unl, nucleus_unl);
    }

    // Connect the command stream through the ACCM.
    size_t field_idx = 0;
    for (const auto &cmd : r->GetFieldPorts(FieldPort::Function::COMMAND)) {
      // Get the ports on either side of the ACCM.
      auto accm_nucleus_cmd = accms[accm_idx]->prt("nucleus_cmd");
      auto accm_kernel_cmd = accms[accm_idx]->prt("kernel_cmd");
      auto accm_ctrl = accms[accm_idx]->prt_arr("ctrl");

      // Get the corresponding cmd ports on this nucleus and the kernel.
      auto nucleus_cmd = prt(cmd->name());
      auto kernel_cmd = kernel_inst->prt(cmd->name());

      // Connect the nucleus cmd to the ACCM cmd and the ACCM command to the kernel cmd.
      Connect(nucleus_cmd, accm_nucleus_cmd);
      Connect(accm_kernel_cmd, kernel_cmd);

      // To connect the buffer addresses from the mmio to the ACCM, we need to figure out which buffers there are.
      // We can look this up in the RecordBatchDescription.
      auto field_bufs = r->batch_desc().fields[field_idx].buffers;
      for (size_t b = 0; b < field_bufs.size(); b++) {
        // TODO(johanpel): it is here somewhat blatantly assumed mmio_buffer_ports follows ordering, etc.. properly.
        //  Perhaps it would be nicer if this was somewhat better synchronized.
        Connect(accm_ctrl->Append(), mmio_buffer_ports[buf_idx]);
        buf_idx++;
      }
      field_idx++;
      accm_idx++;
    }
    batch_idx++;
  }

  // Perform some magic to abstract the buffer addresses away from the ctrl stream at the kernel level.
  // First, obtain the intended name for the kernel from the metadata of the vhdmmio component port metadata.
  // Then, make a connection between these two components.
  for (auto &p : mmio_inst->GetAll<MmioPort>()) {
    if (ExposeToKernel(p->reg.function)) {
      auto inst_port = kernel_inst->prt(p->reg.name);
      if (p->dir() == Port::Dir::OUT) {
        Connect(inst_port, p);
      } else {
        Connect(p, inst_port);
      }
    }
  }

  // Gather all Field-derived ports that require profiling on this Nucleus.
  ProfileDataStreams(mmio_inst);
}

std::shared_ptr<Nucleus> nucleus(const std::string &name,
                                 const std::vector<std::shared_ptr<RecordBatch>> &recordbatches,
                                 const std::shared_ptr<Kernel> &kernel,
                                 const std::shared_ptr<Component> &mmio) {
  return std::make_shared<Nucleus>(name, recordbatches, kernel, mmio);
}

std::vector<FieldPort *> Nucleus::GetFieldPorts(FieldPort::Function fun) const {
  std::vector<FieldPort *> result;
  for (const auto &ofp : GetNodes()) {
    auto fp = dynamic_cast<FieldPort *>(ofp);
    if (fp != nullptr) {
      if (fp->function_ == fun) {
        result.push_back(fp);
      }
    }
  }
  return result;
}

void Nucleus::ProfileDataStreams(Instance *mmio_inst) {
  cerata::NodeMap rebinding;
  // Insert a signal in between, and then mark that signal for profiling.
  std::vector<cerata::Signal *> profile_nodes;
  for (const auto &p : GetFieldPorts(FieldPort::Function::ARROW)) {
    if (p->profile_) {
      // At this point, these ports should only have one edge straight into the kernel.
      if (p->edges().size() != 1) {
        FLETCHER_LOG(ERROR, "Nucleus port has other than exactly one edge.");
      }
      // Insert a signal node in between that we can attach the profiler probe onto.
      auto s = AttachSignalToNode(this, p, &rebinding);
      profile_nodes.push_back(s);
    }
  }

  if (!profile_nodes.empty()) {
    // Attach stream profilers to the ports that need to be profiled.
    auto profile_results = EnableStreamProfiling(this, profile_nodes);

    // TODO(johanpel): in the following code it is assumed ordering between profile nodes, streams and mmio ports is
    //  unchanged as well. This assumption might be a bit wild if things get added in the future, so it would be nice
    //  to figure out a better way to keep this synchronized.

    // Get the enable and clear ports.
    auto mmio_enable = signal("profile_enable", cerata::bit(), kernel_cd());
    auto mmio_clear = signal("profile_clear", cerata::bit(), kernel_cd());
    Add({mmio_enable, mmio_clear});

    Connect(mmio_enable.get(), mmio_inst->prt("f_profile_enable_data"));
    Connect(mmio_clear.get(), mmio_inst->prt("f_profile_clear_data"));

    // Gather all mmio profile result ports
    std::vector<MmioPort *> mmio_profile_ports;
    for (auto &p : mmio_inst->GetAll<MmioPort>()) {
      if ((p->reg.function == MmioReg::Function::PROFILE) && (p->reg.behavior == MmioReg::Behavior::STATUS)) {
        mmio_profile_ports.push_back(p);
      }
    }
    // Loop over all profiled nodes and connect them.
    size_t port_idx = 0;
    for (const auto &pair : profile_results) {
      auto prof_instances = pair.second.first;
      auto prof_ports = pair.second.second;

      for (const auto &prof_inst : prof_instances) {
        Connect(prof_inst->prt("enable"), mmio_enable.get());
        Connect(prof_inst->prt("clear"), mmio_clear.get());
      }

      for (const auto &profile_result : prof_ports) {
        Connect(mmio_profile_ports[port_idx], profile_result);
        port_idx++;
      }
    }
  }
}

}  // namespace fletchgen
