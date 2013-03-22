#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"


#define SCALLING_PAR 0.0

using namespace Network;
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug, unsigned int max_window_size, unsigned int max_delay, float scaling_factor_delay, float multi_dsf )
  : debug_( debug ), the_window_size(1), max_window_size_(max_window_size), 
    max_delay_(max_delay),  scaling_factor_delay_(scaling_factor_delay),
    multi_dsf_(multi_dsf), control_scheme(Controller::ControlSchemes::DELAY_SCALING),
    past_Delays()
{
  for(int i = 0; i< 4; i++)
    past_Delays.push_back(-1);
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{

 /* Default: fixed window size of one outstanding packet */
  if ( debug_ ) {
    fprintf( stderr, "-----window_size = %d.\n", the_window_size );
  }
  if (control_scheme == Controller::ControlSchemes::FIXED)
    return max_window_size_;
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
    else if(control_scheme == Controller::ControlSchemes::DELAY){
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
    else if (control_scheme == Controller::ControlSchemes::FIXED){
      //do nothing
      return;
    }
    else if(control_scheme == Controller::ControlSchemes::DELAY_SCALING){
      const uint64_t delay = recv_timestamp_acked - send_timestamp_acked;
      //calculate direction
      int direction = 0;
      if(past_Delays[0] >=0){
	direction = past_Delays[0] - (int)delay;
	if(debug_){
	  fprintf(stderr, "delay = %lu and direction = %d.\n", delay, direction);
	}
      }
      //update the window size
      unsigned int average_delay = delay; //*3 + past_Delays[0];
      float aux_window_size = 0;
      if(average_delay < max_delay_){
	if((int)delay <= (int)past_Delays[0]){
	  //float delay_decrease = (past_Delays[0] - delay)/past_Delays;
	  //float delay_ratio = (float)(max_delay_ - delay)/(float)max_delay_;
	  aux_window_size = the_window_size + 1; //scaling_factor_delay_*delay_ratio;// + multi_dsf_*delay_ratio;
	}else{
	  aux_window_size = the_window_size; // + 1;
	  //aux_window_size = the_window_size; // + scaling_factor_delay_*direction;
	}      
      }else{
	//if((int)delay < (int)past_Delays[0])
	//  aux_window_size = the_window_size - 1;
	//else
	//  aux_window_size = 1; //the_window_size + multi_dsf_*scaling_factor_delay_*direction;
	if((int)delay <= (int)past_Delays[0]){
	  aux_window_size = the_window_size/2;
	}else{
	  aux_window_size = 0; //the_window_size + multi_dsf_*scaling_factor_delay_*direction;
	  //aux_window_size = the_window_size -1 ;
	}
      }
      if(aux_window_size < 1)
	the_window_size = 1;
      else if( aux_window_size > max_window_size_)
	the_window_size = max_window_size_;
      else
	the_window_size = (unsigned int) aux_window_size;
      //update past delays
      for (unsigned int i =0; i < past_Delays.size()-1; i++){
	past_Delays[i+1] = past_Delays[i];
      }
      past_Delays[0] = delay;
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
    if (control_scheme == Controller::ControlSchemes::FIXED)
      return;

      // multiplicative increase
      if (the_window_size/2 > 1)
          the_window_size /= 2;
      else
          the_window_size = 1;
}
