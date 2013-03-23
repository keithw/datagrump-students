#include <stdio.h>
#include <math.h>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

#include <queue>

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), rttmin(1000), packet_times() {
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  // This loop removes all old receive times for the packet_times queue.
  uint64_t current_time = timestamp();
  while(!packet_times.empty()
        && packet_times.front() + time_interval < current_time) {
    packet_times.pop();
  }

  // Set rate to packets received per second.
  float rate = packet_times.size() * 1000 / time_interval;
  // Rates below 1000/time_interval cannot be detected.
  if (rate < 1000/time_interval)
    rate = 1000/time_interval;

  float actual_rttmin = rttmin * 0.001; // Convert units to seconds.

  // The window size is proportional to rtt_min times the rate of receipt.
  float win_size = min_window_size + stretch_factor * actual_rttmin * rate;
  int the_window_size = (int)win_size;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }

  return the_window_size;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
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
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  int64_t rtt = (int64_t)timestamp_ack_received - (int64_t)send_timestamp_acked;
  if (rtt < rttmin)
    rttmin = rtt;

  packet_times.push(timestamp_ack_received);

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void ) {
  return 100;
}
