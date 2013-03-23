#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>

#include <queue>

/* Flow controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */

  int64_t rttmin;

  // The time interval in ms during which packet receipts are counted to measure
  // the packet receipt rate.
  const uint64_t time_interval = 150;

  const float min_window_size = 1;
  const float stretch_factor = 2;

  // Stores the packet receipt times during the last 150ms.
  std::queue< uint64_t > packet_times;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* tells controller a timeout has occurred */
  void notify_timeout( void );

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
