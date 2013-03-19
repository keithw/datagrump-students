#include <stdio.h>


#include "delaycontroller.hh"
#include "controller.hh"
#include "timestamp.hh"
using namespace Network;

/* Default constructor */
DelayController::DelayController( const bool debug, const unsigned int cwnd, const uint64_t delay_threshold )
  : Controller(debug, cwnd), delay_threshold(delay_threshold)
{
}


/* An ack was received */
void DelayController::ack_received( const uint64_t sequence_number_acked,
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
  const uint64_t rtt = timestamp_ack_received-send_timestamp_acked;
  
  if (rtt > delay_threshold){
	cwnd = cwnd-1;
  }
  else{
	cwnd = cwnd+1;
  }
}



