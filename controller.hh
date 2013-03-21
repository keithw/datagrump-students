#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <stdint.h>

enum ControllerType { CONSTCWND, AIMD, DELAY, ADAPTIVEDELAY };

/* Flow controller interface */

class Controller
{
protected:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
	unsigned int cwnd;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  Controller( const bool debug, const unsigned int cwnd );

  /* Virtual Destructor for inheritance*/
  virtual ~Controller(){};

  /* Get current window size, in packets */
  unsigned int window_size( void );

  /* A packet was sent */
  virtual void packet_was_sent( const uint64_t sequence_number,
			const uint64_t send_timestamp, bool is_retransmit );

  /* An ack was received */
  virtual void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* How long to wait if there are no acks before sending one more packet */
  unsigned int timeout_ms( void );
};

#endif
