#include <stdio.h>
#include <iostream>
#include <math.h>

#include "controller.hh"
#include "timestamp.hh"

#define max(a,b) a > b ? a : b

using namespace Network;
const int countdown_max = 4;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), w_size( 15.0 ), prev_rtt( 1.0 ), countdown( countdown_max ), fast( false )
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  //  int the_window_size = 1;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %f.\n",
	     timestamp(), w_size );
  }

  return max((unsigned int)(w_size + 0.5), 1) ;
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

  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  double rtt_change = rtt / (double)prev_rtt;

  if (rtt_change < 1.0) {
    if (countdown == 0) {
      countdown = countdown_max;
      fast = true;
    } else {
      countdown--;
      fast = false;      
    }
  } else {
    fast = false;
    countdown = countdown_max;
    prev_rtt = rtt;
  }

  if (fast) {
    if (rtt_change == 0) {
      w_size = 15;
    } else {
      w_size = 5.0 / sqrt(rtt_change);
    }
  } else {
    w_size = 1;
  }

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
