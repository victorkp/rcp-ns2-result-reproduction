/* UDP sender for congestion-control contest */

#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

#include "socket.hh"
#include "contest_message.hh"
#include "controller.hh"
#include "poller.hh"
#include "timestamp.hh"

using namespace std;
using namespace PollerShortNames;

/* simple sender class to handle the accounting */
class DatagrumpSender
{
private:
  UDPSocket socket_;
  Controller controller_; /* your class */

  uint64_t sequence_number_; /* next outgoing sequence number */

  /* if network does not reorder or lose datagrams,
     this is the sequence number that the sender
     next expects will be acknowledged by the receiver */
  uint64_t next_ack_expected_;

  uint64_t start_timestamp_;

  int send_datagram( int RCP_pkt_type );
  void got_ack( const uint64_t timestamp, const ContestMessage & msg );
  bool window_is_open( void );

/* Added for RCP */
  uint64_t lastpkttime_;
  uint64_t rtt_;
  uint64_t min_rtt_;
  uint64_t interval_;
  uint64_t numpkts_;
  uint64_t num_sent_;
  int RCP_state_;
  mutex RCP_state_mutex_;
  condition_variable RCP_state_cv_;

public:
  DatagrumpSender( const char * const host, const char * const port,
		   const bool debug );
  int loop( void );
  int senderloop( void );
  thread loopThread( void ) {
    thread loopthread(&DatagrumpSender::loop, this);
    return loopthread;
  }
  thread senderloopThread( void ) {
    thread senderloopthread(&DatagrumpSender::senderloop, this);
    return senderloopthread;
  }

/* Added for RCP */
  void timeout();
  void ref_timeout();
  int rcp_recv( const ContestMessage & msg );
};

uint64_t rtt_timeout = 0;
uint64_t timeout_reset = 0;
uint64_t rand_linear = 0;
float timeout_multiplier = 0;
uint64_t min_rand_target = 0;
uint64_t flow_size = 10000;
uint64_t pkt_size = 960;
uint64_t flow_running = 0;
mutex flow_mutex;
condition_variable flow_cv;
#define NUM_FLOW 10
#define RCP_DESIRED_RATE -1
#define POLL_TIMEOUT 1000

int main( int argc, char *argv[] )
{
  int i;
  thread t[NUM_FLOW];
  thread ts[NUM_FLOW];
  DatagrumpSender* sender[NUM_FLOW];

   /* check the command-line arguments */
  if ( argc < 1 ) { /* for sticklers */
    abort();
  }

  bool debug = false;

  if ( argc == 4 and string( argv[ 3 ] ) == "debug" ) {
    debug = true;
  } else if ( argc == 3 ) {
    /* do nothing */
  } else if (argc == 8) {
      cout << "Using specified training arguments" << endl;
      rtt_timeout = atoi(argv[3]);
      timeout_reset = atoi(argv[4]);
      rand_linear = atoi(argv[5]);
      timeout_multiplier = atof(argv[6]);
      min_rand_target = atoi(argv[7]);
  } else {
    cerr << "Usage: " << argv[ 0 ] << " HOST PORT [debug]" << endl;
    return EXIT_FAILURE;
  }

  flow_running = 0;
  /* create sender object to handle the accounting */
  /* all the interesting work is done by the Controller */

  for (i = 0; i < NUM_FLOW; i++) {
    sender[i] = new DatagrumpSender( argv[ 1 ], argv[ 2 ], debug );
    t[i] = sender[i]->loopThread();
    ts[i] = sender[i]->senderloopThread();
  }

  unique_lock<mutex> lock(flow_mutex);
  while (flow_running != 0) {
    flow_cv.wait(lock);
  }
  for (i = 0; i < NUM_FLOW; i++) {
    t[i].join();
    ts[i].join();
    delete sender[i];
  }
  return (0);
}

DatagrumpSender::DatagrumpSender( const char * const host,
				  const char * const port,
				  const bool debug )
  : socket_(),
    controller_( debug ),
    sequence_number_( 0 ),
    next_ack_expected_( 0 ),
    start_timestamp_( 0 ),
    lastpkttime_( 0 ),
    rtt_( 0 ),
    min_rtt_( 0 ),
    interval_( 0 ),
    numpkts_( flow_size ),
    num_sent_( 0 ),
    RCP_state_( RCP_INACT ),
    RCP_state_mutex_(),
    RCP_state_cv_()
{
  /* turn on timestamps when socket receives a datagram */
  socket_.set_timestamps();

  /* connect socket to the remote host */
  /* (note: this doesn't send anything; it just tags the socket
     locally with the remote address */
  socket_.connect( Address( host, port ) );  

  cerr << "Sending to " << socket_.peer_address().to_string() << endl;
  unique_lock<mutex> lock(flow_mutex);
  flow_running++;
}

void DatagrumpSender::got_ack( const uint64_t timestamp,
			       const ContestMessage & ack )
{
  if ( not ack.is_ack() ) {
    throw runtime_error( "sender got something other than an ack from the receiver" );
  }

  /* Update sender's counter */
  next_ack_expected_ = max( next_ack_expected_,
			    ack.header.ack_sequence_number + 1 );

  rtt_ = timestamp - ack.header.ack_send_timestamp;
}

