#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"
#include <unordered_map>

#define TIMEOUT 117
#define MINRTT 1500
#define MINCWND 3.0
using namespace Network;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd_frac(MINCWND), md_factor(1.6), ai_factor(0.83),
    min_rtt(MINRTT), prev_timeval(timestamp()), slow_start(1), sent_times()
{
    
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */
  int the_window_size = (int)cwnd_frac;

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
  sent_times[sequence_number] = send_timestamp;
  
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

  // remove this packet from table
  //if(sent_times.count(sequence_number_acked))
  //  sent_times.erase(sequence_number_acked);
  
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  
  if (rtt < min_rtt)   
    min_rtt = rtt;
  fprintf( stdout, "Received packet %lu : rtt is %lu\n",
           sequence_number_acked, rtt );  

  if (rtt  < timeout_ms()) {
    ai();
    if(sent_times.count(sequence_number_acked))
      sent_times.erase(sequence_number_acked);
    check_timeout();
  } else { // detect network congestion, reduce window size
    if(sequence_number_acked != 0) { 
      check_timeout();
    }
    
    if(sent_times.count(sequence_number_acked))
      sent_times.erase(sequence_number_acked);
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
  return TIMEOUT; 
}

void Controller::check_timeout( void )
{ 
  uint64_t current_time = timestamp();
  fprintf( stdout, "time diff : %lu\n", current_time - prev_timeval);
/*
  bool do_md = 0;
  double num_packets = 0.0;
  double num_packets_delayed = 0.0;
*/
  if (current_time - prev_timeval > min_rtt){
//  if (current_time - prev_timeval > timeout_ms()) { 
    fprintf(stdout, "Checking timeouts\n");
    for ( auto it = sent_times.begin(); it != sent_times.end(); ++it ) {
//      num_packets++;
      if (current_time - it->second > timeout_ms()) {
//        do_md = 1;
//       fprintf(stdout, "Will do MD\n");
//        num_packets_delayed++;

        md();
        break;

      }
    }
/*
    if (do_md) {
      fprintf(stdout, "num_packets_delayed: %f\n", num_packets_delayed);
      fprintf(stdout, "num_packets: %f\n", num_packets);
      md_factor = 2.4*num_packets_delayed/num_packets;
      md();
    }
*/
    prev_timeval = current_time;
  }  
}

/* Multiplicative Decrease: divide cwnd by md_factor */
void Controller::md( void )
{
  fprintf(stdout, "Did MD, factor = %f\n", md_factor);
  slow_start = 0;
  cwnd_frac /= md_factor;
  if (cwnd_frac < MINCWND) cwnd_frac = MINCWND;
}


/* Additive Increase: add 1 per time interval to cwnd */
void Controller::ai( void ) 
{
  if (slow_start)
    cwnd_frac += ai_factor;
  else
    cwnd_frac += ai_factor/(cwnd_frac);
}
