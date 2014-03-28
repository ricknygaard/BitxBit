#include "stdafx.h"

#include "Averager.h"
#include "BKProtocol.h"
#include "GetTime.h"
#include "LaptopIdle.h"
#include "MovementInfo.h"
#include "ArduinoIdle.h"

static char *movementFilePath =
  "S:\\Programs\\ScreenGrabber_BlueKnights\\ScreenGrabber\\Code\\Test1\\Movements.txt";

const __int64 IDLE_SYNCH_INTERVAL = 100000I64;  // usec between SYNCH when idle

const bool AUTO_START_NEXT_MOVEMENT = false;

typedef struct
{
  AveragerT avgTempo;
  ClipID    prevClipID;
  ClipID    currentClipID;
  __int64   tLastUdp;
  __int64   tLastTempoKey;
  __int64   tClipStarted;
  __int64   tClipWillEnd;
  __int64   tNextIdleUdpDue;
  u_int32   usecPerFrame;
  u_int32   currentFrameTempo;
  double    usecPerBeat;
} LaptopStateT;

static LaptopStateT laptopState;

///////////////////////////////////////////////////////////////////////////////
// PRIVATE TEMP HACK
static char *cmdStrings[] =
{
  "Synch",
  "Stop ",
  "Start",
  "Tempo"
};

