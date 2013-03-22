#include <stdio.h>
#include <set>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

const double Controller::ADDITIVE_INCREASE = 0.2f;
const double Controller::MULTIPLICATIVE_DECREASE = 10;

const double Controller::DELAY_TRIGGER_DECREASE = 4.0f;
const double Controller::DELAY_TRIGGER_INCREASE = 15.0f;
const double Controller::SUPER_DELAY_TRIGGER_INCREASE = 18.0f;

/* Default constructor */
Controller::Controller( const bool debug )
    : probes_count(0), acked(), should_acked(), debug_( debug ), mode (MODE_CONTEST),
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
    should_acked.push(
		      std::make_pair(
			  -(send_timestamp + DELAY_UPPER_BOUND ),
			  sequence_number )
		      );
    
  /* Default: take no action */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }
}

void Controller::ack_received_fixed_window_size( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received)
                               /* when the ack was received (by sender) */
{
    
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}


void Controller::ack_received_aimd( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received)
                               /* when the ack was received (by sender) */
{
  
  the_window_size += ADDITIVE_INCREASE/the_window_size;
  
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}

void Controller::ack_received_delay_trigger ( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received)
                               /* when the ack was received (by sender) */
{
  // compute the round trip time
  //  uint64_t to_time = recv_timestamp_acked - send_timestamp_acked;
  //  uint64_t from_time = timestamp_ack_received - recv_timestamp_acked;
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  
  if ((int)rtt >= DELAY_UPPER_BOUND) {
        //decrease the window size by some amount
        the_window_size -= DELAY_TRIGGER_DECREASE;
        if (the_window_size <= CONSERVATIVE_WINDOW_SIZE)
            the_window_size = CONSERVATIVE_WINDOW_SIZE;
        
  } else if ((int)rtt <= DELAY_LOWER_BOUND) {
        the_window_size += DELAY_TRIGGER_INCREASE/the_window_size;
  }
  
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}


void Controller::ack_received_delay_contest ( const uint64_t sequence_number_acked,
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

  // Don't need this here anymore not to act twice.
  /*
  
  if ((int)rtt >= DELAY_UPPER_BOUND) {
        //decrease the window size by some amount
        the_window_size -= DELAY_TRIGGER_DECREASE;
        if (the_window_size <= CONSERVATIVE_WINDOW_SIZE)
            the_window_size = CONSERVATIVE_WINDOW_SIZE;
  */
    if (late_packet_count <= 10) {
      if ((int)rtt <= DELAY_LOWER_BOUND) {
        the_window_size += SUPER_DELAY_TRIGGER_INCREASE /the_window_size;
      } else if ((int)rtt >= DELAY_LOWER_BOUND && (int)rtt <= 2*DELAY_LOWER_BOUND) {
        the_window_size += ( 1 - (rtt - DELAY_LOWER_BOUND)/DELAY_LOWER_BOUND)*DELAY_TRIGGER_INCREASE/the_window_size;
      }
    }
  
  if (rtt > 0) {
    
  }
  
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
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
  acked.insert(sequence_number_acked);
  
  switch(mode) {
    case MODE_FIXED_WINDOW_SIZE:
        ack_received_fixed_window_size(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
        break;
    case MODE_AIMD:
        ack_received_aimd(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
        break;
    case MODE_DELAY_TRIGGER:
        ack_received_delay_trigger(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
        break;
    case MODE_CONTEST:
        ack_received_delay_contest(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
        break;
  }
  
  /*
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
  */
}

void Controller::preempt_decrease ( 
    const uint64_t current_time )
{
    //probes_count++;

    //bool preempt = false;
    late_packet_count = 0;
    
    while ( !should_acked.empty() ) {        
        
	uint64_t
	    sha = -should_acked.top().first,
	    id = should_acked.top().second;
	    
       
	if (sha > current_time ) {
	    // nothing fancy going on here.
	    break;
	}

    //fprintf(stderr, " id: %lu, sha: %lu,time: %lu.\n",id,sha,current_time);
     
	if (acked.find(id) != acked.end() /* && !preempt*/) {
	    // Instantiate preemptive algorithm;
        late_packet_count += 1;
        if (late_packet_count >= 10) {
            //fprintf(stderr,"Late packets: %d\n",late_packet_count);
        }
	    
	    // This is COPIED! need to factor out later.
	    the_window_size -= DELAY_TRIGGER_DECREASE;
	    if (the_window_size <= CONSERVATIVE_WINDOW_SIZE)
            the_window_size = CONSERVATIVE_WINDOW_SIZE;
	    
        fprintf(stderr, "Haven't seen packet for 150ms or something. Cutting down window size to %f.\n", the_window_size);
	    //preempt = true;
	}
	should_acked.pop();
    }
}

void Controller::acknowledgment_timeout( void ) {
 
    if ( debug_ ) {
	fprintf( stderr,"Received acknowledgment timeout.\n" );
    }
    
    switch(mode) {
        case MODE_AIMD:
            //we cut the window in half
            if (the_window_size >= CONSERVATIVE_WINDOW_SIZE)
                the_window_size /= (float)MULTIPLICATIVE_DECREASE;
                if (the_window_size < CONSERVATIVE_WINDOW_SIZE)
                    the_window_size = CONSERVATIVE_WINDOW_SIZE;
            else
                the_window_size = CONSERVATIVE_WINDOW_SIZE;
            
            break; 
    }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  switch (mode) {
    case MODE_AIMD:
        return AIMD_TIMEOUT;
        break;
  }
  return TIMEOUT; /* Be preemptive, utilizing sender's timeout; */
}
