#include "Averager.h"

///////////////////////////////////////////////////////////////////////////////
void AveragerReset(AveragerT *pInfo)
{
  pInfo->index = 0;
}

///////////////////////////////////////////////////////////////////////////////
bool AverageIsValid(AveragerT *pInfo)
{
  return pInfo->index >= NUM_VALUES_IN_AVERAGER;
}

///////////////////////////////////////////////////////////////////////////////
bool AveragerAddNearbyValue(AveragerT *pInfo, u_int32 value)
{
  // Add value if average not established or if value is +/- 25% of average.
  //
  u_int32 average = pInfo->average;
  u_int32 delta   = average >> 2;  // divide by 4
  if ((pInfo->index < NUM_VALUES_IN_AVERAGER)
    || ((value > (average - delta)) && (value < (average + delta))))
  {
    return AveragerAddValue(pInfo, value);
  }

  return false;  // The value was not added.  No change to the average.
}

///////////////////////////////////////////////////////////////////////////////
bool AveragerAddValue(AveragerT *pInfo, u_int32 value)
{
  u_int32 *pSamples = pInfo->samples;

  pSamples[(pInfo->index) & AVERAGER_INDEX_MASK] = value;
  pInfo->index += 1;

  if (pInfo->index >= NUM_VALUES_IN_AVERAGER)
  {
    // Enough samples to generate the average.
    // This assumes NUM_VALUES_IN_AVERAGER is 4.
    // Use an int64_t to handle overflow during addition of four u_int32's.
    //
    int64_t average = pSamples[0];
    average += pSamples[1];
    average += pSamples[2];
    average += pSamples[3];
    average >>= 2;  // Divide by 4
    pInfo->average = (u_int32) average;

    return true;  // The value was added and the average was updated.
  }

  return false;  // The value was not added.  No change to the average.
}
