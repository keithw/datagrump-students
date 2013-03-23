#include <stdio.h>
#include <math.h>
#include <queue>
#include <cassert>
#include <utility>
#include <unistd.h>
#include <cstdlib>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
using namespace std;

double cwind;
queue<int>  runmean;
list<int>  runmeanLR;//long-range queue
list<int>  stimes;
list<int>  rtimes;

double uploaddelay=0;
double eps=0.01;

#define RTT 40.0
FILE *fsend = stderr;
FILE *fget = stderr;
int lastspike = 0;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwind(20),
    runmean(queue<int>()),
    packetBalance(list<uint64_t>()),
    resolution(100),
    resolutionLR(200),
    rtt(40),
    rttsum(400),
    rttn(10),
    ackTracker(0.0),
    delayTracker(RTT),
    ackLastDelta(0.0),
    lastAck(0),
    networkDown(false),
    recovery(0),
    lastPB(0),
    lastcint(1),
    lastcwind(1),
    start_time(timestamp()),
    rho(0.25),
                  burstPackets(list< pair<uint64_t, uint64_t> >()),
    sendTimestamp(list< pair<uint64_t, uint64_t> >())
{
  start_time = timestamp();
  fprintf( stderr, "startTime %lu\n", start_time);
}





/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  //double cwindDL = estimateParameters();
//assert(cwindDL >= 0) ;
  int cint = (int) cwind;
  //cint = chompWindow(cint, cwindDL);
  if(cint<1){cint=1;}
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
  sendTimestamp.push_back(pair<uint64_t, uint64_t>(sequence_number, send_timestamp));
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
  refineModulation(sequence_number_acked,send_timestamp_acked,recv_timestamp_acked,timestamp_ack_received);
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
  if(sequence_number_acked < 10){
    uploaddelay=uploaddelay*0.5+(timestamp_ack_received-recv_timestamp_acked-RTT/2)*0.5;
    return;
  }
  double rtttarget=20;
  //push new packet info onto queue
  stimes.push_front(send_timestamp_acked);
  rtimes.push_front(recv_timestamp_acked);
  runmean.push(timestamp_ack_received);
  runmeanLR.push_front(timestamp_ack_received);
  //trim queue to only include last (resolution+rtt/2) of packets.
  while(runmean.size()>0 && (timestamp_ack_received-runmean.front())>(resolution)){
    /*fprintf( stderr, "pop %i, timediff %lu \n",
      runmean.front(),timestamp_ack_received-runmean.front());*/
    runmean.pop();
  }
  // Long range queue trim
  while(runmeanLR.size()>0 && (timestamp_ack_received-runmeanLR.back())>(resolutionLR)){
    runmeanLR.pop_back();
    stimes.pop_back();
    rtimes.pop_back();
  }
  //account for truncation at start
  int truncres = resolution;
  int truncresLR = resolutionLR;
  double bwestSR=((double)runmean.size())/truncres;
  double bwestLR=((double)runmeanLR.size())/truncresLR;
  double bwest=(bwestSR+bwestLR)/(2);
  if(rtimes.size()>0){
    list<int>::const_iterator rIt=rtimes.begin();
    list<int>::const_iterator sIt=stimes.begin();
    list<int>::const_iterator rmIt=runmeanLR.begin();
    int diffsum=0;
    int dldelay=0;
    for(; (rIt!=rtimes.end() && sIt != stimes.end() && rmIt != runmeanLR.end()); ++rIt, ++sIt, ++rmIt){
    int rtime= *rIt;
    int stime= *sIt;
    int atime= *rmIt;
    diffsum+=rtime-stime;
    dldelay+=atime-rtime;
    }
    double mrtt=diffsum/((int)rtimes.size());
    double mrttD=dldelay/((int)runmeanLR.size());
    // over 100 ms RTT? ridiculous
    double delta = (mrttD-RTT/2)-uploaddelay;
    if(change > 0){
      uploaddelay+=eps;
    }else{
      uploaddelay-=eps;
    }
    //if(change < -10){change=-10;}
    double rtteps=rtttarget+uploaddelay;
    //fprintf(stderr,"rttmean: %i\n",(int)mrtt);
    // if our RTT is low and stable with at least 2xRTT our last time
    if(mrtt< (rtt/2+rtteps/4+uploaddelay) && ((timestamp_ack_received-lastspike)>(rtt))){
      cwind=bwest*(rtt+2*rtteps);
      lastspike=timestamp_ack_received;
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,TRUE,%.4f,%.4f\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind,
	      rtteps,
	      mrtt);
    }else{
      cwind= bwest*(rtt+rtteps);
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,FALSE,%.4f,%.4f\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind,
	      rtteps,
	      mrtt);
    }
  }else{
    double rtteps=rtttarget;
    cwind= bwest*(rtt+rtteps);
      fprintf(stdout,"%i,%i,%i,%.4f,%.4f,FALSE,-1,%.4f\n",
	      (int)(timestamp_ack_received-start_time),
	      (int)(recv_timestamp_acked-send_timestamp_acked),
	      (int)(timestamp_ack_received-recv_timestamp_acked),
	      bwest,
	      cwind,
	      rtteps);
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
  double ackRateEst = cwind/RTT;
  double ackRateObs = (ackTracker > 0.0) ? (1 / ackTracker) : ackRateEst;
  double cwindDL = ackRateObs * RTT;
  double wt = 0.5;
  if (rho > 0.5) { // we have confident, recent estimates
    if ((delayTracker <= (1.1*RTT)) && (ackRateObs > ackRateEst)) {
      fprintf(fsend, "%lu: cwinds: %.4f, %.4f : %.4f\n", tStamp, cwindDL, cwind, ackTracker);
      wt = 0.0;//9;
      cwind =  (wt*cwindDL + (1-wt)*cwind);
      if (cwind > lastcwind)
        cwind += 1;
      //else
      //cwind += 1;
    }
    else if ((delayTracker > (1.5*RTT)) && (delayTracker < (2*RTT)) && (cwind > 1) && (ackRateObs < ackRateEst))
      cwind -= 1;
    // // else if ((delayTracker > (2.0*RTT)) && (cwind > 1)  && (ackRateObs < ackRateEst))
    // //   cwind -= 2;
  } else { // not so confident
    // if ((delayTracker <= (1.1*RTT)) && (ackRateObs > ackRateEst)) {
    //   fprintf(fsend, "%lu: cwinds: %.4f, %.4f : %.4f\n", tStamp, cwindDL, cwind, ackTracker);
    //   wt = 0.0;//9;
    //   cwind =  (wt*cwindDL + (1-wt)*cwind);
    //   if (cwind > lastcwind)
    //     cwind += 2;
    // }
    // else if ((delayTracker > (1.5*RTT)) && (delayTracker < (2*RTT)) && (cwind > 1) && (ackRateObs < ackRateEst))
    //   cwind -= 1;
    // // else if ((delayTracker > (2.0*RTT)) && (cwind > 1)  && (ackRateObs < ackRateEst))
    // //   cwind -= 2;

  }
  return cwindDL;
}