int DatagrumpSender::send_datagram( int RCP_pkt_type )
{
  if (sequence_number_ == 0)
    start_timestamp_ = timestamp_ns();

  /* All messages use the same dummy payload */
  static const string dummy_payload( pkt_size, 'x' );

  ContestMessage cm( sequence_number_++, RCP_pkt_type, rtt_, RCP_DESIRED_RATE, dummy_payload );
  cm.set_send_timestamp();
  socket_.send( cm.to_string() );

  if (sequence_number_ == flow_size) {
    return 1;
  } else {
    return 0;
  }
}

int DatagrumpSender::loop( void )
{
  /* read and write from the receiver using an event-driven "poller" */
  Poller poller;

  /* first rule: if the window is open, close it by
     sending more datagrams */
/*
  poller.add_action( Action( socket_, Direction::Out, [&] () {
	return ResultType::Continue;
      } ) );
*/

  /* second rule: if sender receives an ack,
     process it and inform the controller
     (by using the sender's got_ack method) */
  poller.add_action( Action( socket_, Direction::In, [&] () {
	const UDPSocket::received_datagram recd = socket_.recv();
	const ContestMessage ack  = recd.payload;
	got_ack( recd.timestamp, ack );
	if (rcp_recv(ack)) {
          return ResultType::Exit;
	};
	return ResultType::Continue;
      } ) );

  /* Run these two rules forever */
  while ( true ) {
    const auto ret = poller.poll( POLL_TIMEOUT );
    if ( ret.result == PollResult::Exit ) {
      return ret.exit_status;
    } else if ( ret.result == PollResult::Timeout ) {
      /* After a timeout, send one datagram to try to get things moving again */
    }
  }

  return (0);
}

int DatagrumpSender::senderloop()
{
  /* Start sendfile */
  lastpkttime_ = timestamp_ns();
  send_datagram(RCP_SYN);
  RCP_state_ = RCP_SYNSENT;
  num_sent_++;
  
  /* Wait for synack */
  unique_lock<mutex> rlock(RCP_state_mutex_);
  while (RCP_state_ != RCP_RUNNING) {
    RCP_state_cv_.wait(rlock);
  }
  rlock.unlock();
  
  /* send for every interval_ */
  while (true) {
    this_thread::sleep_for(chrono::nanoseconds(lastpkttime_ + interval_ - timestamp_ns()));
    if (RCP_state_ != RCP_RUNNING) {
	continue;
    }
    if (num_sent_ < numpkts_ - 1) {
      lastpkttime_ = timestamp_ns();
      send_datagram(RCP_DATA);
      num_sent_++;
    } else {
      /* send last */
      lastpkttime_ = timestamp_ns();
      send_datagram(RCP_FIN);
      num_sent_++;
      RCP_state_ = RCP_FINSENT;
      break;
    }
  }

  return (0);
}

int DatagrumpSender::rcp_recv( const ContestMessage & msg )
{
	switch (msg.header.RCP_pkt_type) {
	case RCP_SYNACK:
		if (min_rtt_ > rtt_)
			min_rtt_ = rtt_;

		if (msg.header.RCP_rate > 0) {
			interval_ = (int) (NANO_FACT * (pkt_size + RCP_HDR_BYTES)/(msg.header.RCP_rate));
    			unique_lock<mutex> rlock(RCP_state_mutex_);
			RCP_state_ = RCP_RUNNING;
			RCP_state_cv_.notify_all();
			rlock.unlock();
		}
		else {
			if (msg.header.RCP_rate < 0) 
				cout << "Error: RCP rate < 0: " << msg.header.RCP_rate << endl;
			RCP_state_ = RCP_CONGEST;
		}
		break;

	case RCP_ACK:
		if (msg.header.RCP_rate > 0) {
			interval_ = (uint64_t) (NANO_FACT * (pkt_size + RCP_HDR_BYTES)/(msg.header.RCP_rate));
			if (RCP_state_ == RCP_CONGEST)
				RCP_state_ = RCP_RUNNING;
		}
		else {
			cout << "Error: RCP rate < 0: " << msg.header.RCP_rate << endl;
			RCP_state_ = RCP_CONGEST; //can do exponential backoff or probalistic stopping here.
		}
		break;

	case RCP_FIN:
		send_datagram(RCP_FINACK);
		break;

	case RCP_SYN:
		send_datagram(RCP_SYNACK);
		break;

	case RCP_FINACK:
		{
		unique_lock<mutex> flock(flow_mutex);
    		cout << "time spent sending flow size " << flow_size << ": " << (timestamp_ns() - start_timestamp_)/1000000 << "ms" << endl;
    		flow_running--;
    		flow_cv.notify_all();
		flock.unlock();
		return (1);
		}

	case RCP_DATA:
		send_datagram(RCP_ACK);
		break;

	case RCP_OTHER:
		cout << "received RCP_OTHER\n" << endl;
		break;

	default:
		cout << "Unknown RCP packet type!\n" << endl;
		break;
	}
	return (0);
}
