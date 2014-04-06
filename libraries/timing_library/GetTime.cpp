#include <Arduino.h>
#include <GetTime.h>

static u_int32 tArduinoAtReset = 0;  // units of 4 usec
static u_int32 numTimeRollovers = 0;

///////////////////////////////////////////////////////////////////////////////
void ResetArduinoTime()
{
  // Divide by 4 and mask to ensure there is no sign extension
  tArduinoAtReset  = (micros() >> 2) & 0x3fffffff;
  numTimeRollovers = 0;
}

///////////////////////////////////////////////////////////////////////////////
u_int32 GetArduinoTime()
{
  // The built-in micros() call rolls over every 70 minutes.  We assume here
  // that this function is called more often than that.
  //
  u_int32 tNow = (micros() >> 2) & 0x3fffffff;
  if (tNow < tArduinoAtReset)
  {
    ++numTimeRollovers;
  }

  // The bottom two bits of the rollover counter become the top two bits
  // of the current time.
  //
  tNow = (numTimeRollovers << 30) + tNow;

  return tNow - tArduinoAtReset;
}
