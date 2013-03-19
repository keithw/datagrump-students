#ifndef AIMD_HH
#define AIMD_HH

#include <stdint.h>
#include "controller.hh"

/* Flow controller interface */
class AIMDController : public Controller
{
private:
  double ai_coeff;
  double md_coeff;

  double inc_counter;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  AIMDController( const bool debug, const unsigned int cwnd, const double ai_coeff, const double md_coeff );

  /* A packet was sent */
  void packet_was_sent( const uint64_t sequence_number,
			const uint64_t send_timestamp, bool is_retransmit );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

};

#endif
