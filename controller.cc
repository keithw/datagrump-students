#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"
#include <algorithm>

using namespace Network;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd(100), count(0), sa(0), sv(0), rto(1200), sasa(0),
    previousSA(0), minRTT(5000), sendTimestamps(), slowStart(true)
{
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  int the_window_size = cwnd;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size );
  }

  return the_window_size;
}

void Controller::set_window_size(uint64_t new_size) {
  cwnd = new_size;
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp )
                                  /* in milliseconds */
{
  std::pair<uint64_t, uint64_t> sentPair(sequence_number, send_timestamp);
  sendTimestamps.insert(sentPair);

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
  uint64_t sendTimestamp = sendTimestamps[sequence_number_acked];
  uint64_t rtt = timestamp_ack_received - sendTimestamp;
  if (rtt < minRTT) {
    minRTT = rtt;
  }

  // Update estimators
  int64_t m = rtt;
  m -= (sa >> 3);
  sa += m;
  if (m < 0) {
    m = -m;
  }
  m -= (sv >> 2);
  sv += m;
  rto = (sa >> 3) + sv;

  int64_t deltaSA = 0;
  if (previousSA != 0) {
    deltaSA = sa - previousSA;
  }
  previousSA = sa;

  int64_t md = deltaSA;
  md -= (sasa >> 2);
  sasa += md; 

  //if (rtt > RTT_THRESHOLD_MS || (rtt > (RTT_THRESHOLD_MS - 30) && (sasa >> 3) > 15)) {
  //  slowStart = false;
  //  if (deltaSA >= 80 && (sasa >> 3) >= 0) {
  //    cwnd = std::max(cwnd / 8, CWND_MIN);
  //  } else if (deltaSA >= 40 && (sasa >> 3) < 0) {
  //    cwnd = std::max(cwnd / 4, CWND_MIN);
  //  } else {
  //    cwnd = std::max(cwnd / 2, CWND_MIN);
  //  }
  //}  else {
  //  if (slowStart || (deltaSA <= -50 && (sasa >> 3) <= 0)) {
  //    ++cwnd;
  //    count = 0;
  //  } else if (deltaSA <= -25 && (sasa >> 3) > 0) {
  //    count += 4;
  //  } else {
  //    count++;
  //  }
  //  if (count >= cwnd) {
  //    ++cwnd;
  //    count = 0;
  //  }
  //}

  //uint64_t rttAvg = sa >> 3;
//  uint64_t avgChange = sasa >> 2;
  //if (rtt > RTT_THRESHOLD_MS) {
  if (rtt > RTT_THRESHOLD_MS && (sasa >> 2) > 10) {
    cwnd = std::max(cwnd - 1, CWND_MIN);
  } else {
    if (rtt < (minRTT + 15)) {
      count += 2;
    } else {
      count += 1;
    }
    //if ((sasa >> 2) < 0 && ((sa >> 3) < RTT_THRESHOLD_MS)) {
    //  count++;
    //}
    if (count >= cwnd) {
      ++cwnd;
      count = 0;
    }
  }

  //if ((abs(rtt - (sa >> 3)) < (sv >> 2)) && rtt < RTT_THRESHOLD_MS)  {
  //  ++count;
  //  if (count >= cwnd) {
  //    ++cwnd;
  //    count = 0;
  //  }
  //} else {
  //  cwnd = std::max(cwnd / 2, CWND_MIN);
  //}

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
    fprintf( stderr, "rtt: %lu and scaled rtt average: %lu sasa: %ld minRTT: %lu\n",
             rtt, (sa >> 3), (sasa >> 2), minRTT);
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return rto; /* timeout of one second */
}
