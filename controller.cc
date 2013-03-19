#include <stdio.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace Network;

#define TICK_LEN (25)
#define TIMEOUT_MS (100)
#define EWMA_GAIN (.2)
#define TAU (60)
#define STARTING_WINDOW (0.85 * TAU)

/* Default constructor */
Controller::Controller(const bool debug)
  : debug_(debug),
    first_recv_time(0),
    cur_pkt_count(0),
    last_tick_time(0),
    throughput(-1)
{
  fprintf(stderr, "Using forecast model!\n");
}

void Controller::update_estimate(uint64_t cur_time, uint64_t recent_delay) {
  // If we've stepped into a new tick.
  if (cur_time - last_tick_time >= TICK_LEN) {
    uint64_t threshold_time = (cur_time - (cur_time % TICK_LEN)) - TICK_LEN;
    if ((first_recv_time != 0) &&
        (threshold_time >= first_recv_time)) {
      // Update throughput
      double recent_throughput = 1.0*cur_pkt_count / TICK_LEN;
      if (recent_delay != 0) {
        if (recent_throughput < 1.0 / recent_delay) {
          recent_throughput = 1.0 / recent_delay;
        }
      }
      // Just use recent_throughput if not initialized.
      if (throughput < 0) {
        throughput = recent_throughput;
      } else {
        throughput = (1-EWMA_GAIN)*throughput + EWMA_GAIN*recent_throughput;
        if (recent_delay != 0) {
          if (throughput < 1.0 / recent_delay) {
            throughput = 1.0 / recent_delay;
          }
        }
      }
    }

    cur_pkt_count = 0;
    // Round down to nearest TICK_LEN.
    last_tick_time = cur_time - (cur_time % TICK_LEN);
  }
}

/* Get current window size, in packets */
unsigned int Controller::window_size(void)
{
  unsigned int the_window_size;

  if (throughput < 0) {
    the_window_size = STARTING_WINDOW;
  } else {
    // We want W = TP * DELAY
    the_window_size = (unsigned int)(throughput * TAU);
  }

  if (debug_) {
    fprintf(stderr, "At time %lu, return window_size = %d.\n",
	     timestamp(), the_window_size);
  }

  return the_window_size;
}

/* A packet was sent */
void Controller::packet_was_sent(
          const uint64_t sequence_number,
				  /* of the sent packet */
				  const uint64_t send_timestamp)
          /* in milliseconds */
{
  if (debug_) {
    fprintf(stderr, "At time %lu, sent packet %lu.\n",
	     send_timestamp, sequence_number);
  }
  update_estimate(send_timestamp, 0);
}

/* An ack was received */
void Controller::ack_received(const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged packet was sent */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged packet was received */
			       const uint64_t timestamp_ack_received)
                               /* when the ack was received (by sender) */
{
  if (debug_) {
    fprintf(stderr, "At time %lu, received ACK for packet %lu",
	     timestamp_ack_received, sequence_number_acked);

    fprintf(stderr, " (sent %lu, received %lu by receiver's clock).\n",
	     send_timestamp_acked, recv_timestamp_acked);
  }

  if (first_recv_time == 0) {
    first_recv_time = timestamp_ack_received;
  }
  update_estimate(timestamp_ack_received, recv_timestamp_acked - send_timestamp_acked);
  // It's important that we do this after update_estimate!
  cur_pkt_count++;
}

/* How long to wait if there are no acks before sending one more packet */
unsigned int Controller::timeout_ms(void)
{
  return TIMEOUT_MS; /* timeout of one second */
}

