#ifndef __INCLUDED_AVERAGER__

#include "common.h"

const __int32 NUM_VALUES_IN_AVERAGER = 4;    // Must be power of two
const __int32 AVERAGER_INDEX_MASK    = 0x03; // Expose meaningful part of index

typedef struct
{
  u_int32 samples[NUM_VALUES_IN_AVERAGER];
  u_int32 average;
  u_int32 index;
} AveragerT;

extern void AveragerReset(AveragerT *pInfo);
extern bool AverageIsValid(AveragerT *pInfo);
extern bool AveragerAddNearbyValue(AveragerT *pInfo, u_int32 value);
extern bool AveragerAddValue(AveragerT *pInfo, u_int32 value);

#define __INCLUDED_AVERAGER__
#endif
