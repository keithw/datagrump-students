#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>

#include <map>

/* Flow controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */

  // The current window size.
  double my_window_size_;

  // A multimap of <sequence number, send timestamps> of the outstanding
  // packets.
  std::multimap<uint64_t, uint64_t> sent_packet_timestamps_;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in packets */
  unsigned int window_size( void );

  /* A packet was sent */
  void packet_was_sent( const uint64_t sequence_number,
			const uint64_t send_timestamp );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* How long to wait if there are no acks before sending one more packet */
  unsigned int timeout_ms( void );
};

#endif