///////////////////////////////////////////////////////////////////////////////
// PRIVATE TEMP HACK
static void SendUdp(u_int8 *payload, __int32 numBytes)
{
  u_int32 *pTime = (u_int32*) &(payload[UDP_PAYLOAD_TIME_OFFSET]);

  fprintf(stdout,
    "UDP[%d] %s %2d %d\n",
    numBytes,
    cmdStrings[payload[UDP_PAYLOAD_CMD_OFFSET]],
    (int) payload[UDP_PAYLOAD_ID_OFFSET],
    *pTime
    );

  ArduinoPushUdpPacket(payload);
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static __int32 BusyWait(__int32 tWaitUsec)
{
  __int64 tWaitUsec64 = tWaitUsec;
  int x = 0;
  while ((GetLaptopUsec() - laptopState.tLastUdp) < tWaitUsec64)
  {
    ++x; // Spin, spin.  (Let the compiler think we're doing something.)
  }

  return x;
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static void SendCommand(
  UdpCmd  cmd,
  ClipID  clipID,
  u_int32 tValue
  )
{
  u_int8 payload[UDP_PAYLOAD_SIZE];

  payload[UDP_PAYLOAD_CMD_OFFSET] = (u_int8) cmd;
  payload[UDP_PAYLOAD_ID_OFFSET]  = (u_int8) clipID;

  u_int32 *pTime = (u_int32*) &(payload[UDP_PAYLOAD_TIME_OFFSET]);
  *pTime = tValue;

  SendUdp(payload, UDP_PAYLOAD_SIZE);
  laptopState.tLastUdp = GetLaptopUsec();
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static void StartPlaying(bool bRestart)
{
  if (bRestart)
  {
    laptopState.currentClipID = GetFirstClipIDInTheShow();
  }
  else  // Get the next one.  Get none if at the end.  Do not start over.
  {
    if (laptopState.currentClipID > CLIPID_NONE)
    {
      laptopState.prevClipID    = laptopState.currentClipID;
      laptopState.currentClipID = GetNextClipID(laptopState.currentClipID);
    }
    else if (laptopState.prevClipID > CLIPID_NONE)
    {
      // nothing running right now, use what was running
      //
      laptopState.currentClipID = GetNextClipID(laptopState.prevClipID);
    }
  }

  // If none to play, reset the previous, too, and quit.
  //
  if (laptopState.currentClipID == CLIPID_NONE)
  {
    laptopState.prevClipID = CLIPID_NONE;
    return;
  }

  // Go!
  //
  // Send 20 commands over about 0.1 seconds.
  //
  // Calculate the desired start time (about midway through the repetitions).
  //   0.5 * 0.1 seconds = 50000 usec
  //
  laptopState.tClipStarted = GetLaptopUsec() + 50000;  // 50 msec
  __int32 tStartCmd = GetLaptopCmdTimeFromUsec(laptopState.tClipStarted);

  for (int i = 0; i < 20; ++i)
  {
    SendCommand(UDPCMD_START, laptopState.currentClipID, tStartCmd);
    BusyWait(5000);  // 0.1 seconds / 20 repetitions = 5 msec
  }

  MovementInfoT *pCurrentMovement =
    GetMovementInfoForClipID(laptopState.currentClipID);

  laptopState.usecPerFrame = pCurrentMovement->usecPerFrame;
  laptopState.usecPerBeat  = pCurrentMovement->usecPerBeat;
  laptopState.currentFrameTempo = 0;  // Will go nonzero when updated.

  laptopState.tClipWillEnd =
    laptopState.tClipStarted + pCurrentMovement->usecTotal;

  laptopState.tLastTempoKey = 0;
  AveragerReset(&(laptopState.avgTempo));

  // Repeat tempo or synch commands every two frames while running
  //
  laptopState.tNextIdleUdpDue  = laptopState.tLastUdp;
  laptopState.tNextIdleUdpDue += (__int64) (2 * laptopState.usecPerFrame);
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static void StopPlaying()
{
  // Stop!
  // Send 1 command per frame 20 times.
  //
  for (int i = 0; i < 20; ++i)
  {
    // Always get the current time immediately before sending STOP command
    // because the receiver uses the time value in the command to improve
    // the synchronization to laptop time.
    //
    SendCommand(UDPCMD_STOP, laptopState.currentClipID, GetLaptopCmdTime());
    BusyWait(laptopState.usecPerFrame);  // Typically 33 or 50 msec
  }

  laptopState.prevClipID    = laptopState.currentClipID;
  laptopState.currentClipID = CLIPID_NONE;

  // Drop back to the non-playing idle message rate.
  //
  laptopState.tNextIdleUdpDue = laptopState.tLastUdp + IDLE_SYNCH_INTERVAL;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void LaptopInit()
{
  ResetLaptopTime();
  AveragerReset(&(laptopState.avgTempo));
  ReadMovementFile(movementFilePath);

  laptopState.prevClipID      = CLIPID_NONE;
  laptopState.currentClipID   = CLIPID_NONE;
  laptopState.tLastUdp        = GetLaptopUsec();
  laptopState.tNextIdleUdpDue = laptopState.tLastUdp + IDLE_SYNCH_INTERVAL;
  laptopState.tLastTempoKey   = 0;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void LaptopIdleFunction(KeyCode key)
{
  // Look for a keyboard command.
  //
  switch (key)
  {
    case KEYCODE_BEGIN: StartPlaying(true);   break;
    case KEYCODE_NEXT:  StartPlaying(false);  break;
    case KEYCODE_STOP:  StopPlaying();        break;

    case KEYCODE_RESET:
      // Only do a reset when nothing is running.
      //
      if (laptopState.currentClipID == CLIPID_NONE)
      {
        LaptopInit();
      }
      break;
  }

  __int64 tNow = GetLaptopUsec();

  // Is a movement clip running?  Is it time to advance to the next one?
  //
  if (laptopState.currentClipID != CLIPID_NONE)
  {
    if (tNow >= laptopState.tClipWillEnd)
    {
      if (AUTO_START_NEXT_MOVEMENT)
      {
        // Auto-start the next movement.  (Same as NEXT key.)
        //
        StartPlaying(false);
      }
      else // Let it stop.  The NEXT key will pick up with the next clip.
      {
        // Set the current clip to none, but save the previous for the NEXT key
        //
        laptopState.prevClipID    = laptopState.currentClipID;
        laptopState.currentClipID = CLIPID_NONE;
      }
    }
  }

  // Generate tempo updates while playing.
  //
  if ((laptopState.currentClipID != CLIPID_NONE) && (key == KEYCODE_TEMPO))
  {
    if (laptopState.tLastTempoKey <= 0)
    {
      // First one since this clip started playing.
      //
      laptopState.tLastTempoKey = tNow;
    }
    else  // Calculate interval and add to the average
    {
      __int64 tInterval = tNow - laptopState.tLastTempoKey;
      laptopState.tLastTempoKey = tNow;

      if (AveragerAddNearbyValue(&(laptopState.avgTempo), (__int32)tInterval))
      {
        // The average was updated.  Calculate the new usecPerFrame and
        // send it out.
        //
        double ratio =   // ratio is more than one if real time is slower
          ((double) laptopState.avgTempo.average) / laptopState.usecPerBeat;

        double newUsecPerFrame = ((double) laptopState.usecPerFrame) * ratio;

        laptopState.currentFrameTempo =
          GetLaptopCmdTimeFromUsec((u_int32) (newUsecPerFrame + 0.5));

        // No need for a long wait to pace the delivery of these commands.
        // This event only happens once per keystroke.
        // Send two copies, though.
        //
        SendCommand(
          UDPCMD_TEMPO,
          laptopState.currentClipID,
          laptopState.currentFrameTempo
          );

        BusyWait(1000);  // 1 msec

        SendCommand(
          UDPCMD_TEMPO,
          laptopState.currentClipID,
          laptopState.currentFrameTempo
          );
      }
    }
  }

  // Basic idle loop
  //
  if (tNow >= laptopState.tNextIdleUdpDue)
  {
    if (laptopState.currentClipID != CLIPID_NONE)
    {
      if (laptopState.currentFrameTempo > 0)
      {
        SendCommand(
          UDPCMD_TEMPO,
          laptopState.currentClipID,
          laptopState.currentFrameTempo
          );
      }
      else  // Tempo not ready, just send a time synch update.
      {
        // Always get the current time immediately before sending SYNCH command.
        //
        SendCommand(UDPCMD_SYNCH, CLIPID_NONE, GetLaptopCmdTime());
      }

      // Repeat tempo or synch commands every two frames while running
      //
      laptopState.tNextIdleUdpDue  = laptopState.tLastUdp;
      laptopState.tNextIdleUdpDue += (__int64) (2 * laptopState.usecPerFrame);
    }
    else  // not running, always send a SYNCH
    {
      // Always get the current time immediately before sending SYNCH command.
      //
      SendCommand(UDPCMD_SYNCH, CLIPID_NONE, GetLaptopCmdTime());
      laptopState.tNextIdleUdpDue = laptopState.tLastUdp + IDLE_SYNCH_INTERVAL;
    }
  }
}
