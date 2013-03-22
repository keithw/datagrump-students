#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>
#include <vector>
/* Flow controller interface */

class Controller
{
public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */
  enum ControlSchemes {FIXED, AIMD, DELAY, DELAY_SCALING};

  /* Default constructor */
  Controller( const bool debug, unsigned int max_window_size, unsigned int max_delay, float scaling_factor_delay, float multi_dsf );

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

  void packet_timed_out (void) ; 

private:
    bool debug_; /* Enables debugging output */
    
    /* Add member variables here */
    unsigned int the_window_size;
    unsigned int max_window_size_;
    unsigned int max_delay_;
    float scaling_factor_delay_;
    float multi_dsf_;
    ControlSchemes control_scheme;
    std::vector<int> past_Delays;

};

#endif
