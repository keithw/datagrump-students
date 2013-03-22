#include <stdio.h>
#include <map>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
std::map<uint64_t,uint64_t> unacked;
std::map<uint64_t, uint64_t>::iterator it;
float ws = 1.0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  //makes sure window s
  int the_window_size = ((int)ws >= 1 ? (int) ws: 1);
  
  //make sure none of the outstanding packets have timed out.
  uint64_t delta;
  for(it=unacked.begin(); it != unacked.end(); it++) {
    delta = timestamp() - (it->second);
    if (delta > timeout_ms()){
      ws = ws/2.0; //halve window size
      fprintf( stderr, "Pkt number %lu timed out\n", (it->first));
      unacked.erase(it->first);
    }
  }

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }

  return the_window_size;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{

  //add sent pkt to dict
  unacked[sequence_number] = send_timestamp;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{

  //remove received packet from map
  if(unacked.find(sequence_number_acked) != unacked.end()){
    unacked.erase(sequence_number_acked);//erase the entry, if it existed (might not after timeout)
    ws+=(2.0/ws);
    else fprintf(stderr, "Ack not found in outstanding\n");
  } else fprintf(stderr, "Ack not found in outstanding\n");
  
  

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
