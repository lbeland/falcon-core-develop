// ---------------------------------------------------------------------
// This file is part of falcon-core.
//
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
//
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with falcon-core. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

#include "simplesink.hpp"
#include "logging/log.hpp"
#include <numeric>
#include <limits>

void SimpleSink::CreatePorts() {
  data_in_port_ = create_input_port<MultiChannelType<double>>(
      "data", 
      MultiChannelType<double>::Capabilities(ChannelRange(1, 256), SampleRange(1, 10000)),
      PortInPolicy(SlotRange(1)));
}

void SimpleSink::Process(ProcessingContext &context) {
  MultiChannelType<double>::Data *data_in = nullptr;

  while (!context.terminated()) {
    // Try to retrieve data
    if (!data_in_port_->slot(0)->RetrieveData(data_in)) {
      break;
    }

    // Process the data - sum all values
    double sum = std::accumulate(data_in->data().begin(), data_in->data().end(), 0.0);
    sum_of_values_ += sum;
    printf("%s. Received packet %u with sum %.2f.\n", name().c_str(), packet_count_ + 1, sum);
    
    packet_count_++;
    
    // Log every 10 packets
    if (packet_count_ % 10 == 0) {
      LOG(INFO) << name() << ". Received " << packet_count_ 
                << " packets. Sum so far: " << sum_of_values_;
    }

    // Release data
    data_in_port_->slot(0)->ReleaseData();
  }
}

void SimpleSink::Postprocess(ProcessingContext &context) {
  LOG(INFO) << name() << ". Received " << packet_count_ << " data packets.";
  LOG(INFO) << name() << ". Total sum of all values: " << sum_of_values_;
}

REGISTERPROCESSOR(SimpleSink);
