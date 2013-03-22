#include <stdio.h>
#include <list>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

double real_window = 1;
double MAX_WINDOW_SIZE = 100;
double MIN_WINDOW_SIZE = 1;
int HALF_LEVEL = 3;
int num_since_half = HALF_LEVEL;
std::list<uint64_t> delays;
uint64_t last_ack_received = 0;
uint64_t MAX_DELAY_LIST_SIZE = 9;

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
   double rate = Controller::get_rate_estimate();
  double cap = 110 * rate;

  fprintf( stderr, "Cap computed as %f", cap);
  real_window = (real_window > MAX_WINDOW_SIZE) ? MAX_WINDOW_SIZE : real_window;
  if (real_window > 10) {
	  real_window = (real_window > cap) ? cap : real_window;
  }
  real_window = (real_window < MIN_WINDOW_SIZE) ? MIN_WINDOW_SIZE : real_window;

 /* Default: fixed window size of one outstanding packet */
  int the_window_size = (int)real_window;

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
  /* Default: take no action */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }
}

// Delay in packets / ms
double Controller::get_rate_estimate() {
	uint64_t sum = 0;
	std::list<uint64_t>::const_iterator iterator;
	for (iterator = delays.begin(); iterator != delays.end(); ++iterator) {
		sum += *iterator;
	}
	uint64_t time_since_last = Network::timestamp() - last_ack_received;
	  fprintf( stderr, "sum was %lu", sum);
	  fprintf( stderr, "time since last %lu", time_since_last);
	if (last_ack_received != 0) { sum += time_since_last; }
	return ((double)delays.size()+1) / sum;
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
/*TCP AIMD*/
  if (last_ack_received == 0) {
	last_ack_received = timestamp_ack_received;
  } else {
	uint64_t diff;
	diff = timestamp_ack_received - last_ack_received;
	  fprintf( stderr, "ack diff was %lu", diff);
	delays.push_back(diff);
	if (delays.size() > (uint64_t)Controller::window_size()) {
		delays.pop_front();
	}
	last_ack_received = timestamp_ack_received;
  }

  uint64_t rtt = recv_timestamp_acked - send_timestamp_acked;
  if (rtt > (timeout_ms()/8)) {
	if (num_since_half == HALF_LEVEL) {
		real_window /= 2;
		num_since_half = 0;
	} else {
		num_since_half += 1;
	}
  } else {
//	double inc_amount = 2.5-(rtt/50.0);
	double inc_amount = 1;
	real_window += (inc_amount/real_window);
	num_since_half = HALF_LEVEL;
  }
/*
  double rate = Controller::get_rate_estimate();
  double cap = 150 * rate;

  fprintf( stderr, "Cap computed as %f", cap);
  real_window = (real_window > MAX_WINDOW_SIZE) ? MAX_WINDOW_SIZE : real_window;
  real_window = (real_window > cap) ? cap : real_window;
  real_window = (real_window < MIN_WINDOW_SIZE) ? MIN_WINDOW_SIZE : real_window;
*/
/* Delay triggers
  uint64_t low_watermark = 150;
  uint64_t high_watermark = 200;

  if (recv_timestamp_acked - send_timestamp_acked > high_watermark) {
	//time to shrink
	real_window -= (1/real_window);
  } else if (recv_timestamp_acked - send_timestamp_acked < low_watermark) {
	//time to grow
	real_window += (1/real_window);
  } else {
	//stay the same
  }
  real_window = (real_window < MIN_WINDOW_SIZE) ? MIN_WINDOW_SIZE : real_window;
  real_window = (real_window > MAX_WINDOW_SIZE) ? MAX_WINDOW_SIZE : real_window;
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
  return 1000; /* timeout of one second */
}
