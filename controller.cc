#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

// #define FIXED_WINDOW
// #define AIMD
#define DELAY_TRIGGER

// AIMD constants
#define ALPHA (0.95)
#define BETA (1.5)

// Delay-trigger constants
#define DELAY_THRESHOLD (150)

// Shared AIMD Delay-trigger constants
#define AI (1.0)
#define MD (1.0 / 2)

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd( 1.0 ), rtt( 0.0 )
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
#ifdef FIXED_WINDOW
  /* Default: fixed window size of one outstanding packet */
  int the_window_size = 2;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }

  return the_window_size;
#endif
#ifdef AIMD
  return (unsigned int) cwnd;
#endif
#ifdef DELAY_TRIGGER
  return (unsigned int) cwnd;
#endif
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  /* Default: take no action */
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
  /* Default: take no action */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
#ifdef AIMD
  // Calculate RTT
  const uint64_t this_rtt = timestamp_ack_received - send_timestamp_acked;

  // Update moving average RTT
  if (rtt == 0.0) {
    rtt = this_rtt;
  } else {
    rtt = (ALPHA) * rtt + (1.0 - ALPHA) * this_rtt;
  }

  // If above average, consider as congestion signal and MD
  // Otherwise, AI 1/cw
  if (this_rtt > BETA * rtt) {
    cwnd *= MD;
    if (cwnd < 1.0) {
      cwnd = 1.0;
    }
  } else {
    cwnd += (AI / ((unsigned int) cwnd));
  }
#endif
#ifdef DELAY_TRIGGER
  const uint64_t this_rtt = timestamp_ack_received - send_timestamp_acked;
  if (this_rtt > DELAY_THRESHOLD) {
    cwnd *= MD;
    if (cwnd < 1.0) {
      cwnd = 1.0;
    }
  } else {
    cwnd += (AI / ((unsigned int) cwnd));
  }
#endif
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}

