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
//////// std:: map<uint64_t,uint64_t> packet_sent_times;
  float curr_window_size;
  
////  uint64_t last_window_size_change;
////  bool sent_once;
////  bool in_initialization = true;
////  bool in_getting_first_pkts_phase = true;
////  uint64_t prev_delivery_time;
////  uint64_t prev_delivery_times[10];
////  size_t count_prev_pkts = 0;
////  uint64_t curr_delivery_times[10];
////  size_t count_curr_pkts = 0;
////  size_t num_pkts_to_update = 10;
////  size_t count_pkts_since_last_update;

  uint64_t timeout_time;

  int packets_in_queue;
  int packet_counter;
  int time_cutoff;
  int measured_rate;
  int rate_difference;
  int skip_counter;
  uint64_t last_receive_time;
  int64_t packet_gap;
  int64_t rttmin;
  const uint64_t time_interval = 100;

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
