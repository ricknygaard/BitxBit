#ifndef __INCLUDED_GETTIME__

#include "common.h"

// extern    void ResetLaptopTime();
// extern __int64 GetLaptopUsec();
// extern u_int32 GetLaptopCmdTime();
// extern u_int32 GetLaptopCmdTimeFromUsec(__int64 tLaptopUsec);
extern    void ResetArduinoTime();
extern u_int32 GetArduinoTime();

#define __INCLUDED_GETTIME__
#endif
