#include <stdio.h>
#include <math.h>
#include <queue>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
double cwind;
std::queue<int>  runmean;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwind(0.001),
    runmean(std::queue<int>()),
    packetBalance(std::list<uint64_t>()),
    resolution(100),
    rtt(40),
    rttsum(400),
    rttn(10),
    ackTracker(0.0),
    ackLastDelta(0.0),
    lastAck(0),
    rho(2.5),
    recovery(0),
    lastPB(0),
    lastCW(0),
    start_time(timestamp())
{
  start_time = timestamp();
  fprintf( stderr, "startTime %lu\n", start_time);
}



void Controller::estimateParameters() {
}



int Controller::chompWindow(int cint) {
  return cint;
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  estimateParameters();
  /* Default: fixed window size of one outstanding packet */
  int cint = (int)cwind;

  cint = chompWindow(cint);

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
             timestamp(), cint );
  }

  return cint;
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
  refineParameters(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
             timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
             send_timestamp_acked, recv_timestamp_acked );
  }
}

void Controller::refineParameters(const uint64_t sequence_number_acked,
                               /* what sequence number was acknowledged */
                               const uint64_t send_timestamp_acked,
                               /* when the acknowledged packet was sent */
                               const uint64_t recv_timestamp_acked,
                               /* when the acknowledged packet was received */
                               const uint64_t timestamp_ack_received )
{
  runmean.push(timestamp_ack_received);
  while(runmean.size()>0 && (timestamp_ack_received-runmean.front())>(resolution)){
    fprintf( stderr, "pop %i, timediff %lu \n",
             runmean.front(),timestamp_ack_received-runmean.front());
    runmean.pop();
  }
  fprintf(stderr, "size: %i\n",(int)runmean.size());
  cwind=((double)runmean.size())/resolution*rtt*0.914+0.05811*rtt;

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
