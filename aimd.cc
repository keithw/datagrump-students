#include <stdio.h>

#include "aimd.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
AIMDController::AIMDController( const bool debug, const unsigned int cwnd, const double ai_coeff, const double md_coeff )
  : Controller(debug, cwnd), ai_coeff(ai_coeff), md_coeff(md_coeff), inc_counter(0)
{
}

/* A packet was sent */
void AIMDController::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp, bool is_retransmit )
                                  /* in milliseconds */
{
  /* Default: take no action */
  if ( debug_ ) {
    fprintf( stderr, "Att time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }

  if(is_retransmit){
    cwnd = (unsigned int) cwnd/md_coeff;
    fprintf( stderr, "At time %lu, MD: cwnd %d inc_cnt %g \n", send_timestamp, cwnd, inc_counter);
  }
}

/* An ack was received */
void AIMDController::ack_received( const uint64_t sequence_number_acked,
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
    fprintf( stderr, "Att time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }

  inc_counter += ai_coeff;
  if(inc_counter >= cwnd){
    inc_counter -= cwnd;
    ++cwnd;
  }

 fprintf( stderr, "At time %lu, AI: cwnd %d inc_cnt %g \n",timestamp_ack_received, cwnd, inc_counter);
}

