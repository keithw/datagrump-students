#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

// #define FIXED_WINDOW
#define AIMD
// #define DELAY_TRIGGER

// FIXED_WINDOW constants
#define FIXED_WINDOW_SIZE (18)

// Delay-trigger constants
#define DELAY_THRESHOLD (300)

// Shared AIMD Delay-trigger constants
#define AI (1.0)
#define MD (1.0 / 2)
#define TIMEOUT_MS (500)

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwnd( 1.0 ),
    rtt( 0.0 ),
    last_ack_received( 0 )
{
#ifdef FIXED_WINDOW
    fprintf( stderr, "Using FIXED_WINDOW\n");
#endif
#ifdef AIMD
    fprintf( stderr, "Using AIMD, MD=%f, AI=%f, TIMEOUT_MS=%d\n", MD, AI, TIMEOUT_MS);
#endif
#ifdef DELAY_TRIGGER
    fprintf( stderr, "Using DELAY_TRIGGER\n");
#endif
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
#ifdef FIXED_WINDOW
  /* Default: fixed window size of one outstanding packet */
  int the_window_size = FIXED_WINDOW_SIZE;

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
  // Calculate gap since last ack
  uint64_t receive_gap;
  if (last_ack_received == 0) {
    receive_gap = 0;
  } else {
    receive_gap = timestamp_ack_received - last_ack_received;
  }

  // If above timeout, consider as congestion signal and MD
  // Otherwise, AI 1/cw
  if (receive_gap > timeout_ms()) {
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
  return TIMEOUT_MS; /* timeout of one second */
}

