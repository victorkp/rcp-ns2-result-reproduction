/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TIMESTAMP_HH
#define TIMESTAMP_HH

#include <cstdint>

#define NANO_FACT 1000000000

uint64_t timestamp( void );
uint64_t initial_timestamp( void );
uint64_t timestamp_ns( void );

#endif /* TIMESTAMP_HH */
