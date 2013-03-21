#ifndef DELAYCONTROLLER_HH
#define DELAYCONTROLLER_HH

#include <stdint.h>
#include "controller.hh"


/* Flow controller interface */

class DelayController: public Controller
{
private:
 const uint64_t delay_threshold; 
 double inc_counter;
  

public:
  /* Public interface for the flow controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in datagrump-sender.cc) */

  /* Default constructor */
  DelayController( const bool debug, const unsigned int cwnd, const uint64_t delay_threshold  );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
                     const uint64_t send_timestamp_acked,
                     const uint64_t recv_timestamp_acked,
                     const uint64_t timestamp_ack_received );

};

#endif

