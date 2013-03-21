#include <stdio.h>
#include <math.h>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), curr_window_size(1), timeout_time(timestamp()-1000),
        packets_in_queue(0), packet_counter(0), time_cutoff(timestamp()),
        measured_rate(0), rate_difference(0), skip_counter(0),
        last_receive_time(timestamp()), packet_gap(100), rttmin(1000)
{
}

void Controller::notify_timeout( void ) {
  packets_in_queue--;
  if (packets_in_queue < 0)
    packets_in_queue = 0;
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  if (timestamp() > time_interval + time_cutoff) {
    rate_difference = measured_rate - packet_counter; 
    measured_rate = packet_counter;
    packet_counter = 0;
    time_cutoff += time_interval;
  }

  float rate = measured_rate * 1000 / time_interval;
  if (rate < 1000/time_interval)
    rate = 1000/time_interval;

/////  float rate_deriv = rate_difference * 1000 / time_interval;
/////  float safety = 0.2 * rate_deriv * ((rate_deriv > 0) ? 1 : -1);

/////  float stretch = 4;

  float actual_rttmin = rttmin/1000;
  if (actual_rttmin < 0.001)
    actual_rttmin = 0.001;
  else if (rttmin > 1)
    actual_rttmin = 1;

  float queue_target = 1 + 0.11*rate;
/////+0*(rate_deriv-safety); ////stretch * actual_rttmin * (rate );////+ rate_deriv - safety);
/*
  if (packet_gap > 9*1000/rate) {
    float drate = 1000 / packet_gap;
    float rate_diff = rate - drate;
    float base = 2;
    float ln_base = 0.7 + 0*base;
    float log_ratio = -(drate/rate - 1)/ln_base;
    queue_target -= stretch * actual_rttmin * rate_diff * log_ratio;
  } 
*/
/////  if (queue_target < 20)
/////    queue_target = 20;
/////  if (queue_target > 1000)
/////    queue_target = 1000;
  float win_size = queue_target - packets_in_queue;

  int the_window_size = (int)win_size;
  if (the_window_size < 1)
    the_window_size = 1;
  if (the_window_size > 500)
    the_window_size = 500;

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
  packets_in_queue++;

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
  uint64_t current_receive_time = timestamp_ack_received;
  packet_gap = current_receive_time - last_receive_time;
  last_receive_time = current_receive_time;

  if (timestamp_ack_received < rttmin + send_timestamp_acked )
    rttmin = (int64_t)timestamp_ack_received - (int64_t)send_timestamp_acked;

  packets_in_queue--;
  if (packets_in_queue < 0)
    packets_in_queue = 0;

  packet_counter++;

/*
  // if delay is greater than the threshold, decrease window size.
  //   Otherwise, increase the window size.
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  if (rtt > 50 && curr_window_size > 1)
    curr_window_size--;
  else
    curr_window_size++;
*/
/*
  // additive increase
  if (timestamp() >= timeout_time + 1000) {
    if (curr_window_size < 1)
      curr_window_size = 1;
    else
      curr_window_size += 1/(curr_window_size*curr_window_size);
  }
*/

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
  return 100;
}
