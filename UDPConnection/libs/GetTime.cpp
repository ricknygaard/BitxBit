#include <stdafx.h>
#include "GetTime.h"

static __int64 tLaptopAtReset = 0;

// Hack to define gettimeofday on Windows
//
struct timeval
{
  __int32 tv_sec;
  __int32 tv_usec;
};

///////////////////////////////////////////////////////////////////////////////
static void gettimeofday(struct timeval *tv, void *)
{
  // NOTE: This stupid call pretends to operate in 100 nsec, but actually only
  // has msec resolution.  Thank you, Microsoft.
  //
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);

  __int64 tSys = ft.dwHighDateTime;
  tSys <<= 32;
  tSys += ft.dwLowDateTime;
  tSys /= 10I64;  // Convert from 0.1 usec to 1 usec units

  __int64 seconds = tSys / 1000000I64;
  __int64 usec    = tSys - (seconds * 1000000I64);

  tv->tv_sec  = (__int32) seconds;
  tv->tv_usec = (__int32) usec;
}

///////////////////////////////////////////////////////////////////////////////
void ResetLaptopTime()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);

  tLaptopAtReset = tp.tv_sec;
  tLaptopAtReset *= 1000000;
  tLaptopAtReset += tp.tv_usec;
}

///////////////////////////////////////////////////////////////////////////////
__int64 GetLaptopUsec()
{
  // This is host time in microseconds.
  //
  struct timeval tp;
  gettimeofday(&tp, NULL);

  __int64 tLaptop = tp.tv_sec;
  tLaptop *= 1000000;
  tLaptop += tp.tv_usec;

  tLaptop -= tLaptopAtReset;

  return tLaptop;
}

///////////////////////////////////////////////////////////////////////////////
u_int32 GetLaptopCmdTimeFromUsec(__int64 tLaptopUsec)
{
  tLaptopUsec >>= 2;  // Divide by 4 to get units of 4 usec

  return (u_int32) tLaptopUsec;  // bottom 32 bits
}

///////////////////////////////////////////////////////////////////////////////
u_int32 GetLaptopCmdTime()
{
  return GetLaptopCmdTimeFromUsec(GetLaptopUsec());
}

///////////////////////////////////////////////////////////////////////////////
// Hack to define micros() on Windows
//
static __int32 micros()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);

  __int64 tNow = tp.tv_sec;
  tNow *= 1000000;
  tNow += tp.tv_usec;
  tNow &= 0xfffffffc;  // micros() time tick is 4 usec
  return (__int32) tNow;
}

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
