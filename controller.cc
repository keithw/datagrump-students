#include <stdio.h>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), window(40), window_float(40.0), timeout(1500),
  rtt(0), srtt(0), alpha(0.4), dev(0), rttdev(0),
  beta(0.4), rtt_rec{0,0,0}, rsize(sizeof(rtt_rec)/sizeof(float)),
  avg(0), ratio(0), wb(2)
{ 
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  //int the_window_size = 15;
  
  if (window_float<=0){window_float = 1.0;}
  window = (unsigned int) window_float;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size_float = %f.\n",
	     timestamp(), window_float );
	fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), window );
  }

  //return the_window_size;
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
  //int rsize = 5;//sizeof(rtt_rec)/sizeof(float);
  
  /*if (srtt == 0){
  	srtt = rtt;
  }*/
  
  srtt = (alpha*rtt) + ((1-alpha)*srtt);
  dev = rtt - srtt;
  dev = dev>0?dev:0-dev;
  rttdev = (beta*dev) + ((1-beta)*rttdev);
  timeout = srtt + (4*rttdev);
  
  avg = 0;
  for (int n=0 ; n<rsize ; n++ ){
  	avg = avg + rtt_rec[n];
  }
  avg = avg/rsize;
  
  int avg_i = (int) avg;
  int rtt_i = (int) rtt;
  int diff = (avg_i-rtt_i)>0?(avg_i-rtt_i):(rtt_i-avg_i);
  if (avg_i != 0){
  	ratio = 4.0*diff/(1.0*avg_i);
  }
  if (rtt < (avg)){
  	//window_float = (1.0+(1.0*(avg_i-rtt_i)/avg_i))*window_float;// + (1.0/window);
  	//window_float = window_float + (4.75/window_float);
  	window_float = window_float + ((wb+ratio)/window_float);
  }
  else{
  	//window_float = (1.0*(rtt_i-avg_i)/avg_i)*window_float;// - (1.5/window);
  	//window_float = window_float - (2.75/window_float);
  	window_float = window_float - ((wb+ratio)/window_float);
  }
  
  for (int n=0 ; n<(rsize-1) ; n++ ){
  	rtt_rec[n] = rtt_rec[n+1];
  }
  rtt_rec[rsize-1] = rtt;
  
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
	fprintf( stderr, "rtt = %u.\n", rtt );
	fprintf( stderr, "srtt = %u.\n", srtt );
	fprintf( stderr, "dev = %u.\n", dev );
	fprintf( stderr, "rttdev = %u.\n", rttdev );
	fprintf( stderr, "avg = %u.\n", avg );
	fprintf( stderr, "ratio = %f.\n", ratio );
	fprintf( stderr, "timeout = %u.\n", timeout );
	return timeout;
  //return 1000; /* timeout of one second */
}

void Controller::timout_detected(void)
{
	if ( debug_ ) {
    fprintf( stderr, "Timeout Detected. \n" );
  }
  
	window_float = window_float/2;
}

void Controller::debugging(int n)
{
	if ( debug_ ) {
    fprintf( stderr, "debugging code = %d.\n", n );
  	}
}