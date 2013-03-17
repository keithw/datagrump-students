#include <stdio.h>

#include <algorithm>
#include <set>
#include <vector>

#include "controller.hh"
#include "timestamp.hh"

namespace {
  const double ADDITIVE_INCREASE = 0.5;
  const double MULTIPLICATIVE_DECREASE = 0.8;
}

using namespace Network;

typedef std::multimap<uint64_t, uint64_t> PacketTimestamp;
typedef PacketTimestamp::iterator PacketTimestampIterator;
typedef std::vector<PacketTimestampIterator> PacketTimestampIteratorList;
typedef PacketTimestampIteratorList::iterator
        PacketTimestampIteratorListIterator;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    my_window_size_(1.0),
    sent_packet_timestamps_() {
}

/* Get current window size, in packets */
unsigned int Controller::window_size( void ) {
  // ---------------------------------------------------------------------------
  // Check for lost packets.
  // ---------------------------------------------------------------------------
  uint64_t current_time = timestamp();

  // Find the lost packet timestamps.
  PacketTimestampIteratorList lost_packet_timestamps;
  for (PacketTimestampIterator it = sent_packet_timestamps_.begin();
       it != sent_packet_timestamps_.end();
       it++) {
    uint64_t send_timestamp = (*it).second;
    if (send_timestamp + this->timeout_ms() < current_time) {
      if (debug_) {
        fprintf(stderr, "No ack for packet sent at %lu\n", send_timestamp);
      }
      lost_packet_timestamps.push_back(it);
    }
  }

  // Remove the lost timestamps.
  for (PacketTimestampIteratorListIterator it = lost_packet_timestamps.begin();
       it != lost_packet_timestamps.end();
       it++) {
    sent_packet_timestamps_.erase(*it);
    // Update the window size for the lost packet.
    my_window_size_ *= MULTIPLICATIVE_DECREASE;
  }

  if ( debug_ ) {
    fprintf( stderr, "At time %lu, return window_size = %f.\n",
	     timestamp(), my_window_size_ );
  }

  return static_cast<int>(my_window_size_);
}

/* A packet was sent */
void Controller::packet_was_sent( const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp ) {
          /* in milliseconds */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number );
  }

  // Update the multiset of send timestamps.
  sent_packet_timestamps_.insert(
      std::pair<uint64_t, uint64_t>(sequence_number, send_timestamp));
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received ) {
             /* when the ack was received (by sender) */
  if ( debug_ ) {
    fprintf( stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked );

    fprintf( stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked );
  }

  // Remove the packet from the multiset.
  PacketTimestampIterator iterator
      = sent_packet_timestamps_.find(sequence_number_acked);
  if (iterator != sent_packet_timestamps_.end()) {
    sent_packet_timestamps_.erase(iterator);
    // Update the window size.
    my_window_size_ += std::min(ADDITIVE_INCREASE / my_window_size_, 1.0);
  }
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms( void ) {
  return 1000; /* timeout of one second */
}
