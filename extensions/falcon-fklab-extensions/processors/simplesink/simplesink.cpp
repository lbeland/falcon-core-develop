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
#include <chrono>
#include <numeric>
#include <limits>

SimpleSink::SimpleSink() : IProcessor(PRIORITY_HIGH) {
  add_option("n_messages", n_messages_, "Number of packets to receive (0 = infinite).");
  add_option("output_file", output_file_, "Path to output CSV file.");
}

void SimpleSink::CreatePorts() {
  data_in_port_ = create_input_port<MultiChannelType<double>>(
      "data", 
      MultiChannelType<double>::Capabilities(ChannelRange(1, 256), SampleRange(1, 10000)),
      PortInPolicy(SlotRange(1)));
}

double get_sink_time() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void SimpleSink::Process(ProcessingContext &context) {
  MultiChannelType<double>::Data *data_in = nullptr;
  recv_times.resize(n_messages_());
  process_times.resize(n_messages_());

  // Warm-up
  while (warm_up_count_ < 500 && !context.terminated()) {
    if (!data_in_port_->slot(0)->RetrieveData(data_in)) {
      break;
    }
    data_in_port_->slot(0)->ReleaseData();
    warm_up_count_++;
  }

  // Measurement phase
  while (packet_count_ < n_messages_() && !context.terminated()) {
    
    // Try to retrieve data
    if (!data_in_port_->slot(0)->RetrieveData(data_in)) {
      break;
    }
    double now = get_sink_time();

    // Process the data, take first sample of first channel
    double timestamp = data_in->data()[0];
    if (first_timestamp_ == 0.0) {
      first_timestamp_ = timestamp;
    }

    recv_times[packet_count_] = now;
    process_times[packet_count_] = now - timestamp;

    // printf("%s. Received packet %u with timestamp %9f.\n", name().c_str(), packet_count_ + 1, timestamp);
    
    packet_count_++;
    // Release data
    data_in_port_->slot(0)->ReleaseData();
  }

}

void SimpleSink::Postprocess(ProcessingContext &context) {
  printf("Total messages processed: %d\n", packet_count_);

  // Jitter statistics
  double sum_diff = 0.0;
  double max_diff = 0.0;
  int max_idx = 0;
  double sum_sq_diff = 0.0;
  std::vector<double> recv_times_diff;
  recv_times_diff.resize(packet_count_ - 1);

  for (int i = 0; i < packet_count_-1; i++) {
      double diff = recv_times[i+1] - recv_times[i];
      recv_times_diff[i] = diff;
      sum_diff += diff;
      sum_sq_diff += diff * diff;
      if (diff > max_diff) {
          max_diff = diff;
          max_idx = i;
      }
  }
  double avg_period_sec = sum_diff / (packet_count_ - 1);
  double avg_period = avg_period_sec * 1e6;  // us
  double variance = (sum_sq_diff / (packet_count_ - 1)) - (avg_period_sec * avg_period_sec);  // s²
  double std_period = sqrt(fmax(0.0, variance)) * 1e6;  // us

  printf("Average receive period (us): %.6f\n", avg_period);
  printf("Max receive period (us): %.6f, idx: %d\n", max_diff*1e6, max_idx);
  printf("Std receive period (us): %.6f\n", std_period);

  // Latency statistics
  double sum = 0.0;
  double max = 0.0;
  max_idx = 0;
  double sum_sq = 0.0;

  for (int i=0;i<packet_count_;i++) {
      sum += process_times[i];
      sum_sq += process_times[i] * process_times[i];
      if (process_times[i] > max) {
          max = process_times[i];
          max_idx = i;
      }
  }
  double avg_latency_sec = sum / packet_count_;
  double avg_latency = avg_latency_sec * 1e6;  // us
  double variance_latency = (sum_sq / packet_count_) - (avg_latency_sec * avg_latency_sec);  // s²
  double std_latency = sqrt(fmax(0.0, variance_latency)) * 1e6;  // us

  printf("Average latency (us): %.6f\n", avg_latency);
  printf("Max latency (us): %.6f, idx: %d\n", max*1e6, max_idx);
  printf("Std latency (us): %.6f\n", std_latency);

  double throughput = packet_count_ / (recv_times[packet_count_-1] - first_timestamp_)/1e3;
  printf("Throughput (msg/ms): %d/%.6f = %.2f\n", packet_count_, (recv_times[packet_count_-1] - first_timestamp_)*1e3, throughput);


  // Save to CSV
  FILE *fp = fopen(output_file_().c_str(), "w");
  if (fp == NULL) {
      fprintf(stderr, "Error opening file %s\n", output_file_().c_str());
      return;
  }

  fprintf(fp, "Metric,Recv_period,Latency,Throughput\n");
  fprintf(fp, "mean,%.6f,%.6f,%.6f\n", avg_period, avg_latency, throughput);
  fprintf(fp, "std,%.6f,%.6f, \n", std_period, std_latency);
  fprintf(fp, "max,%.6f,%.6f, \n", max_diff*1e6, max*1e6);

  fclose(fp);
}

REGISTERPROCESSOR(SimpleSink);
