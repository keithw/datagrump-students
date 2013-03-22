#ifndef CONTROLLER_HH
#define CONTROLLER_HH
#include <queue>
#include <list>

#include <stdint.h>

/* Flow controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
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



  double cwind;
  std::queue<int>  runmean;
  std::list<uint64_t> packetBalance;
  double resolution;
  double resolutionLR;
  double rtt;
  double rttsum;
  double rttn;
  double ackTracker ;
  double ackLastDelta ;
  uint64_t lastAck ;
  bool networkDown;
  unsigned int recovery ;
  unsigned int lastPB ;
  unsigned int lastcint ;
  unsigned int lastcwind;
  uint64_t start_time ;
  double rho;

  double estimateParameters();
  void refineParameters(const uint64_t sequence_number_acked,
                        /* what sequence number was acknowledged */
                        const uint64_t send_timestamp_acked,
                        /* when the acknowledged packet was sent */
                        const uint64_t recv_timestamp_acked,
                        /* when the acknowledged packet was received */
                        const uint64_t timestamp_ack_received );
  void refineModulation(const uint64_t sequence_number_acked,
                        /* what sequence number was acknowledged */
                        const uint64_t send_timestamp_acked,
                        /* when the acknowledged packet was sent */
                        const uint64_t recv_timestamp_acked,
                        /* when the acknowledged packet was received */
                        const uint64_t timestamp_ack_received );

  int chompWindow(unsigned int cint, double cwindDL);
  void markBeginning(const uint64_t start_sequence_number, const uint64_t end_sequence_number);
};

#endif
