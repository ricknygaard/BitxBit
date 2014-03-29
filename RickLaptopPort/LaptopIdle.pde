import hypermedia.net.*;

static final String movementFilePath = "testMovements/Movements.txt";
static final long IDLE_SYNCH_INTERVAL = 100000;
static final boolean AUTO_START_NEXT_MOVEMENT = false;

static final char KEYCODE_BEGIN = 'b';
static final char KEYCODE_NEXT  = 'n';
static final char KEYCODE_TEMPO = 't';
static final char KEYCODE_STOP  = 'S';
static final char KEYCODE_RESET = 'R';

class LaptopStateT
{
  private AveragerT     avgTempo;
  private LaptopTimeT   laptopTime;
  private MovementListT movementList;
  private UDP           udp;
  private byte          payloadBuf[];
  
  private int  prevClipID;
  private int  currentClipID;
  private long tLastUdp;
  private long tLastTempoKey;
  private long tClipStartUsec;
  private long tClipWillEnd;
  private long tNextIdleUdpDue;
  private long usecPerFrame;
  private long currentFrameTempo;
  private double usecPerBeat;
  
  
  public LaptopStateT(UDP director)
  {
    udp = director;
    Init();
  }
  
  private void Init()
  {
    avgTempo = new AveragerT();
    avgTempo.Reset();
    
    laptopTime = new LaptopTimeT();
    laptopTime.Reset();
    
    movementList = new MovementListT();
    movementList.ReadMovementFile(movementFilePath);

    payloadBuf = new byte[UDP_PAYLOAD_SIZE];
   // udp = new UDP();
    
    prevClipID      = CLIPID_NONE;
    currentClipID   = CLIPID_NONE;
    tLastUdp        = laptopTime.GetUsec();
    tNextIdleUdpDue = tLastUdp + IDLE_SYNCH_INTERVAL;
    tLastTempoKey   = 0;
  }

  private int BusyWait(long tWaitUsec)
  {
    int x = 0;  // Using this so the interpreter doesn't "optimize" the wait loop out.
    while ((laptopTime.GetUsec() - tLastUdp) < tWaitUsec)
    {
      ++x;  // Spin, spin.  Makework to be sure the loop doesn't go away.
    }
    
    return x;  // And return the result so it looks like it's worth producing.
  }
  
  private void SendCommand(int udpCmd, int clipID, long tValue)
  {
    payloadBuf[UDP_PAYLOAD_CMD_OFFSET] = (byte) udpCmd;
    payloadBuf[UDP_PAYLOAD_ID_OFFSET]  = (byte) clipID;
    
    for (int i = 0; i < 4; ++i)
    {
      payloadBuf[UDP_PAYLOAD_TIME_OFFSET + i] = (byte) (tValue & 0xff);
      tValue >>= 8;
    }
    
    udp.send(payloadBuf);
    
   // println("sent " + (char) ('0' + payloadBuf[UDP_PAYLOAD_CMD_OFFSET]));
  }
  
  private void StartPlaying(boolean bStartFromTheBeginning)
  {
    if (bStartFromTheBeginning)
    {
      currentClipID = movementList.GetFirstClipIDInTheShow();
    }
    else  // Get the next one.  Get none if at the end.  Do not start over.
    {
      if (currentClipID > CLIPID_NONE)
      {
        prevClipID    = currentClipID;
        currentClipID = movementList.GetNextClipID(currentClipID);
      }
      else if (prevClipID > CLIPID_NONE)
      {
        // nothing running right now, use what was running
        //
        currentClipID = movementList.GetNextClipID(prevClipID);
      }
    }
    
    // If none to play, reset the previous, too, and quit.
    //
    if (currentClipID == CLIPID_NONE)
    {
      prevClipID = CLIPID_NONE;
      return;
    }
  
    // Go!
    //
    // Send 20 commands over about 0.1 seconds.
    //
    // Calculate the desired start time (about midway through the repetitions).
    //   0.5 * 0.1 seconds = 50000 usec
    //
    tClipStartUsec = laptopTime.GetUsec() + 50000;  // 50 msec
    long tStartCmd = laptopTime.GetCmdTimeFromUsec(tClipStartUsec);
  
    for (int i = 0; i < 20; ++i)
    {
      SendCommand(UDPCMD_START, currentClipID, tStartCmd);
      BusyWait(5000);  // 0.1 seconds / 20 repetitions = 5 msec
    }
  
    MovementInfoT currentMovement =
      movementList.GetMovementInfoForClipID(currentClipID);
      
    usecPerFrame = currentMovement.GetUsecPerFrame();
    usecPerBeat  = currentMovement.GetUsecPerBeat();
    currentFrameTempo = 0;  // Will go nonzero when updated.
  
    tClipWillEnd = tClipStartUsec + currentMovement.GetUsecTotal();
  
    tLastTempoKey = 0;
    avgTempo.Reset();
  
    // Repeat tempo or synch commands every two frames while running
    //
    tNextIdleUdpDue  = tLastUdp + (2 * usecPerFrame);
  }
  
