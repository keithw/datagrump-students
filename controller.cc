#include <stdio.h>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), window(15), window_float(15), timeout(1000),
  timeout_float(1000), rtt(0), srtt(0), alpha(0.8), dev(0), rttdev(0),
  beta(0.8), rtt_rec({0,0,0,0,0})
{
	/*window = 15;
	window_float = 15;
	timeout = 1000;
	timeout_float = 1000;
	rtt = 1000;
	srtt = 1000;
	alpha = 0.5;*/
	
  /*unsigned int window = 15;
  unsigned int timeout = 1000;
  float window_float = 15;
  float timeout_float = 1000;
  uint64_t srtt = 1000;
  uint64_t rtt = 1000;
  float alpha = 0.5;*/
  
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  //int the_window_size = 15;

  if ( debug_ ) {
    /*fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );*/
	fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), window );
  }

  //return the_window_size;
  window = (int) window_float;
  return window;
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
  
  
  rtt = timestamp_ack_received - send_timestamp_acked;
  int rsize = 5;//sizeof(rtt_rec)/sizeof(float);
  
  /*if (srtt == 0){
  	srtt = rtt;
  }*/
  
  srtt = (alpha*rtt) + ((1-alpha)*srtt);
  dev = rtt - srtt;
  dev = dev>0?dev:0-dev;
  rttdev = (beta*dev) + ((1-beta)*rttdev);
  timeout_float = srtt + (4*rttdev);
  
  float avg = 0;
  for (int n=0 ; n<rsize ; n++ ){
  	avg = avg + (rtt_rec[n]/rsize);
  }
  
  if (rtt > avg){
  	window_float = window_float + (2.0/window);
  }
  else{
  	window_float = window_float - (0.5/window);
  }
  
  for (int n=0 ; n<(rsize-1) ; n++ ){
  	rtt_rec[n] = rtt_rec[n+1];
  }
  rtt_rec[rsize-1] = rtt;
  
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
	timeout = (int) timeout_float;
	return timeout;
  //return 1000; /* timeout of one second */
}

void Controller::timout_detected(void)
{
	window_float = window_float/2;
}