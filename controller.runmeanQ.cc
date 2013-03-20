#include <stdio.h>
#include <math.h>
#include <queue>
#include <list>
#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
double cwind;
std::queue<int>  runmean;
std::list<uint64_t> packetBalance;
double resolution = 200;
double rtt=40;
double rttsum=400;
double rttn=10;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  cwind = 5;
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  while(runmean.size()>0 && (timestamp()-runmean.front())>(resolution)){
    fprintf( stderr, "pop %i, timediff %lu \n",
	     runmean.front(),timestamp()-runmean.front());
    runmean.pop();
  }
  double rttest=rttsum/rttn;
  int rmsize = (int)runmean.size();
  if(rmsize/resolution*rttest<cwind){
    cwind=rmsize/resolution*rttest;
  }
  /* Default: fixed window size of one outstanding packet */
  unsigned int cint = (int)cwind;
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), cint );
  }

  //factor in the number of packets in the queue. If our congestion
  //window is cint, we should have at most 2*cint unacked packets (due
  //to rtt, cint will be in the pipe forward and cint acks will be on
  //the way back). Anything else is in the queue.
  if (packetBalance.size() >= (2*cint)) {
    unsigned int tmp = (2*cint) - packetBalance.size();
    // don't fully compensate so that we can test the way this changes things
    if (tmp < cint) cint -= tmp/2;
    else cint /= 2;
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

  runmean.push(timestamp_ack_received);

  while(runmean.size()>0 && (timestamp_ack_received-runmean.front())>(resolution)){
    fprintf( stderr, "pop %i, timediff %lu \n",
	     runmean.front(),timestamp_ack_received-runmean.front());
    runmean.pop();
  }
  double rttest=rttsum/rttn;
  int rmsize = (int)runmean.size();
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
  fprintf(stderr, "size: %i\n",(int)runmean.size());
  //cwind=((double)runmean.size())/resolution*rtt+1;
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
  return 50; /* timeout of one second */
}
