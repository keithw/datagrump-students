#include <stdio.h>

#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

namespace {
  const double INITIAL_WINDOW = 1.0;
  const int RTT_THRESHOLD = 80;
  const double WINDOW_SIZE_ADJUSTMENT = 0.1;

  const double RTT_ALPHA = 1.0;
}

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    my_window_size_(INITIAL_WINDOW),
    my_rtt_estimate_(RTT_THRESHOLD),
    my_outstanding_packets_() {
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void ) {
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %f.\n",
	     timestamp(), my_window_size_ );
  }

  // Make sure we sent out at least one packet.
  return std::max(static_cast<int>(my_window_size_), 1);
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp ) {
          /* in milliseconds */
  my_outstanding_packets_[sequence_number] = send_timestamp;

  if ( debug_ ) {
    fprintf(stderr, "At time %lu, sent packet %lu.\n",
	          send_timestamp, sequence_number );
    fprintf(stderr, "%lu packet(s) outstanding.\n",
            my_outstanding_packets_.size());
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received ) {
             /* when the ack was received (by sender) */

  // Remove the packet from the window.
  std::map<uint64_t, uint64_t>::iterator packet =
      my_outstanding_packets_.find(sequence_number_acked);
  if (packet != my_outstanding_packets_.end()) {
    my_outstanding_packets_.erase(packet);
  }

  if ( debug_ ) {
    fprintf(stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf(stderr, " (sent %lu, received %lu by receiver's clock).\n",
	          send_timestamp_acked, recv_timestamp_acked );
    fprintf(stderr, "%lu packet(s) outstanding.\n",
            my_outstanding_packets_.size());
  }


  double rtt = static_cast<double>(timestamp_ack_received)
      - static_cast<double>(send_timestamp_acked);
  my_rtt_estimate_ = rtt * RTT_ALPHA + my_rtt_estimate_ * (1.0 - RTT_ALPHA);

  double difference = RTT_THRESHOLD - my_rtt_estimate_;
  if (difference >= 0) {
    my_window_size_ += std::min(1.0, difference * 0.04 / my_window_size_);
  } else {
    my_window_size_ -= std::min(1.0, -difference * 0.1 / my_window_size_);
  }

  // Make sure our window size doesn't drop below 0.
  my_window_size_ = std::max(my_window_size_, 0.0);
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void ) {
  return 1000; /* timeout of one second */
}
