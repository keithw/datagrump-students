#include <stdio.h>
#include <math.h>
#include <queue>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
double cwind;
//std::queue<int>  runmean;

//
double rtttarget=50;
double intcoef=0;
double propcoef=0.1;
double derivcoef=0.01;
double interr=0;
double lerr=0;

//double resolution = 200;
//double rtt=40;
double rttsum=400;
double rttn=10;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  cwind = 1;
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  int cint = (int)cwind;
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), cint );
  }

  return cint;
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
  double thisrtt = recv_timestamp_acked-send_timestamp_acked;
  double err = rtttarget-thisrtt;
  double derr= lerr-err;
  lerr=err;
  interr+=err;
  double delta=intcoef*interr+propcoef*err+derivcoef*derr;
  fprintf (stderr, "err%f, delta %f, cwnd %f \n",err,delta,cwind);
  cwind+=delta;
  //if(cwind>1000){cwind=1000;}
  if(cwind<0){cwind=0;}
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock diff of %d).\n",
	     send_timestamp_acked, recv_timestamp_acked , (int)(recv_timestamp_acked-send_timestamp_acked));
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
