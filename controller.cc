#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;
using namespace std;

int acks = 0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), cwnd(1), reduced(0), thres(50), table{0}, pointer_table(0),min_table(1<<20)
{
	std::srand(std::time(0));
	cwnd = std::rand()%100+1;
	cwnd = 100;
	for (int i=0; i<1000;i++)
		table[i]=50;
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
  /* Default: take no action */

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );

	  }
	int diff = recv_timestamp_acked - send_timestamp_acked;
	fprintf( stderr, "the difference is %d.\n", diff);
	cwnd_from_delay2(diff);
	//cwnd += 1.0/cwnd;
	cerr<<"min table = "<<min_table<<endl;

}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void )
{

  //if(acks < 1000)
    return 100; /* timeout of one second */
  //else
  //  return 2*min_table;
  //cwnd/=2;
  //cwnd = max(cwnd, 1.0);
}

void Controller::cwnd_from_delay( int diff )
{
	if (diff>1000)
		cwnd = cwnd;
	else if (diff > 800 && reduced<=6){
		cwnd = cwnd/2;
		reduced=7;
	}else if (diff > 600 && reduced<=5){
		cwnd = cwnd/2;
		reduced=6;
	}else if (diff > 400 && reduced<=4){
		cwnd = cwnd/2;
		reduced=5;
	}else if (diff > 300 && reduced<=3){
		cwnd = cwnd/2;
		reduced=4;
	}else if (diff > 200 && reduced<=2){
		cwnd = cwnd/2;
		reduced=3;
	}else if (diff > 100 && reduced<=1){
		cwnd = cwnd/2;
		reduced=2;
	}else if (diff > 50 && reduced<=0){
		cwnd = cwnd/2;
		reduced=1;
	}else if (diff<50){
		++cwnd;
		reduced = 0;
	}

}

void Controller::add_table( int item)
{
	table[pointer_table] = item;
	min_table = (item<min_table)?item:min_table;
	++pointer_table;
	pointer_table = pointer_table%800;

}	

void Controller::cwnd_from_delay2( int item)
{
	add_table(item);	

	int smaller = 0;
	int randitem = 0;
	for (int i=0 ; i< 100 ;i++){
		randitem = table[std::rand()%1000];
		if (item> randitem)
			++smaller;
	}
	fprintf( stderr, "percentile: %d.\n", smaller);
	if (smaller>80 && reduced<=0){
		cwnd = cwnd/2;
		reduced = 1;
	}else if(smaller<5){
		cwnd+=1.5;
		reduced = 0;
	}else if(smaller<10){
		cwnd+=1;
		reduced = 0;
	}else if(smaller<30){
		cwnd+=(double)(1/cwnd);
		reduced = 0;
	}
	if (cwnd < 1)
 	  cwnd=1;

}

void Controller::cwnd_from_score( int diff)
{
	int item =  (diff * 100)/cwnd;
	fprintf( stderr, "score: %d.\n", item);
	add_table(item);	

	int smaller = 0;
	int randitem = 0;
	for (int i=0 ; i< 100 ;i++){
		randitem = table[std::rand()%1000];
		if (item> randitem)
			++smaller;
	}
	fprintf( stderr, "percentile: %d.\n", smaller);
	if (smaller>80 && reduced<=0){
		cwnd = cwnd/2;
		reduced = 1;
	}else if(smaller<10){
		++cwnd;
		reduced = 0;
	}else if(smaller<30){
		cwnd+=(double)(1/cwnd);
		reduced = 0;
	}
	if (cwnd < 1)
		cwnd=1;
	if (item >2000)
		++cwnd;

}
