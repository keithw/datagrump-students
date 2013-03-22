#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>
#include <set>
#include <queue>

/* Flow controller interface */
enum CONTROLLER_MODES { MODE_FIXED_WINDOW_SIZE, MODE_AIMD, MODE_DELAY_TRIGGER, MODE_CONTEST };

class Controller
{
public:
  // How many PROBE_TIMEOUTs have passed?
  uint64_t probes_count;

  static const int TIMEOUT = 100;
    
private:
  // What is acked, we might ask?!
  uint64_t last_acked = -1;

  uint64_t last_acked_time = 0;
  uint64_t last_acked_diff = 0;
  uint64_t last_acked_count = 0;
  // Sorted in time, who should be acked by that time
  std::priority_queue < std::pair < uint64_t, uint64_t > > should_acked;
    
  bool debug_; /* Enables debugging output */
  
  int mode;
    
  /* Delay trigger mode */
  static const int DELAY_UPPER_BOUND = 100;
  static const int DELAY_LOWER_BOUND = 40;
  static const int DELAY_LOWER_BOUND2 = 80;
  static const double DELAY_TRIGGER_DECREASE;
  static const double DELAY_TRIGGER_INCREASE;
  static const double SUPER_DELAY_TRIGGER_INCREASE;
  
  /* Add member variables here */
  static const int BASELINE_WINDOW_SIZE = 20;
  static const int CONSERVATIVE_WINDOW_SIZE = 1;
  
  int late_packet_count = 0;

  double the_window_size;
  
  void ack_received_fixed_window_size( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );
  
  void ack_received_aimd( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );
             
  void ack_received_delay_trigger ( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );
            
  void ack_received_delay_contest ( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );
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
  
  void acknowledgment_timeout( void);
  
  void preempt_decrease( const uint64_t current_time );
};

#endif
