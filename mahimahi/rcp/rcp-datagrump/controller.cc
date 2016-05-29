#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <cmath>

#define WINDOW_SIZE_FIXED 50
#define MAX_WIN_SIZE 1000
#define RTT_ADJUST_INTERVAL 1

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ),
    win_size_( 15 ), // Start window size higher than 1, avoid very slow start
    timeout_( 51 ),
    min_rtt_thresh_( 30 ),
    max_rtt_thresh_( 70 ),
    last_rtt_timestamp_(0),
    mode_( AIMD_PROBABALISTIC ),
    outstanding_packets_( ),
    last_timeout_( 0 ),
    timeout_reset_( 74 ),
    rand_linear_( 5205 ),
    timeout_multiplier_( 0.23 ),
    minimum_rand_target_( 3428 )
{}

void Controller::set_params(uint64_t rtt_timeout, uint64_t timeout_reset, uint64_t rand_linear, float timeout_multiplier, uint64_t min_rand_target) {
    if(rtt_timeout != 0) {
        timeout_ = rtt_timeout;
    }

    if(timeout_reset != 0) {
        timeout_reset_ = timeout_reset;
    }

    if(rand_linear != 0) {
        rand_linear_ = rand_linear;
    }

    if(timeout_multiplier != 0) {
        timeout_multiplier_ = timeout_multiplier;
    }

    if(min_rand_target != 0) {
        minimum_rand_target_ = min_rand_target;
    }
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << win_size_ << endl;
  }

  if (mode_ == NA) {
    return (WINDOW_SIZE_FIXED);
  } else {
    return (win_size_);
  }
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Keep track of sent packets and their times,
   * for our own implementation of timeouts */
  if (mode_ == SIMPLE_DELAY || mode_ == AIMD || mode_ == AIMD_PROBABALISTIC) {
    struct SentPacket sent = {sequence_number, send_timestamp};
    outstanding_packets_.insert(sent);

    /* Check if a timeout should have triggered. Also, don't call timeout too often.
     * We only call at most one timeout during TIMEOUT_RESET. This is because we can get
     * a batch of timeout packets that may reduce our window too much. */
    if(is_timeout(send_timestamp) && (send_timestamp - last_timeout_ > timeout_reset_)) {
        timeout_received();
        last_timeout_ = send_timestamp;
    } else if (last_timeout_ == 0) {
      /* Improve startup behavior by sending last_timeout time to NOW.
       * This allows for a higher probability of increasing send window size
       * on startup (otherwise last_timeout ~= infinute, so relying on the 
       * fixed minimum percent chance to send */
      last_timeout_ = send_timestamp;
    }
  } 

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* Iterate through outstanding_packets_,
 * check if one or more has timed out */
bool Controller::is_timeout(uint64_t current_time) {
    bool timeout = 0;

    std::set<SentPacket>::iterator it;
    it = outstanding_packets_.begin();
    while (it != outstanding_packets_.end())
    {
       if(current_time - (*it).sent_time > timeout_) {
         it = outstanding_packets_.erase(it);
         timeout = 1;
       } else {
         ++it;
       }
    }

    return timeout;
}


void Controller::remove_outstanding_packet(uint64_t seqno) {
    struct SentPacket acked = {seqno, 0};
    std::set<SentPacket>::iterator it = outstanding_packets_.find(acked);
    if(it != outstanding_packets_.end()) {
      outstanding_packets_.erase(it);
    }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  if(mode_ == AIMD || mode_ == AIMD_PROBABALISTIC) {
    // Remove ACK'd packet from outstanding_packets
    remove_outstanding_packet(sequence_number_acked);
  }

  unsigned int rtt;
  rtt = timestamp_ack_received - send_timestamp_acked;
  if (mode_ == AIMD) {
      win_size_++;
  } else if (mode_ == AIMD_PROBABALISTIC) {
      /* Don't want to increase window size too quickly
       * when there's no timeouts for a while - indicative of
       * overshooting the network capacity */
     uint64_t time_since_timeout = timestamp_ack_received - last_timeout_;
     if((uint64_t)(rand() % (rand_linear_)) > std::min((uint64_t) time_since_timeout*time_since_timeout, minimum_rand_target_)) {
        win_size_++;
     }
   } else if (mode_ == SIMPLE_DELAY) {
    if (rtt < max_rtt_thresh_) {
      win_size_++;
    } else {
      win_size_ = win_size_ * max_rtt_thresh_ / rtt;
      if (win_size_ == 0)
        win_size_ = 1;
    }
  } else if (mode_ == DOUBLE_THRESH) {
    if (timestamp_ack_received - last_rtt_timestamp_ > RTT_ADJUST_INTERVAL) {
      last_rtt_timestamp_ = timestamp_ack_received;
      if (rtt < min_rtt_thresh_) {
        win_size_ = win_size_ * min_rtt_thresh_ / rtt + 1;
      }
      else if (rtt < max_rtt_thresh_) {
        win_size_++;
      } else {
        win_size_ = win_size_ * max_rtt_thresh_ / rtt;
      }
      if (win_size_ == 0)
        win_size_ = 1;
      if (win_size_ > MAX_WIN_SIZE)
        win_size_ = MAX_WIN_SIZE;
    }
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}


/* A timeout was received */
void Controller::timeout_received( void )
{
  if ((mode_ == AIMD) || (mode_ == AIMD_PROBABALISTIC)) {
    win_size_ = std::max(1, (int) (win_size_ * timeout_multiplier_) );
  }
  return;
}


/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return (timeout_); /* timeout of one second */
}