  private void StopPlaying()
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
      SendCommand(UDPCMD_STOP, currentClipID, laptopTime.GetCmdTime());
      BusyWait(usecPerFrame);  // Typically 33 or 50 msec
    }
  
    prevClipID    = currentClipID;
    currentClipID = CLIPID_NONE;
  
    // Drop back to the non-playing idle message rate.
    //
    tNextIdleUdpDue = tLastUdp + IDLE_SYNCH_INTERVAL;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // PUBLIC
  void LoopFunction(char key)
  {
    switch (key)
    {
      case KEYCODE_BEGIN: StartPlaying(true);   break;
      case KEYCODE_NEXT:  StartPlaying(false);  break;
      case KEYCODE_STOP:  StopPlaying();        break;
  
      case KEYCODE_RESET:
        // Only do a reset when nothing is running.
        //
        if (currentClipID == CLIPID_NONE)
        {
          Init();
        }
        break;
    }
  
    long tNow = laptopTime.GetUsec();
  
    // Is a movement clip running?  Is it time to advance to the next one?
    //
    if (currentClipID != CLIPID_NONE)
    {
      if (tNow >= tClipWillEnd)
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
          prevClipID    = currentClipID;
          currentClipID = CLIPID_NONE;
        }
      }
    }
  
    // Generate tempo updates while playing.
    //
    if ((currentClipID != CLIPID_NONE) && (key == KEYCODE_TEMPO))
    {
      if (tLastTempoKey <= 0)
      {
        // First one since this clip started playing.
        //
        tLastTempoKey = tNow;
      }
      else  // Calculate interval and add to the average
      {
        long tInterval = tNow - tLastTempoKey;
        tLastTempoKey = tNow;
  
        if (avgTempo.AddNearbyValue(tInterval))
        {
          // The average was updated.  Calculate the new usecPerFrame and
          // send it out.
          //
          double ratio =   // ratio is more than one if real time is slower
            ((double) avgTempo.GetAverage()) / usecPerBeat;
  
          double newUsecPerFrame = ((double) usecPerFrame) * ratio;
  
          currentFrameTempo =
            laptopTime.GetCmdTimeFromUsec((long) (newUsecPerFrame + 0.5));
  
          // No need for a long wait to pace the delivery of these commands.
          // This event only happens once per keystroke.
          // Send two copies, though.
          //
          SendCommand(UDPCMD_TEMPO, currentClipID, currentFrameTempo);
  
          BusyWait(1000);  // 1 msec
  
          SendCommand(UDPCMD_TEMPO, currentClipID, currentFrameTempo);
        }
      }
    }
  
    // Basic idle loop
    //
    if (tNow >= tNextIdleUdpDue)
    {
      if (currentClipID != CLIPID_NONE)
      {
        if (currentFrameTempo > 0)
        {
          SendCommand(UDPCMD_TEMPO, currentClipID, currentFrameTempo);
        }
        else  // Tempo not ready, just send a time synch update.
        {
          // Always get the current time immediately before sending SYNCH command.
          //
          SendCommand(UDPCMD_SYNCH, CLIPID_NONE, laptopTime.GetCmdTime());
        }
  
        // Repeat tempo or synch commands every two frames while running
        //
        tNextIdleUdpDue = tLastUdp + (2 * usecPerFrame);
      }
      else  // not running, send a SYNCH
      {
        // Always get the current time immediately before sending SYNCH command.
        //
        SendCommand(UDPCMD_SYNCH, CLIPID_NONE, laptopTime.GetCmdTime());
        tNextIdleUdpDue = tLastUdp + IDLE_SYNCH_INTERVAL;
      }
    }
  }
}
  

