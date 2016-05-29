/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DROPPING_PACKET_QUEUE_HH
#define DROPPING_PACKET_QUEUE_HH

#include <queue>
#include <cassert>
#include <atomic>

#include "abstract_packet_queue.hh"
#define TIMESLOT 1000000 /* 1ms */

class DroppingPacketQueue : public AbstractPacketQueue
{
private:
    const unsigned int packet_limit_;
    const unsigned int byte_limit_;

    int queue_size_in_bytes_ = 0, queue_size_in_packets_ = 0;
    std::queue<QueuedPacket> internal_queue_ {};

    virtual const std::string & type( void ) const = 0;

protected:
    const unsigned int link_capacity_; /* In bytes/s */
    uint64_t Tq_; /* Time interval for each stats collection. In nanosecond. */
    double alpha_;
    double beta_;
    unsigned int size_bytes( void ) const;
    unsigned int size_packets( void ) const;
    std::atomic<int> input_traffic_; /* In bytes */
    uint64_t avg_rtt_; /* In nanoseconds */
    int64_t flow_rate_; /* In bytes/s */
    uint64_t next_check_time_;

    /* put a packet on the back of the queue */
    void accept( QueuedPacket && p );

    /* are the limits currently met? */
    bool good( void ) const;
    bool good_with( const unsigned int size_in_bytes,
                    const unsigned int size_in_packets ) const;

public:
    DroppingPacketQueue( const std::string & args );

    virtual void enqueue( QueuedPacket && p ) = 0;

    QueuedPacket dequeue( void ) override;

    bool empty( void ) const override;

    std::string to_string( void ) const override;
};

#endif /* DROPPING_PACKET_QUEUE_HH */ 
