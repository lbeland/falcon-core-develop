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
  add_option("n_messages", n_messages_, "Number of packets to generate (0 = infinite).");
  add_option("output_file", output_file_, "Path to output CSV file.");
}

void SimpleSource::CreatePorts() {
  data_out_port_ = create_output_port<MultiChannelType<double>>(
      "data",
      MultiChannelType<double>::Parameters(nchannels_(), nsamples_(), 1.0),
      PortOutPolicy(SlotRange(1)));
}

void SimpleSource::CompleteStreamInfo() {
  // Set the parameters for the output stream
  dynamic_cast<StreamInfo<MultiChannelType<double>>&>(data_out_port_->slot(0)->streaminfo())
      .set_parameters(MultiChannelType<double>::Parameters(nchannels_(), nsamples_(), 1.0));
}

#include <chrono>

double get_source_time() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void SimpleSource::Process(ProcessingContext &context) {
  MultiChannelType<double>::Data *data_out = nullptr;
  send_times.resize(n_messages_());

  // Warm-up
  double now = get_source_time();
  for (int i=0; i < 500 && !context.terminated(); i++) {
    // Claim output buffer
    data_out = data_out_port_->slot(0)->ClaimData(false);

    // Set timestamp as value
    std::fill(data_out->data().begin(), data_out->data().end(), now);

    // Publish data
    data_out_port_->slot(0)->PublishData();
  }

  // Measurement phase
  for (int i=0; i < n_messages_() && !context.terminated(); i++) {
    // Claim output buffer
    data_out = data_out_port_->slot(0)->ClaimData(false);

    // Set timestamp as value
    double timestamp = get_source_time();
    std::fill(data_out->data().begin(), data_out->data().end(), timestamp);

    // Publish data
    data_out_port_->slot(0)->PublishData();
    // printf("%s. Sent message %u with timestamp %9f.\n",
    //     name().c_str(), packet_count_ + 1, timestamp);
    send_times[i] = timestamp;

    // sleep(1);
    
  }

}

void SimpleSource::Postprocess(ProcessingContext &context) {

  // Calculate Statistics
  printf("Total messages sent: %d\n", n_messages_());

  // Calculate statistics (skip first measurement)
  double sum_diff = 0.0;
  double max_diff = 0.0;
  int max_idx = 0;
  double sum_sq_diff = 0.0;
  std::vector<double> send_times_diff;
  send_times_diff.resize(n_messages_() - 1);

  for (int i = 0; i < n_messages_()-1; i++) {
      double diff = send_times[i+1] - send_times[i];
      send_times_diff[i] = diff;
      sum_diff += diff;
      sum_sq_diff += diff * diff;
      if (diff > max_diff) {
          max_diff = diff;
          max_idx = i;
      }
  }

  double avg_period_sec = sum_diff / (n_messages_() - 1);
  double avg_period = avg_period_sec * 1e6;  // us
  double variance = (sum_sq_diff / (n_messages_() - 1)) - (avg_period_sec * avg_period_sec);  // s²
  double std_period = sqrt(fmax(0.0, variance)) * 1e6;  // us

  printf("Average send period (us): %.6f\n", avg_period);
  printf("Max send period (us): %.6f, idx: %d\n", max_diff * 1e6, max_idx);
  printf("Std send period (us): %.6f\n", std_period);

  // Save to CSV
  FILE *fp = fopen(output_file_().c_str(), "w");
  if (fp == NULL) {
      fprintf(stderr, "Error opening file %s\n", output_file_().c_str());
      return;
  }

  fprintf(fp, "Metric,Send_period\n");
  fprintf(fp, "mean,%.6f\n", avg_period);
  fprintf(fp, "std,%.6f\n", std_period);
  fprintf(fp, "max,%.6f\n", max_diff * 1e6);

  fclose(fp);
}

REGISTERPROCESSOR(SimpleSource);
