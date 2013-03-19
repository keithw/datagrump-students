#include <stdio.h>
#include <fstream>

#include "math.h"

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), 
    w_size_(1), 
    rtt_last_(0),
    rtt_min_(100000.0),
    rtt_max_(0),
    rtt_avg_(0),
    rtt_ratio_(1.0),
    params_()
{
  /* Default parameters */
  params_.AI = 1.0;
  params_.MD = 0.5;
  params_.AVG = 0.5;

  LoadParams("controller_config.txt");
}

/* Loads all params from file */
void Controller::LoadParams(const char* filename) {
  try {
    ifstream param_file(filename);
    param_file >> params_.AI;
    param_file >> params_.MD;
  } catch (...) {
    fprintf(stderr, "There's something wrong with the file. Some params will have default values.\n");
  }
  fprintf(stderr, "Read params AI:%.1f, MD:%.1f\n", params_.AI, params_.MD);
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of one outstanding packet */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), (unsigned int) w_size_ );
  }

  return (unsigned int) w_size_;
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
  

  double rtt = (double)(timestamp_ack_received - send_timestamp_acked);
  /*fprintf(stdout, "%lu", timestamp_ack_received);*/
  update_rtt_stats(rtt);

  /* Default: take no action */
  w_size_ = w_size_ + (pow((1/rtt_ratio_),2)-0.2)*(params_.AI / max(w_size_, 1.0));

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
  return 1000; /* timeout in ms */
}

void Controller::packet_timed_out(void)
{
  w_size_ = w_size_ - params_.MD * w_size_;
}

void Controller::update_rtt_stats(const double rtt)
{
  if (rtt_min_ > rtt){
    rtt_min_ = rtt;
  }
  if (rtt_max_ < rtt){
    rtt_max_ = rtt;
  }

  if (rtt_avg_ == 0){
    rtt_avg_ = rtt;
  }
  else {
    rtt_avg_ = params_.AVG*rtt + (1-params_.AVG)*rtt_avg_;
  }
  rtt_ratio_ = rtt/rtt_min_;

}
