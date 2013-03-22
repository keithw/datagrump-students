#include <stdio.h>
#include <math.h>
#include <queue>
#include <cassert>
#include <utility>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
double cwind;
std::queue<int>  runmean;
std::queue<int>  runmeanLR;//long-range queue
std::list<int>  stimes;
std::list<int>  rtimes;
std::list< std::pair<uint64_t, uint64_t> > burstPackets;
#define rttest 40.0
FILE *fsend = stderr;
FILE *fget = stderr;
int lastspike = 0;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwind(10),
    runmean(std::queue<int>()),
    packetBalance(std::list<uint64_t>()),
    resolution(80),
    resolutionLR(200),
    rtt(40),
    rttsum(400),
    rttn(10),
    ackTracker(0.0),
    ackLastDelta(0.0),
    lastAck(0),
    networkDown(false),
    recovery(0),
    lastPB(0),
    lastCW(0),
    start_time(timestamp()),
    rho(0.25)
{
  start_time = timestamp();
  fprintf( stderr, "startTime %lu\n", start_time);
}





/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  //double cwindDL = estimateParameters();
  int cint = (int) cwind;
  if(cint<1){cint=1;}
  //cint = chompWindow(cint, cwindDL);
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



//////////////////////////////////////////////////
// adjustments and updates
//////////////////////////////////////////////////

void Controller::refineParameters(const uint64_t sequence_number_acked,
                               /* what sequence number was acknowledged */
                               const uint64_t send_timestamp_acked,
                               /* when the acknowledged packet was sent */
                               const uint64_t recv_timestamp_acked,
                               /* when the acknowledged packet was received */
                               const uint64_t timestamp_ack_received )
{
  double rtteps=20;
  //push new packet info onto queue
  stimes.push_front(send_timestamp_acked);
  rtimes.push_front(recv_timestamp_acked);
  runmean.push(timestamp_ack_received);
  runmeanLR.push(timestamp_ack_received);
  //trim queue to only include last (resolution+rtt/2) of packets.
  while(runmean.size()>0 && (timestamp_ack_received-runmean.front())>(resolution)){
    /*fprintf( stderr, "pop %i, timediff %lu \n",
      runmean.front(),timestamp_ack_received-runmean.front());*/
    runmean.pop();
    stimes.pop_back();
    rtimes.pop_back();
  }
  // Long range queue trim
  while(runmeanLR.size()>0 && (timestamp_ack_received-runmeanLR.front())>(resolutionLR)){
    runmeanLR.pop();
  }
  //fprintf(stderr, "size: %i\n",(int)runmean.size());
  double bwestSR=((double)runmean.size())/resolution;
  double bwestLR=((double)runmeanLR.size())/resolutionLR;
  double bwest=(bwestSR+bwestLR*4)/(1+4);
  if(rtimes.size()>0){
    std::list<int>::const_iterator rIt=rtimes.begin();
    std::list<int>::const_iterator sIt=stimes.begin();
    int diffsum=0;
    for(; (rIt!=rtimes.end() && sIt != stimes.end()); ++rIt, ++sIt){
    int rtime= *rIt;
    int stime= *sIt;
    diffsum+=rtime-stime;
    }
    double mrtt=diffsum/((int)rtimes.size());
    //fprintf(stderr,"rttmean: %i\n",(int)mrtt);
    // if our RTT is low and stable with at least 2xRTT our last time
    if(mrtt< (rtt/2+rtteps/4) && ((timestamp_ack_received-lastspike)>(rtt))){
      cwind=bwest*(rtt+3*rtteps);
      lastspike=timestamp_ack_received;
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,TRUE,%.4f\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind,
	      mrtt);
    }else{
      cwind= bwest*(rtt+rtteps);
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,FALSE,%.4f\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind,
	      mrtt);
    }
  }else{
    cwind= bwest*(rtt+rtteps);
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,FALSE,-1\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind);
  }
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
             timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
             send_timestamp_acked, recv_timestamp_acked );
  }
}




