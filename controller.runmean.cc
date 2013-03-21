#include <stdio.h>
#include <math.h>
#include <queue>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
double cwind;
std::queue<int>  runmean;
std::list<int>  stimes;
std::list<int>  rtimes;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwind(10),
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


/*
int Controller::chompWindow(int cint) {

  // if we have a zero congestion window, push it out of this regime
  // if we are just starting up
  if (cint < 1)
    if ((lastCW == 0) || (ackTracker == 0.0))
      cint = 1;

  if ((lastCW >= cint) && (lastCW <= lastPB) && (lastCW <= 1.5*(cint)))
    cint = lastCW+1;

  // if we haven't seen the last ack in a while, stop sending cause
  // things are queued up!!
  // TODO: change 75 to something related to ~ 2*rtt!!. Try 1.5 or something
  if ((lastAck > 0) && ((tStamp - lastAck) > (1.5*rttest))) {
    fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
    cint = 0;
  }
  if ((lastAck > 0) && ((tStamp - lastAck) > rttest)) {
    //fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
    cint = cint/2;
  }

  if ( debug_ ) {
    fprintf( fsend, "@%lu, %d, %.4f, %.4f, %.4f, %u, %.2f, %.1f, %lu\n",
       (tStamp - start_time), cint, cwindDL, cwind, ackTracker, lastCW, ackLastDelta, rttest, (lastAck > 0) ? (tStamp - lastAck) : 0);
  }
  // make sure %change in cint isn't too spiky : causes delays
  if ((lastCW > 0) && (cint > lastCW))
    if ((cint - lastCW)/float(lastCW) > 2)
      cint = 1.25*lastCW;

  lastCW = cint;
  return cint;
}
*/
/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  //estimateParameters();
  /* Default: fixed window size of one outstanding packet */
  int cint = (int) cwind;
  if(cint==0){cint=1;}
  //cint = chompWindow(cint);

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
  //refineModulation(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
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
  //push new packet info onto queue
  stimes.push_front(send_timestamp_acked);
  rtimes.push_front(recv_timestamp_acked);
  runmean.push(timestamp_ack_received);
  //trim queue to only include last (resolution+rtt/2) of packets.
  while(runmean.size()>0 && (timestamp_ack_received-runmean.front())>(resolution)){
    fprintf( stderr, "pop %i, timediff %lu \n",
             runmean.front(),timestamp_ack_received-runmean.front());
    runmean.pop();
    stimes.pop_back();
    rtimes.pop_back();
  }
  fprintf(stderr, "size: %i\n",(int)runmean.size());
  std::list<int>::const_iterator rIt=rtimes.begin();
  std::list<int>::const_iterator sIt=stimes.begin();
  int diffsum=0;
  for(; rIt!=rtimes.end() && sIt != stimes.end(); ++rIt, ++sIt){
    int rtime=*rIt;
    int stime=*sIt;
    diffsum+=rtime-stime;
  }
  double mrtt=diffsum/((int)rtimes.size());
  fprintf(stderr,"rttmean: %i\n",(int)mrtt);
  //double bwest=runmean.size()/resolution;
  double slope = 0.5414;
  double icept = -1.0402;
  double tfbest = 2*sqrt(runmean.size()+3/8)*slope+icept;
  double bwest=(tfbest*tfbest/4-1/8)/20;
  if(mrtt > (rtt/2+5)){
  //if(cwind > runmean.size()/resolution*rtt){
    cwind= bwest*(rtt+20);
  }else{
    cwind= (bwest+sqrt(bwest)*0.598+1.11023)*(rtt+20);
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
  return 10000; /* timeout of one second */
}

/*
void Controller::refineModulation(const uint64_t sequence_number_acked,
                                  const uint64_t send_timestamp_acked,
                                  const uint64_t recv_timestamp_acked,
                                  const uint64_t timestamp_ack_received ){
  if (lastAck == 0) {
    lastAck = recv_timestamp_acked;
  }
  else if(lastAck < recv_timestamp_acked){ //if this is not true, something funny is going on
    if (recovery > 0)  {
      // need to adjust for overlapping acks
      double d = ackLastDelta / (double)recovery;
      ackTracker -= rho * (ackLastDelta - d);
      for (unsigned int i=1; i< recovery; i++)
        ackTracker = (1-rho)*ackTracker + rho*d;
      recovery = 0;
    }
    ackLastDelta = (recv_timestamp_acked-lastAck);
    ackTracker = (1-rho)*ackTracker + rho*ackLastDelta;
    lastAck = recv_timestamp_acked;
  }
  else if(lastAck == recv_timestamp_acked) {//multiple acks at once
    ++recovery;
  }
}
*/
