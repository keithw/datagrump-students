#include <stdio.h>
#include <fstream>

#include "math.h"

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    w_size_(50),
    rtt_last_(0),
    rtt_min_(-1),
    rtt_max_(0),
    rtt_sum_(0),
    acks_count_(0),
    rtt_avg_(0),
    rtt_avg_mov_(0),
    rtt_ratio_(1.0),
    initial_timestamp_(0),
    processed_timestamp_(0),
    last_packet_sent_(0),
    last_ack_received_(0),
    capacity_estimate_(0),
    capacity_avg_(0),
    capacity_derivative_(0),
    capacity_derivative_avg_(0),
    capacity_next_(0),
    queue_estimate_(0.0),
    acks_(),
    params_()
{
  /* Default parameters */
  params_.AI = 1.0;
  params_.MD = 0.5;
  params_.use_capacity_estimate = false;
  params_.AVG = 0.2;
  params_.derivAVG = 0.7;
  params_.ack_interval_size = 60; // ms

  LoadParams("controller_config.txt");
}

/* Loads all params from file */
void Controller::LoadParams(const char* filename) {
  try {
    ifstream param_file(filename);
    param_file >> params_.AI;
    param_file >> params_.MD;
    param_file >> params_.use_capacity_estimate;
  } catch (...) {
    fprintf( stderr, "Read error, params will have default values.\n");
  }
  fprintf( stderr, "Read params AI:%.1f, MD:%.1f\n",
      params_.AI, params_.MD);
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp() - initial_timestamp_, (unsigned int) w_size_ );
  }

  return (unsigned int) w_size_;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
            const uint64_t send_timestamp )
{
  last_packet_sent_ = sequence_number;

  if (initial_timestamp_ == 0) {
    initial_timestamp_ = send_timestamp;
    processed_timestamp_ = send_timestamp;
  }

  /* Default: take no action */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp - initial_timestamp_, sequence_number );
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  double rtt = (double)(timestamp_ack_received - send_timestamp_acked);
  update_rtt_stats(rtt);

  /* Update window based on RTT average */
  w_size_ = max(w_size_ + log(pow((1/(rtt_ratio_/2.2)),10))*(params_.AI / max(w_size_, 1.0)),1.0);

  if (params_.use_capacity_estimate) {
    last_ack_received_ = sequence_number_acked;
    acks_.push_back(Ack(send_timestamp_acked, recv_timestamp_acked,
          timestamp_ack_received));
    update_window_size(timestamp_ack_received);
  }
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received - initial_timestamp_, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked - initial_timestamp_, recv_timestamp_acked - initial_timestamp_ );
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 60;
}

void Controller::packet_timed_out(void)
{
}

void Controller::update_rtt_stats(const double rtt)
{
  rtt_last_ = rtt;

  if (rtt_min_ == -1 || rtt_min_ > rtt){
    rtt_min_ = rtt;
  }
  if (rtt_max_ < rtt){
    rtt_max_ = rtt;
  }

  if (rtt_avg_mov_ == 0){
    rtt_avg_ = rtt;
  }
  else {
    rtt_avg_mov_ = params_.AVG*rtt + (1-params_.AVG)*rtt_avg_mov_;
  }

  rtt_sum_ = rtt_sum_ + rtt;
  acks_count_ = acks_count_ + 1;
  rtt_avg_ = rtt_sum_/acks_count_;

  rtt_ratio_ = rtt/rtt_min_;
}

/* Update capacity estimates after ack received */
void Controller::update_capacity_stats(const uint64_t timestamp) {
  uint64_t min_time = timestamp - params_.ack_interval_size;
  while (acks_.size() > 0 && acks_.front().acked_ < min_time) {
    acks_.pop_front();
  }

  /* Capacity estimate heuristic */
  double capacity_last = capacity_estimate_;
  capacity_estimate_ = ((double) acks_.size()) / params_.ack_interval_size;

  /* Weighted average of capacity */
  if (capacity_avg_ == 0) {
    capacity_avg_ = capacity_estimate_;
  } else {
    capacity_avg_ = params_.AVG * capacity_estimate_ + (1 - params_.AVG) * capacity_avg_;
  }

  /* Derivatives */
  capacity_derivative_ = (capacity_estimate_ - capacity_last) / params_.ack_interval_size;
  capacity_derivative_avg_ = params_.derivAVG * capacity_derivative_ +
    (1 - params_.AVG) * capacity_derivative_avg_;
}

void Controller::update_window_size(const uint64_t timestamp) {
  /* Process all the tics that happened before timestamp */
  while (processed_timestamp_ + params_.ack_interval_size < timestamp) {
    processed_timestamp_ += params_.ack_interval_size;
    update_capacity_stats(processed_timestamp_);
  }

  capacity_next_ = capacity_avg_ +
    capacity_derivative_avg_ * (timestamp - processed_timestamp_) / 100;
  queue_estimate_ = (rtt_last_ - rtt_min_) * capacity_avg_;

  if (capacity_next_ > 0) {
     w_size_ = max(1.0, 1.95 * capacity_next_ * 40 - queue_estimate_ / 100 * 40);
  }
}
