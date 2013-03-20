#include <stdio.h>
#include <math.h>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), curr_window_size(1), last_window_size_change(timestamp()),
    sent_once(false), prev_delivery_time(0), prev_delivery_times(),
    count_pkts_since_last_update(0)
{
}

void Controller::notify_timeout( void ) {
  /*
  // MD
  curr_window_size = pow(curr_window_size, 0.25);
  if (curr_window_size <= 1)
    curr_window_size = 1;
  */
  
  if (in_initialization) {
    in_initialization = false;
    curr_window_size /= 2;
  }
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /*
  uint64_t time_delta = timestamp() - last_window_size_change;
  if (curr_window_size == 1) {
    curr_window_size = 0;
  }
  */
/*
    if (time_delta > 10000) {
      curr_window_size = 0;
      last_window_size_change = timestamp();
    }
  }
*/ /*
  else if (curr_window_size == 0 && !sent_once && time_delta > 5000) {
    curr_window_size = 1;
    sent_once = true;
  }
  */

/*    
    if (time_delta > 1000) {
      curr_window_size = 1;
      last_window_size_change = timestamp();
    }
  }
*/
  /*
  if (timestamp() - last_window_size_change > 10000) {
    curr_window_size = (curr_window_size == 0) ? 1 : 0;
    last_window_size_change = timestamp();
  }*/

  /* Default: fixed window size of one outstanding packet */
  int the_window_size = (int)curr_window_size;

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
  /* update packets sent times */
  /////////packet_sent_times[sequence_number] = send_timestamp;

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

  uint64_t curr_delivery_time = recv_timestamp_acked - send_timestamp_acked;
  
 /* 
  // if that's the first pkt, do nothing
  if (!sent_once) {
    sent_once = true;
    return;
  }
  */

  // slow start, update prev pkts times
  /*
  if (in_initialization) {
    if (count_prev_pkts < 10) {
      curr_window_size += 1/curr_window_size;
      prev_delivery_times[count_prev_pkts] = curr_delivery_time;
      count_prev_pkts++;
      return;
    }
    else { 
      in_initialization = false;
    }
  }*/
  
  if (in_initialization) {
    curr_window_size += 1/(curr_window_size*curr_window_size);
    return;
  }

  if (in_getting_first_pkts_phase) {
    if (count_prev_pkts < 10) {
      prev_delivery_times[count_prev_pkts] = curr_delivery_time;
      count_prev_pkts++;
    }
    else if (count_prev_pkts == 10) {
      in_getting_first_pkts_phase = false;
      }
    return;
  }
  
  if (count_curr_pkts < 10) {
  // accumulate current pkts times
  curr_delivery_times[count_curr_pkts] = curr_delivery_time;
  count_curr_pkts++;
  return;
  }
  
  // if seen 10 new pkts, make a decision
  if (count_curr_pkts == 10) {
    // reset counters
    count_curr_pkts = 0;
    count_prev_pkts = 0;
    // calculate average delivery times
    uint64_t curr_avg = 0;
    uint64_t prev_avg = 0;
    for (int i=0; i<10; ++i) {
      curr_avg += curr_delivery_times[i];
      prev_avg += prev_delivery_times[i];
      // curr -> prev
      prev_delivery_times[i] = curr_delivery_times[i];
    }
    curr_avg /= 10;
    prev_avg /= 10;
    std::cout << "at time " << timestamp() << ": current average " << curr_avg
             << " previous average " << prev_avg << "\n";

    // now make a decision
    if (curr_avg > prev_avg && curr_window_size > 1) {
      curr_window_size--;
    }
    else if (curr_avg < prev_avg) {
      curr_window_size++;
    }
  }







  

  /* 
  // if last change was too recent, do nothing
  if (timestamp() - last_window_size_change < 200) {
    return;
  }

  if (curr_delivery_time > 2*prev_delivery_time && curr_window_size >= 1) {
    curr_window_size -= 0.5;
  }
  else if (curr_delivery_time < prev_delivery_time/2) {
    curr_window_size += 0.5;
  }
  prev_delivery_time = curr_delivery_time;
  last_window_size_change = timestamp();
  */
 

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
  curr_window_size += 1/curr_window_size;
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
  return 100; /* timeout of one second */
}
