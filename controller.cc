#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

int the_window_size = 5;
/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }
  //fprintf( stderr, "asdfasfd",);
  return the_window_size;
}

uint64_t prev_sequence_number = 0;
/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  /* Default: take no action */
  if (sequence_number <= prev_sequence_number) {
    // retransmission due to timeout, packetloss, drop?
    // i.e. detect tcp cong., need to decrease cwnd by multiplicative factor
    the_window_size /= 2; //TCP_MULTI_DEC_FACTOR;
    fprintf(stdout, "Timeout and retransmitting packet with seq_num %lu \n", sequence_number);

  } else {
    // normal transmission, do nothing? will additive increase on acks
    // update latest sequence number
    // WRONG!!???    prev_sequence_number = sequence_number;
  }

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }
}

int cwnd_fraction = 0;
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
  // ACK received, do additive increase
  // increment by 1/cwnd, use cwnd_fraction to keep track of fractional increment
  // of cwnd, until add up to one cwnd size.
  cwnd_fraction++;
  if (cwnd_fraction >= the_window_size) {
    cwnd_fraction = 0;
    the_window_size += 1; //TCP_ADD_INC_FACTOR;
  }
  prev_sequence_number = sequence_number_acked+1;
  
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
  return 600; /* timeout of one second */
}
