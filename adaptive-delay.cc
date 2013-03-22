#include <stdio.h>

#include "adaptive-delay.hh"
#include "timestamp.hh"

using namespace Network;

/* Default constructor */
AdaptDelayController::AdaptDelayController( const bool debug, const unsigned int cwnd, const uint64_t delay_threshold )
  : Controller(debug, cwnd), delay_threshold(delay_threshold), tx_delay(0), sim_queue(std::deque<PacketData>()), recv_queue(std::deque<PacketData>()) 
{
  if ( debug_ ) {
    fprintf( stderr, "Using Adaptive Delay Controller!\n");
  }
}

/* A packet was sent */
void AdaptDelayController::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp, bool is_retransmit )
                                  /* in milliseconds */
{
  /* Default: take no action */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
    if( is_retransmit )
      fprintf( stderr , "Retransmit \n" );
  }

  /* Enqueue the PacketData into the sim_queue */
  sim_queue.push_back(PacketData(sequence_number, send_timestamp));
}

/* An ack was received */
void AdaptDelayController::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  /* Check front of queue for packet drops */
  while(sim_queue.front().sequence_number != sequence_number_acked){
    sim_queue.pop_front();
    tx_delay = recv_timestamp_acked - send_timestamp_acked;
  }

  /* Assign receive time stamp */
  sim_queue.front().recv_timestamp = recv_timestamp_acked;

  /* Cases for coarse tx_delay estimate: 
    - Throughput is low / Link goes down
    - Packet was not queued  
  */
  bool coarse_delay = true;
  if(sim_queue.front().was_queued && !recv_queue.empty()){
    tx_delay = 0.2*tx_delay + 0.8*(recv_timestamp_acked - recv_queue.back().recv_timestamp);
  } else {
    tx_delay = recv_timestamp_acked - PROPDELAY - send_timestamp_acked;
  }

  /* Fine tx_delay estimation: Prune old/non contiguous elements of the recv_queue */
  while(!recv_queue.empty() && ((sim_queue.front().recv_timestamp - recv_queue.front().recv_timestamp)) > 2){
    fprintf( stderr, "Receive Diffence %g\n", (sim_queue.front().recv_timestamp - recv_queue.front().recv_timestamp));
    recv_queue.pop_front();
  }

  if(recv_queue.size() > 1){ 
    tx_delay = (sim_queue.front().recv_timestamp - recv_queue.front().recv_timestamp)/recv_queue.size();
    coarse_delay = false;
    fprintf( stderr, "Receive Queue Size: %d\n", (int) recv_queue.size());
  }

  /* Clip estimate to known maximum */
  tx_delay = std::max(tx_delay, MINTX);

  if(!sim_queue.front().was_queued) recv_queue.clear();
  recv_queue.push_back(sim_queue.front());
  sim_queue.pop_front();

  /* Propogate tx_delay informtaion through the sim_queue */
  double last_recv_timestamp = recv_timestamp_acked;
  if(!sim_queue.empty()){
    std::deque<PacketData>::iterator currPacketP = sim_queue.begin();
    while(currPacketP != sim_queue.end()){
      if((last_recv_timestamp + tx_delay) > (currPacketP->send_timestamp + PROPDELAY)){
        currPacketP->recv_timestamp = last_recv_timestamp + tx_delay;
        currPacketP->was_queued = true;
      } else {
        currPacketP->recv_timestamp = currPacketP->send_timestamp + PROPDELAY;
        currPacketP->was_queued = false;
      }

      double packet_delay = currPacketP->recv_timestamp-currPacketP->send_timestamp;
      fprintf( stderr, "Propagation %lu - delay: %g, l_r_ts:%g, r_ts: %g, queued: %c\n", currPacketP->sequence_number, packet_delay, (last_recv_timestamp-recv_timestamp_acked), (currPacketP->recv_timestamp-recv_timestamp_acked), (currPacketP->was_queued)?'Y':'N' );
      last_recv_timestamp = currPacketP->recv_timestamp;
      ++currPacketP;
    }
  }

  /* Now calculate how much delay will the newest packet in the queue see */
  double delay_estimate;
  if( last_recv_timestamp > (timestamp_ack_received + PROPDELAY)){
    delay_estimate = last_recv_timestamp - timestamp_ack_received + tx_delay;
  } else {
    delay_estimate = tx_delay + PROPDELAY;
  }
  
  double delay_slack = PROPDELAY + tx_delay + delay_threshold - delay_estimate;
  double delay_next_recvd = 0;
  if(cwnd > 1){
    delay_next_recvd = sim_queue.front().recv_timestamp - recv_timestamp_acked;
  } else {
    delay_next_recvd = timestamp_ack_received + PROPDELAY + tx_delay - recv_timestamp_acked;
  }
  int delta_cwnd = (delay_slack>0) ? (int) (std::min(delay_slack, delay_next_recvd)/tx_delay) : -1;
  cwnd += delta_cwnd;
  if(cwnd == 0) cwnd = 1;

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
             timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
             send_timestamp_acked, recv_timestamp_acked );
    
    fprintf( stderr, "Control Decisions - tx_delay: %g, delay_type: %c, delay_estimate %g, delta_cwnd: %d, cwnd: %d\n", tx_delay, (coarse_delay)?'C':'F', delay_estimate, delta_cwnd, cwnd );
  }

}
