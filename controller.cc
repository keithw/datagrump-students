#include <stdio.h>

#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

namespace {
  const double INITIAL_WINDOW = 15.0;
  const int RTT_THRESHOLD = 100;
  const double WINDOW_SIZE_ADJUSTMENT = 0.1;
}

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    my_window_size_(INITIAL_WINDOW) {
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void ) {
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %f.\n",
	     timestamp(), my_window_size_ );
  }

  return static_cast<int>(my_window_size_);
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp ) {
          /* in milliseconds */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
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
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }

  int difference = static_cast<int>(send_timestamp_acked + RTT_THRESHOLD)
      - static_cast<int>(timestamp_ack_received);
  if (difference >= 0) {
    my_window_size_ += std::min(1.0, difference * 0.002 / my_window_size_);
  } else {
    my_window_size_ -= std::min(1.0, -difference * 0.01 / my_window_size_);
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void ) {
  return 1000; /* timeout of one second */
}
