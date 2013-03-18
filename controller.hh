#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>

/* Flow controller interface */

class Controller
{
public:
  struct ConfigParams {
    double AI;
    double MD;
    double AVG;
  };

private:
  bool debug_; /* Enables debugging output */
  
  /* Add member variables here */ 
  double w_size_;
  double rtt_last_;
  double rtt_min_;
  double rtt_max_;
  double rtt_avg_;
  double rtt_ratio_;
  ConfigParams params_; /* Params for AIMD and beyond. */
  
public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Loads all params from file */
  void LoadParams(const char* filename);

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

  /* Packet timed out */
  void packet_timed_out(void);
  
  /* Update RTT statistics after ack received */
  void update_rtt_stats(double rtt);
};
  
  
#endif
