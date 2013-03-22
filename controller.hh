#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <unordered_map>

/* Flow controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  uint64_t cwnd;
  uint64_t count;

  // Retransmit timer calculation used from CAC by Jacobson & Karels
  uint64_t sa;
  uint64_t sv;
  uint64_t rto;

  int64_t sasa;
  uint64_t previousSA;
  uint64_t minRTT;
  // Map packet sequence numbers to sent timestamp
  std::unordered_map<uint64_t, uint64_t> sendTimestamps;

  const uint64_t RTT_THRESHOLD_MS = 100;
  const uint64_t CWND_MIN = 1;

  bool slowStart;

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in packets */
  unsigned int window_size( void );

  void set_window_size(uint64_t new_size);

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
