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

#include "cerata/ipxact/ipxact.h"
#include "cerata/ipxact/component.h"
#include "cerata/logging.h"

namespace cerata::ipxact {

void IPXACTOutputGenerator::Generate() {
  CreateDir(subdir());
  CERATA_LOG(INFO, "IPXACT: generating output.");

  for (const auto &o : outputs_) {
    if ((o.graph != nullptr)) {
      if (o.graph->IsComponent()) {
        
        auto doc = ipxact::Component(*Cast<cerata::Component>(o.graph))
            .ToXML();
        
        doc->SaveFile("./text.xml");

        // // busInterfaces.xsd

        // // file.xsd
        // // auto filesets = doc.NewElement("ipxact:fileSets");
        // // component->InsertEndChild(filesets);

        // // auto fileset = doc.NewElement("ipxact:fileSet");
        // // filesets->InsertFirstChild(fileset);

        // // port.xsd
        // auto ports = doc.NewElement("ipxact:ports");
        // auto node_ports = o.graph->GetNodesOfType(Node::PORT);
        // for (const auto &n : node_ports) {
        //   auto p = *Cast<Port>(n);

        //   auto port = doc.NewElement("ipxact:port");
        //   ports->InsertFirstChild(port);

        //   auto name = doc.NewElement("ipxact:name");
        //   name->InsertFirstChild(doc.NewText(p->name().c_str()));
        //   port->InsertFirstChild(name);

        //   auto wire = doc.NewElement("ipxact:wire");
        //   port->InsertEndChild(wire);

        //   auto direction = doc.NewElement("ipxact:direction");
        //   direction->InsertFirstChild(doc.NewText(cerata::Term::str(p->dir()).c_str()));
        //   wire->InsertFirstChild(direction);
        // }
        // // component->InsertEndChild(ports);

        // // commonStructures.xsd - parameters
        // auto parameters = doc.NewElement("ipxact:parameters");
        // for (const auto &p_ : o.graph->GetNodesOfType(Node::PARAMETER)) {
        //   auto p = *Cast<Parameter>(p_);

        //   auto parameter = doc.NewElement("ipxact:parameter");
        //   parameters->InsertFirstChild(parameter);

        //   auto name = doc.NewElement("ipxact:name");
        //   name->InsertFirstChild(doc.NewText(p->name().c_str()));
        //   parameter->InsertFirstChild(name);

        //   auto value = doc.NewElement("ipxact:value");
        //   // TODO: figure out solid method to resolve this to literal
        //   auto lit = *Cast<Literal>(*p->value());
        //   value->InsertFirstChild(doc.NewText(lit->ToString().c_str()));
        //   parameter->InsertEndChild(value);
        // }
        // component->InsertEndChild(parameters);

        // doc.SaveFile("./test.xml");
        // CERATA_LOG(INFO, "IPXACT: Generating sources for component " +
        // o.graph->name());
      } else {
        // CERATA_LOG(WARNING, "IPXACT: Graph " << o.graph->name() << " is not a
        // component. Skipping output generation.");
      }
    } else {
      throw std::runtime_error("Graph is nullptr.");
    }
  }
}

}  // namespace cerata::ipxact
