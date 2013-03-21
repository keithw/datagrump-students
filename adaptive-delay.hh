#ifndef ADAPTDELCONTROLLER_HH
#define ADAPTDELCONTROLLER_HH

#include <stdint.h>
#include "controller.hh"
#include <deque>

#define PROPDELAY 20
#define MINTX 0.6

/* Flow controller interface */

class PacketData
{
public:
  const uint64_t sequence_number;
  const uint64_t send_timestamp;
  double recv_timestamp;
  bool was_queued;

  PacketData(const uint64_t sequence_number, const uint64_t send_timestamp) 
    : sequence_number(sequence_number), send_timestamp(send_timestamp), recv_timestamp(0), was_queued(false) 
  {};
};


class AdaptDelayController : public Controller
{
private:
 const uint64_t delay_threshold; 
 double tx_delay;

 std::deque<PacketData> sim_queue;
 std::deque<PacketData> recv_queue;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  AdaptDelayController( const bool debug, const unsigned int cwnd, const uint64_t delay_threshold );

  /* A packet was sent */
  virtual void packet_was_sent( const uint64_t sequence_number,
			const uint64_t send_timestamp, bool is_retransmit );

  /* An ack was received */
  virtual void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );
};

#endif
