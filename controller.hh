#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>
#include <unordered_map>

/* Flow controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  
  /* Add member variables here */
  double cwnd_frac;
  double md_factor;
  double ai_factor;  
  uint64_t min_rtt;
  uint64_t prev_timeval;
  bool slow_start;

  // maps outstanding packet seq_num to time_sent
  std::unordered_map<uint64_t, uint64_t> sent_times; 
 
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

  /* Search through outstanding packets for timeouts */
  void check_timeout( void );

  /* Additive Increase */
  void ai( void );

  /* Multiplicative Decrease */
  void md( void );
};

#endif
