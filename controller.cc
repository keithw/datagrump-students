#include <stdio.h>
#include <map>
#include <list>
#include <algorithm>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

//unsigned int window_size = 1;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    the_window_size ( 1 ),
    sent_pkts(),
    missing_pkts(),
    acked_pkts(),
    avg_rtt( 0 ),
    mean_dev(0),
    timeout(105),
    last_congested(false),
    lowest_timeout(10000),
    last_timestamp(0)
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  unsigned int winsize = (unsigned int) std::floor(the_window_size);
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), winsize );
  }

  //return the_window_size;
  return winsize;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  /* Default: take no action */

  // add sequence number and timestamp to sent list
  sent_pkts[sequence_number] = send_timestamp;

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
  /* Default: take no action */

  std::list<uint64_t> timedout_pkt_nos;
  uint64_t successful_pkt_no;
  bool congestion = false;
  //double alpha = 0.6;
  double max_window_size=50;
  double min_window_size=1;
  //bool received_dupack = false;

  // calculate timeout value
  int current_rtt = (int) (timestamp_ack_received - send_timestamp_acked);
  int this_rtt = current_rtt - (avg_rtt >> 3);
  avg_rtt += this_rtt;
  if (this_rtt < 0){
    this_rtt = -this_rtt;
  }
  this_rtt -= (mean_dev >> 2);
  mean_dev += this_rtt;
  timeout = (avg_rtt >> 3) + mean_dev;
  lowest_timeout = std::min(timeout, lowest_timeout);
  lowest_timeout = std::max(lowest_timeout, 125);

  for (std::map<uint64_t,uint64_t>::iterator it=sent_pkts.begin(); it!=sent_pkts.end(); ++it) {
    /* if duplicate ack, doubly increase timeout*/

    //receive packet num greater than some packet in sent
    if (it->first < sequence_number_acked) {
      // move out of order packets to missing list
      if ( timestamp_ack_received - it->second > timeout_ms() /*avg_rtt *timeout_padding*/ ) {
        congestion = true;
        timedout_pkt_nos.insert (timedout_pkt_nos.begin(), it->first );
      }
    }

    
    //receive in-order packet
    else if (it->first == sequence_number_acked ) {
      // mark successful packet for deletion
      successful_pkt_no = sequence_number_acked;
      // update timeout using ewma
      //avg_rtt = (alpha * avg_rtt / (1-alpha) + (timestamp_ack_received - it->second) ) * (1-alpha);
    }
    // check if there is a timeout for all packets
    if ( timestamp_ack_received - it->second > timeout_ms() ){
      congestion = true;
    }

    // don't need to do anything else (if number_acked is lower than others sent)
  }
  // remove successful packet from sent list
  sent_pkts.erase (sent_pkts.find(successful_pkt_no));

  // remove timed out packets from missing_pkts
  for (std::list<uint64_t>::iterator it=timedout_pkt_nos.begin(); it!=timedout_pkt_nos.end(); ++it) {
    missing_pkts.erase ( missing_pkts.find(*it) );
  }

  // update window size
  if (congestion){
    the_window_size = the_window_size * 0.55;
  } 
  else {
    // increase window more if time difference is small
    uint64_t time_diff = (recv_timestamp_acked - last_timestamp);
    int k;
    if (last_timestamp == 0 || time_diff > 10)
      k = 1;
    else
      k = 2;
    the_window_size = the_window_size + k/the_window_size;
  }
  the_window_size = std::min(max_window_size, the_window_size);
  the_window_size = std::max(min_window_size, the_window_size);
  last_congested = congestion;
  last_timestamp = recv_timestamp_acked;


  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );

    unsigned int winsize = std::floor(the_window_size);
    fprintf( stderr, "Window size: %u  congestion: %s  avg_rtt: %u  time_transmitting: %u  time_ack_receiver_to_sender: %d  total_time: %d  timeout_calculated: %u\n", winsize,
             (congestion)?"true":"false",  avg_rtt,
             (unsigned int)(recv_timestamp_acked - send_timestamp_acked),
             (int)(timestamp_ack_received - recv_timestamp_acked),
             (int)(timestamp_ack_received - send_timestamp_acked),
             (unsigned int)timeout);
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  //return 1000; /* timeout of one second */
  return std::min((unsigned int)lowest_timeout, (unsigned int)timeout);
}
