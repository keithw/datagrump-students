//
#include <stdio.h>
#include <math.h>
#include "controller.hh"
#include "timestamp.hh"
#include "packet.hh"

using namespace Network;
//FILE *fsend = fopen("tmp/send.txt", "w");
//FILE *fget = fopen("tmp/get.txt", "w");
FILE *fsend = stderr;
FILE *fget = stderr;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    cwind(0.001),
    runmean(std::queue<int>()),
    packetBalance(std::list<uint64_t>()),
    resolution(200),
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



int Controller::trimRunmean(const uint64_t tstamp) {
  while(runmean.size()>0 && (tstamp-runmean.front())>(resolution)){
    fprintf( stderr, "pop %i, timediff %lu \n",
       runmean.front(),tstamp-runmean.front());
    runmean.pop();
  }
  int rmsize = (int)runmean.size();
  return rmsize;
}

int Controller::getRmsize(const uint64_t tstamp, double res) {
  int sz = runmean.size();
  while(runmean.size()>0 && (tstamp-runmean.front())>(res))
    --sz;

  int rmsize = sz;
  return rmsize;
}


/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  uint64_t tStamp = timestamp();
  int rmsize = trimRunmean(tStamp);
  double rttest=rttsum/rttn;
  if(rmsize/resolution*rttest<cwind){
    cwind=rmsize/resolution*rttest;
  }

  //downlink response rate:
  unsigned int pbs = packetBalance.size();
  double ackRateEst = cwind/rttest;
  double ackRateObs = (ackTracker > 0.0) ? (1 / ackTracker) : ackRateEst;
  double cwindDL = ackRateObs * rttest;
  if (ackRateObs > ackRateEst) {
    // if we are getting acks faster => network has recoved and queue is
    // being flushed and we are getting fast responses
    double wt = 0.5;
    fprintf(fsend, "%lu: cwinds: %.4f, %.4f : %.4f\n", tStamp, cwindDL, cwind, ackTracker);
    cwind = wt*cwindDL + (1-wt)*cwind;
  } else {
    // if we are getting acks slower => network is putting stuff in a queue somewhere
    // This means we need to slow down
    if ((pbs > 1) && (pbs >= (1.7*cwind))) {
      // we owe a debt we may not be able to pay
      double delta = pbs - cwind;
      fprintf(fsend, "%lu: overflow by %.2f : %.2f -> ", tStamp,  delta, cwind);
      if (delta < 2*cwind) cwind -= delta/2;
      else cwind /= 2;
      fprintf(stderr, "%.2f \n", cwind);
    } else if(lastPB <= pbs) {
      lastPB = pbs;
      //cwind += 1;
    }
  }

  unsigned int cint = (int) floor(cwind + 0.5); // round instead of floor

  // if we have a zero congestion window, push it out of this regime
  // if we are just starting up
  if (cint < 1)
    if ((pbs == 0) || (ackTracker == 0.0))
      cint = 1;

  if ((lastCW >= cint) && (pbs <= lastPB) && (pbs <= 1.5*(cint)))
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
       (tStamp - start_time), cint, cwindDL, cwind, ackTracker, pbs, ackLastDelta, rttest, (lastAck > 0) ? (tStamp - lastAck) : 0);
  }
  // make sure %change in cint isn't too spiky : causes delays
  if ((lastCW > 0) && (cint > lastCW))
    if ((cint - lastCW)/float(lastCW) > 2)
      cint = 1.25*lastCW;

  lastCW = cint;
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
  if ((packetBalance.size() == 0) || (packetBalance.back() < sequence_number))
    packetBalance.push_back(sequence_number);
  else {
    for (std::list<uint64_t>::iterator it = packetBalance.begin(); it != packetBalance.end(); ++it) {
      if ((*it) > sequence_number) {
  packetBalance.insert(it, sequence_number);
  break;
      }
      if ((*it) == sequence_number) break;
    }
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
  // remove all packets with smaller seq number
  while ((packetBalance.size() > 0) && (packetBalance.front() <= sequence_number_acked))
    packetBalance.pop_front();

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

  runmean.push(timestamp_ack_received);

  int rmsize = trimRunmean(timestamp_ack_received);
  double rttest=rttsum/rttn;
  if(rmsize/resolution*rttest<cwind){
    cwind=rmsize/resolution*rttest;
  }else{
    // NB - this is a (bad) approximation of the incomplete gamma function.
    //cwind+=0.65;
    cwind=rmsize/resolution*rttest+0.67*sqrt(rmsize/resolution*rttest)+0.94;
    //cwind+=0.05*cwind
    int tdiff= 2*(timestamp_ack_received-recv_timestamp_acked);
    fprintf(stderr, "rtt: %i\n",tdiff);
    rttsum+=tdiff;
    rttn++;
  }
  fprintf(stderr, "runmean size: %d\t packetBalance:%lu\n",(int)runmean.size(), packetBalance.size());
  //cwind=((double)runmean.size())/resolution*rtt+1;
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
       timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
       send_timestamp_acked, recv_timestamp_acked );
    fprintf( fget, "At time %lu, received ACK for packet %lu",
       timestamp_ack_received, sequence_number_acked );

    fprintf( fget, " (sent %lu, received %lu by receiver's clock).\n",
       send_timestamp_acked, recv_timestamp_acked );
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{
  if (packetBalance.size() > 10) return (200 * (packetBalance.size() / 5));
  return 50; /* timeout of one second */
}