int Controller::chompWindow(unsigned int cint, double cwindDL) {
  if (lastAck == 0) return 5;
  assert(cwindDL >= 0);
  //if (networkDown) return 0;
  uint64_t tStamp = timestamp();
  // if we have a zero congestion window, push it out of this regime
  // if we are just starting up
  // if ((lastcint >= cint) && (lastcint <= lastPB) && (lastcint <= 1.5*(cint)))
  //   cint = lastcint+1;

  // if we haven't seen the last ack in a while, stop sending cause
  // things are queued up!!
  // TODO: change 75 to something related to ~ 2*rtt!!. Try 1.5 or something
  if ((lastAck > 0)) {
    if ((lastAck > 0) && ((tStamp - lastAck) > (1.5*RTT))) {
      //fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
      // desired queue buildup should be bw * rtt? Space this out
      // make sure we sent two back to back packets for good estimates
      // int u = (1*(int) (1*(tStamp - lastAck)/((double)RTT) - sendTimestamp.size()))/1;
      // if (u >= 0) cint = u;
      // else cint = 0;
      //cint = 0;
      networkDown = true;
    }
    if ((lastAck > 0) && ((tStamp - lastAck) > (0.5*RTT))) {
      //fprintf(fsend, "%lu: unseen last timestamp %lu = %lu\n", tStamp, lastAck, tStamp - lastAck );
      //cint = 1;//cint/2;
    }
    // else if ((lastAck > 0)  && (cint == lastcint)) {
    //   cint += 1; // don't ever stay in a state without exploring up
    // }
  }
  // // make sure %change in cint isn't too spiky : causes delays
  // if ((lastcint > 0) && (cint > (int)lastcint))
  //   if ((cint - lastcint)/float(lastcint) > 2) // change by more than 200%
  //     cint = 2*lastcint; // can't be anything less than 2 since we are dealing with
  //                      // integers. May give issues with small numbers

  // if ( debug_ ) {
  //   fprintf( fsend, "@%lu, %d, %.4f, %.4f, %.4f, %u, %.2f, %.1f, %lu\n",
  //            (tStamp - start_time), cint, cwindDL, cwind, ackTracker, lastcint, ackLastDelta, RTT, (lastAck > 0) ? (tStamp - lastAck) : 0);
  // }

  lastcwind = cwind;
  lastcint = cint;
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
  networkDown = false;
  double delay = RTT;
  for (list< pair<uint64_t, uint64_t> >::iterator it = sendTimestamp.begin(); it != sendTimestamp.end(); ++it) {
    if (it->first == sequence_number_acked) {
      delay = timestamp_ack_received - it->second;
      sendTimestamp.erase(it);
      break;
    }
  }
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

  delayTracker = (1-rho)*delayTracker + rho * delay;

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
}
void Controller::markBeginning(const uint64_t start_sequence_number, const uint64_t end_sequence_number) {
  burstPackets.push_back(pair<uint64_t, uint64_t>(start_sequence_number, end_sequence_number));
}
