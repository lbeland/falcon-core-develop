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

#include "simplesource.hpp"
#include "logging/log.hpp"
#include <thread>
#include <chrono>

SimpleSource::SimpleSource() : IProcessor(PRIORITY_HIGH) {
  add_option("nchannels", nchannels_, "Number of channels to generate.");
  add_option("nsamples", nsamples_, "Number of samples per packet.");
  add_option("sample_rate", sample_rate_, "Sample rate in Hz.");
  add_option("npackets", npackets_, "Number of packets to generate (0 = infinite).");
}

void SimpleSource::CreatePorts() {
  data_out_port_ = create_output_port<MultiChannelType<double>>(
      "data",
      MultiChannelType<double>::Parameters(nchannels_(), nsamples_(), sample_rate_()),
      PortOutPolicy(SlotRange(1)));
}

void SimpleSource::CompleteStreamInfo() {
  // Set the parameters for the output stream
  dynamic_cast<StreamInfo<MultiChannelType<double>>&>(data_out_port_->slot(0)->streaminfo())
      .set_parameters(MultiChannelType<double>::Parameters(nchannels_(), nsamples_(), sample_rate_()));
}

void SimpleSource::Process(ProcessingContext &context) {
  MultiChannelType<double>::Data *data_out = nullptr;

  while (!context.terminated()) {
    // Check if we've sent enough packets
    if (npackets_() > 0 && packet_count_ >= npackets_()) {
      LOG(INFO) << name() << ". Generated " << packet_count_ << " packets. Done.";
      break;
    }

    // Claim output buffer
    data_out = data_out_port_->slot(0)->ClaimData(false);
    
    // Fill with ones
    std::fill(data_out->data().begin(), data_out->data().end(), 1.0);
    printf("%s. Generated packet %u with %zu channels and %zu samples at %.2f Hz.\n",
           name().c_str(), packet_count_ + 1, data_out->nchannels(), data_out->nsamples(),
           data_out->sample_rate());
    
    // Set timestamps
    uint64_t hw_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    data_out->set_hardware_timestamp(hw_timestamp);
    data_out->set_source_timestamp();
    
    // Publish data
    data_out_port_->slot(0)->PublishData();
    
    packet_count_++;
    
    // Small delay to avoid overwhelming the system
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void SimpleSource::Postprocess(ProcessingContext &context) {
  LOG(INFO) << name() << ". Generated " << packet_count_ << " data packets.";
}

REGISTERPROCESSOR(SimpleSource);
