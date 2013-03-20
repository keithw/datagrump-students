#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"


#define DELAY_THRESH 40
#define SCALLING_PAR 0.0
#define MAX_WIN_SIZE 30


using namespace Network;
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, unsigned int max_window_size, unsigned int max_delay )
  : debug_( debug ), the_window_size(1), max_window_size_(max_window_size), 
    max_delay_(max_delay), control_scheme(Controller::ControlSchemes::DELAY)
    
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
 /* Default: fixed window size of one outstanding packet */
  if ( debug_ ) {
    fprintf( stderr, "-----window_size = %d.\n", the_window_size );
  }
  return the_window_size;

}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  /* Default: take no action */
  if ( 0 ) {
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

  if ( 0 ) {
     fprintf( stderr, "At time %lu, received ACK for packet %lu",
     	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
    	     send_timestamp_acked, recv_timestamp_acked );
  }
  // additive increase
    if(control_scheme == Controller::ControlSchemes::AIMD){
        if (the_window_size + 1 > max_window_size_)
            the_window_size = max_window_size_;
        else
            the_window_size += 1;
    }
   //delay-triggered scheme
    if(control_scheme == Controller::ControlSchemes::DELAY){
        const uint64_t delay = recv_timestamp_acked - send_timestamp_acked;
	if ( debug_ ) {
	  fprintf( stderr, "-----------------  delay = %lu.\n", delay );
	}
        if(delay < max_delay_){
            if (the_window_size + 1 > max_window_size_)
                the_window_size = max_window_size_;
            else
                the_window_size += 1;
        } else {
             // multiplicative increase
            if (the_window_size/2 > 1)
                the_window_size /= 2;
            else
                the_window_size = 1;
        }
    }

}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}

void Controller::packet_timed_out( void)
{
    if(debug_){
      fprintf(stderr, "!!!! ----- packet timed out");
    }
      // multiplicative increase
      if (the_window_size/2 > 1)
          the_window_size /= 2;
      else
          the_window_size = 1;
}
