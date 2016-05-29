/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RCP_PACKET_QUEUE_HH
#define RCP_PACKET_QUEUE_HH

#include <sstream>
#include <algorithm>
#include <iostream>
#include "dropping_packet_queue.hh"
#include "timestamp.hh"

#define RTT_GAIN 0.02

enum RCP_PKT_T {
  RCP_OTHER,
  RCP_SYN,
  RCP_SYNACK,
  RCP_REF,
  RCP_REFACK,
  RCP_DATA,
  RCP_ACK,
  RCP_FIN,
  RCP_FINACK
};

class RCPPacketQueue : public DroppingPacketQueue
{
private:
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "rcp" };
        return type_;
    }

    /* helper to get the nth uint64_t field (in network byte order) */
    uint64_t get_header_field( const size_t n, const std::string & str )
    {
      const uint64_t * const data_ptr
        = reinterpret_cast<const uint64_t *>( str.data() ) + n;

      return be64toh( *data_ptr );
    }
    /* helper to put a uint64_t field (in network byte order) */
    void put_header_field( const size_t pos, std::string & str, const uint64_t val)
    {
      uint64_t network_order = htobe64( val );
      char ptr[8];
      ptr[7] = (uint8_t)(network_order>>56);
      ptr[6] = (uint8_t)(network_order>>48);
      ptr[5] = (uint8_t)(network_order>>40);
      ptr[4] = (uint8_t)(network_order>>32);
      ptr[3] = (uint8_t)(network_order>>24);
      ptr[2] = (uint8_t)(network_order>>16);
      ptr[1] = (uint8_t)(network_order>>8);
      ptr[0] = (uint8_t)(network_order>>0);
  
      str.replace(pos * 8, 8, ptr, 8);
    }

    uint64_t running_avg(uint64_t var_sample, uint64_t var_last_avg, double gain)
    {
	return (uint64_t)(gain * var_sample + ( 1 - gain) * var_last_avg);
    }
    void checkStat( void )
    {
      if (timestamp_ns() < next_check_time_)
        return;
      uint64_t last_traffic = input_traffic_;
      uint64_t last_load = last_traffic * (NANO_FACT / Tq_);
      double Q = (double)size_bytes();
      double ratio = (1 + ((((double)Tq_)/avg_rtt_)*(alpha_*(link_capacity_ - last_load) - beta_*(Q * NANO_FACT/avg_rtt_)))/link_capacity_);
      flow_rate_ = (int64_t)(flow_rate_ * ratio);
      if (flow_rate_ > link_capacity_)
        flow_rate_ = link_capacity_;
      Tq_ = std::min(avg_rtt_, (uint64_t)TIMESLOT);
      input_traffic_ -= last_traffic;
      next_check_time_ += Tq_;
    }

public:
    using DroppingPacketQueue::DroppingPacketQueue;

    void enqueue( QueuedPacket && p ) override
    {
        if ( good_with( size_bytes() + p.contents.size(),
                        size_packets() + 1 ) ) {
            input_traffic_ += p.contents.size();
            uint64_t rtt = get_header_field( 11, p.contents ); /* 11 is rtt field */
            if (rtt > 0)
                avg_rtt_ = running_avg(rtt, avg_rtt_, RTT_GAIN);
            
            int64_t request = get_header_field( 12, p.contents ); /* 12 is rate field */
            if (request < 0 || request > flow_rate_)
                put_header_field( 12, p.contents, 100);

            request = get_header_field( 12, p.contents ); /* 12 is rate field */
            accept( std::move( p ) );
        }

        assert( good() );
    };
};

#endif /* RCP_PACKET_QUEUE_HH */ 
