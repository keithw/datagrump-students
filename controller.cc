#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

#include <iostream>
#include <deque>

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

int the_window_size = 10;

std::deque<uint64_t> packet_times;
std::deque<uint64_t> time_diffs;

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

uint64_t last_ack_seq_num = 0;
/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  packet_times.push_back(send_timestamp);
  /* Default: take no action */


  /*  if (sequence_number <= last_ack_seq_num) {
    // retransmission due to timeout, packetloss, drop?
    // i.e. detect tcp cong., need to decrease cwnd by multiplicative factor
    the_window_size /= 2;
    fprintf(stdout, "Timeout and retransmitting packet with seq_num %lu \n", sequence_number);

  } else {
    // normal transmission
  }
  */

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
  uint64_t diff = timestamp_ack_received - packet_times.front();
  time_diffs.push_front(diff);
  packet_times.pop_front();

  // loop through first 10 diffs to get average RTT
  uint64_t avg_rtt = 0;

  fprintf(stdout, "diff: %lu , time_diffs[0]: %lu , cwnd: %i \n", diff, time_diffs[0], the_window_size);

  if(time_diffs.size() >= 10) {
  for(int i=0; i<10; i++) {
    avg_rtt += time_diffs[i];
  }
  avg_rtt /= 10;
  } else {    
    //
  }

  if (avg_rtt > 100) {
    // multiplicative decrease
    the_window_size /= 2;
  } else {

  // ACK received, do additive increase
  // increment by 1/cwnd, use cwnd_fraction to keep track of fractional increment
  // of cwnd, until add up to one cwnd size.
    cwnd_fraction++;
    if (cwnd_fraction >= the_window_size) {
      cwnd_fraction = 0;
      the_window_size += 1;
    }
  }


  last_ack_seq_num = sequence_number_acked+1; // remove?
  
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
