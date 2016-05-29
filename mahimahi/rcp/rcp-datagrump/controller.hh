#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <set>

enum Controller_Mode { NA, AIMD, AIMD_PROBABALISTIC, SIMPLE_DELAY, DOUBLE_THRESH };
/* Congestion controller interface */

struct SentPacket {
  uint64_t sequence_number;
  uint64_t sent_time;
  bool operator<(const SentPacket& rhs) const {
    return rhs.sequence_number < this->sequence_number;
  }
};

class Controller
{
private:
  bool debug_; /* Enables debugging output */

  /* Add member variables here */
  unsigned int win_size_;
  unsigned int timeout_;
  unsigned int min_rtt_thresh_;
  unsigned int max_rtt_thresh_;
  uint64_t last_rtt_timestamp_;
  Controller_Mode mode_;
  std::set<SentPacket> outstanding_packets_;
  uint64_t last_timeout_;
   
  /* For Probabalistic AIMD, these
   * are the three tunable paramaters */
  uint64_t timeout_reset_;
  uint64_t rand_linear_;
  float timeout_multiplier_;
  uint64_t minimum_rand_target_;

  bool is_timeout(uint64_t current_time);
  void remove_outstanding_packet(uint64_t seqno);

public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* A timeout was received */
  void timeout_received( void );

  void set_params(uint64_t rtt_timeout, uint64_t timeout_reset, 
                  uint64_t rand_linear, float timeout_multiplier,
                  uint64_t min_rand_target);

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms( void );
};

#endif
