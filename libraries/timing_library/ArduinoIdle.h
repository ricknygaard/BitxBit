#ifndef __INCLUDE_ARDUINOIDLE__

#include "Common.h"

extern void ArduinoInit();
extern void ArduinoIdleFunction();
extern void ArduinoPushUdpPacket(u_int8 *buf);

#define __INCLUDE_ARDUINOIDLE__
#endif