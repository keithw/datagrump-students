#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>

/* Flow controller interface */
enum CONTROLLER_MODES { MODE_FIXED_WINDOW_SIZE, MODE_AIMD, MODE_DELAY_TRIGGER, MODE_CONTEST };

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  
  int mode;
  
  static const int TIMEOUT = 1000;
  
  /* AIMD constants here */
  static const double ADDITIVE_INCREASE;
  static const double MULTIPLICATIVE_DECREASE ; //divide by this number
  static const int AIMD_TIMEOUT = 100;
  
  /* Delay trigger mode */
  static const int DELAY_UPPER_BOUND = 140;
  static const int DELAY_LOWER_BOUND = 50;
  static const double DELAY_TRIGGER_DECREASE;
  static const double DELAY_TRIGGER_INCREASE;
  
  /* Add member variables here */
  static const int BASELINE_WINDOW_SIZE = 10;
  static const int CONSERVATIVE_WINDOW_SIZE = 1;
  static const int DELAY_INCR_THRESHOLD = 150;
  static const int DELAY_DECR_THRESHOLD = 50;
  
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
  
  void acknowledgment_timeout( void );
};

#endif
