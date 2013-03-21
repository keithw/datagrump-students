#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>
#include <deque>

/* Flow controller interface */

class Controller
{
public:
  struct ConfigParams {
    double AI;
    double MD;
    double AVG;
    double ack_interval_size;
  };

  class Ack {
    public:
      uint64_t send_;
      uint64_t recv_;
      uint64_t acked_;
    
      Ack(const uint64_t send, const uint64_t recv, const uint64_t acked) 
        : send_(send), recv_(recv), acked_(acked) {}
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

  uint64_t initial_timestamp_;
  uint64_t last_packet_sent_;
  double capacity_estimate_; /* recent_acks / time_frame */
  double capacity_avg_;
  double queue_estimate_;

  std::deque<Ack> acks_; /* Keeps record of all the acks received recently */
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

  /* Update capacity estimates after ack received */
  void update_capacity_stats(
      const uint64_t timestamp, const uint64_t current_ack);
 
};
  
  
#endif
