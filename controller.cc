#include <stdio.h>
#include <set>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

const double Controller::DELAY_TRIGGER_DECREASE = 15.0f;
const double Controller::DELAY_TRIGGER_INCREASE = 6.0f;
const double Controller::SUPER_DELAY_TRIGGER_INCREASE = 10.0f;

/* Default constructor */
Controller::Controller( const bool debug )
    : probes_count(0), should_acked(), debug_( debug ), mode (MODE_CONTEST),
      the_window_size(BASELINE_WINDOW_SIZE)
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  // int the_window_size = 20;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %f.\n",
	     timestamp(), the_window_size );
  }

  return (int)the_window_size;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  should_acked.push(std::make_pair(
				   -(send_timestamp + DELAY_UPPER_BOUND ),
				   sequence_number )
		    );
    
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
			       const uint64_t timestamp_ack_received)
                               /* when the ack was received (by sender) */
{
  // compute the round trip time
  uint64_t to_time = recv_timestamp_acked - send_timestamp_acked;
  uint64_t from_time = timestamp_ack_received - recv_timestamp_acked;
  uint64_t rtt = to_time + from_time;
  
  last_acked = sequence_number_acked;
 
  /*
  last_acked_diff = timestamp_ack_received - last_acked_time;
  if (!last_acked_diff) {
    last_acked_count++;
  } else {
    last_acked_count = 0;
  }
  fprintf(stderr, 
	  "last_acked_count %lu win size %f\n", last_acked_count, the_window_size);
  last_acked_time = timestamp_ack_received;
  */

  if (late_packet_count <= 10) {
    if ((int)rtt <= DELAY_LOWER_BOUND) {
      the_window_size += 
	SUPER_DELAY_TRIGGER_INCREASE 
	/ the_window_size;
    } else if ((int) rtt <= DELAY_LOWER_BOUND2) {
      double diff = (DELAY_LOWER_BOUND2 - DELAY_LOWER_BOUND);
      the_window_size += 
	(1.0 - (rtt - DELAY_LOWER_BOUND) / diff)  
	* DELAY_TRIGGER_INCREASE
	/ the_window_size;
    }
  }

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );
    
    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}

void Controller::preempt_decrease ( 
    const uint64_t current_time )
{
    late_packet_count = 0;
    while ( !should_acked.empty() ) {    
	uint64_t
	    sha = -should_acked.top().first,
	    id = should_acked.top().second;
	    
	if (sha > current_time ) {
	    // nothing fancy going on here.
	    break;
	}
     
	if (id > last_acked) {
	  // preemptive algorithm
	  late_packet_count += 1;
	}
	should_acked.pop();
    }

    the_window_size -= DELAY_TRIGGER_DECREASE * late_packet_count;   
    if (the_window_size < CONSERVATIVE_WINDOW_SIZE) 
      the_window_size = CONSERVATIVE_WINDOW_SIZE;

    /*
    if (late_packet_count >= 6) {
      the_window_size = 1;
      if (the_window_size < CONSERVATIVE_WINDOW_SIZE) 
	the_window_size = CONSERVATIVE_WINDOW_SIZE;
    }
    */

    if (late_packet_count > 0) {
      fprintf(stderr, "Packet destined to be late, Cutting down window size to %f.\n", 
	      the_window_size);
    }
}

void Controller::acknowledgment_timeout( void ) {
 
    if ( debug_ ) {
	fprintf( stderr,"Received acknowledgment timeout.\n" );
    }
    
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT;
}
