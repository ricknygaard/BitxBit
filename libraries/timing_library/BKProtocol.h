#ifndef __INCLUDED_BKPROTOCOL__

#include "common.h"

#define UDP_PAYLOAD_SIZE        (6)
#define UDP_PAYLOAD_TIME_OFFSET (0)
#define UDP_PAYLOAD_CMD_OFFSET  (4)
#define UDP_PAYLOAD_ID_OFFSET   (5)

const unsigned short UDP_CMD_PORT = 37148;

typedef enum
{
  UDPCMD_SYNCH = 0,
  UDPCMD_STOP  = 1,
  UDPCMD_START = 2,
  UDPCMD_TEMPO = 3,
  UDPCMD_FILL  = 4,
  UDPCMD_NONE  = 255
} UdpCmd;

typedef enum
{
  CLIPID_NONE          = -1,
  CLIPID_FIRST_OF_SHOW = 20
} ClipID;

#define __INCLUDED_BKPROTOCOL__
#endif
