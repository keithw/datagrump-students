#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

#include <iostream>
#include <deque>
#include <algorithm>    // std::max

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

int the_window_size = 10;

std::deque<uint64_t> packet_times;
std::deque<uint64_t> time_diffs;

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }
  //fprintf( stderr, "asdfasfd",);
  if (the_window_size == 0) { the_window_size = 10; }
  return the_window_size;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  packet_times.push_back(send_timestamp);
  /* Default: take no action */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }
}

int cwnd_fraction = 0;
uint64_t avg_rtt = 0;
int multi_dec_cooldown = 0;
int max_samples = 0;
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
  // calculate time it took to send packet and receive ack,
  // using saved packet transmission times; saves durations in deque
  // assumes acks are recieved in order packets were sent
  uint64_t diff = timestamp_ack_received - packet_times.front();
  time_diffs.push_front(diff);
  packet_times.pop_front();
  fprintf(stdout, "new ack diff: %lu , cwnd: %i ,", diff, the_window_size);

  // loop through up to first 10 diffs to get average RTT over most recent acks
  max_samples = std::min((int) time_diffs.size(),10); // 10 samples or less
  avg_rtt = 0;
  for(int i=0; i<max_samples; i++) {
    avg_rtt += time_diffs[i];
  }
  avg_rtt /= max_samples;
  fprintf(stdout, " new avg: %lu \n", avg_rtt);
  
  // Congestion detection
  if (avg_rtt > 200) { // TODO: MAGIC NUMBER
    if (multi_dec_cooldown == 0) { // multiplicative decrease
      the_window_size /= 2; // TODO: MAGIC NUMBER FROM TCP
      multi_dec_cooldown = 2; // enter cooldown period TODO: MAGIC NUMBER
      time_diffs.clear(); // clear timestamp diff duration array to react more quickly after decrease
    } else {// in cooldown period, skip mul. dec. so cwnd doesn't collapse
      multi_dec_cooldown--;
    }
  }
  
  // ACK received, do additive increase
  // increment by 1/cwnd, use cwnd_fraction to keep track of fractional increment
  // of cwnd, until add up to one cwnd size.
  cwnd_fraction++;
  if (cwnd_fraction >= the_window_size) {
    cwnd_fraction = 0;
    the_window_size += 1;
  }
  
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
  return 700; /* timeout of one second */
}
