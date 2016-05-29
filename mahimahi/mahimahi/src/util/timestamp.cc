/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <ctime>

#include "timestamp.hh"
#include "exception.hh"

/* nanoseconds per second */
static const uint64_t BILLION = 1000000000;

uint64_t raw_timestamp( void )
{
    timespec ts;
    SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ts ) );

    uint64_t millis = ts.tv_nsec / 1000000;
    millis += uint64_t( ts.tv_sec ) * 1000;

    return millis;
}

uint64_t initial_timestamp( void )
{
    static uint64_t initial_value = raw_timestamp();
    return initial_value;
}

uint64_t timestamp( void )
{
    return raw_timestamp() - initial_timestamp();
}

/* helper functions */
static timespec current_time( void )
{
  timespec ret;
  SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ret ) );
  return ret;
}

uint64_t timestamp_ns( void )
{
  timespec ts = current_time();
  return (ts.tv_sec * BILLION + ts.tv_nsec);
}