double Controller::estimateParameters() {

  uint64_t tStamp = timestamp();
  //downlink response rate:
  double ackRateEst = cwind/rttest;
  double ackRateObs = (ackTracker > 0.0) ? (1 / ackTracker) : ackRateEst;
  double cwindDL = ackRateObs * rttest;
  if (ackRateObs >= ackRateEst) {
    // if we are getting acks faster => network has recoved and queue is
    // being flushed and we are getting fast responses
    //double wt = 0.5;
    //if (rho > 0.25) wt = 0.9;
    fprintf(fsend, "%lu: cwinds: %.4f, %.4f : %.4f\n", tStamp, cwindDL, cwind, ackTracker);
    // cwind = (wt*cwindDL + (1-wt)*cwind);
  } else {
    // // if we are getting acks slower => network is putting stuff in a queue somewhere
    // // This means we need to slow down
    // if ((lastCW > 1) && (lastCW >= (1.7*cwind))) {
    //   // we owe a debt we may not be able to pay
    //   double delta = lastCW - cwind;
    //   fprintf(fsend, "%lu: overflow by %.2f : %.2f -> ", tStamp,  delta, cwind);
    //   if (delta < 2*cwind) cwind -= delta/2;
    //   else cwind /= 2;
    //   fprintf(stderr, "%.2f \n", cwind);
    // } else if(lastPB <= lastCW) {
    //   lastPB = lastCW;
    //   //cwind += 1;
    // }
  }

  return cwindDL;
}



int Controller::chompWindow(int cint, double cwindDL) {
  if (networkDown) return 0;
  uint64_t tStamp = timestamp();
  // if we have a zero congestion window, push it out of this regime
  // if we are just starting up
  if (cint < 1)
    if ((lastCW == 0) || (ackTracker == 0.0))
      cint = 1;
  //if (rho > 0.25) cint += 1;
  // if ((lastCW >= cint) && (lastCW <= lastPB) && (lastCW <= 1.5*(cint)))
  //   cint = lastCW+1;

  // if we haven't seen the last ack in a while, stop sending cause
  // things are queued up!!
  // TODO: change 75 to something related to ~ 2*rtt!!. Try 1.5 or something
  if ((lastAck > 0) && ((tStamp - lastAck) > (rttest))) {
    fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
    cint = 1;
    networkDown = true;
  }
  if ((lastAck > 0) && ((tStamp - lastAck) > (0.5*rttest))) {
    //fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
    cint = 1;//cint/2;
  }
  else if ((lastAck > 0)  && (cint == (int)lastCW)) {
    //cint += 1; // don't ever stay in a state without exploring up
  }
  // // make sure %change in cint isn't too spiky : causes delays
  // if ((lastCW > 0) && (cint > (int)lastCW))
  //   if ((cint - lastCW)/float(lastCW) > 2) // change by more than 200%
  //     cint = 2*lastCW; // can't be anything less than 2 since we are dealing with
  //                      // integers. May give issues with small numbers

  if ( debug_ ) {
    fprintf( fsend, "@%lu, %d, %.4f, %.4f, %.4f, %u, %.2f, %.1f, %lu\n",
             (tStamp - start_time), cint, cwindDL, cwind, ackTracker, lastCW, ackLastDelta, rttest, (lastAck > 0) ? (tStamp - lastAck) : 0);
  }
  if (lastAck == 0) cint = 5;

  lastCW = cint;
  return cint;
}


/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  return 50; /* timeout of one second */
}


void Controller::refineModulation(const uint64_t sequence_number_acked,
                                  const uint64_t send_timestamp_acked,
                                  const uint64_t recv_timestamp_acked,
                                  const uint64_t timestamp_ack_received ){
  rho = 0.25;
  while (burstPackets.size() > 0) {
    if (sequence_number_acked <= burstPackets.front().second) break;
    burstPackets.pop_front();
  }
  if (burstPackets.size() > 0) {
    //if (sequence_number_acked == burstPackets.front().first) rho = 0.0;
    if ((sequence_number_acked >  burstPackets.front().first) &&
        (sequence_number_acked <= burstPackets.front().second))
      rho = 0.9;
  }
  assert(send_timestamp_acked <= timestamp_ack_received);
  assert(recv_timestamp_acked <= timestamp_ack_received);

  if (lastAck == 0) {
    //lastAck = recv_timestamp_acked;
    lastAck = timestamp_ack_received;
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
    ackLastDelta = (timestamp_ack_received-lastAck);
    ackTracker = (1-rho)*ackTracker + rho*ackLastDelta;
    lastAck = timestamp_ack_received;
  }
  else if(lastAck == timestamp_ack_received) {//multiple acks at once
    ++recovery;
  }
  networkDown = false;
}
void Controller::markBeginning(const uint64_t start_sequence_number, const uint64_t end_sequence_number) {
  burstPackets.push_back(std::pair<uint64_t, uint64_t>(start_sequence_number, end_sequence_number));
}
