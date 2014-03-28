#include <stdafx.h>

#include <io.h>
#include <fcntl.h>

#include "Averager.h"
#include "BKProtocol.h"
#include "GetTime.h"
#include "ArduinoIdle.h"

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
//
const int FILE_NAME_SIZE  = 6;    // "NN.dat"
const int FILE_BLOCK_SIZE = 192;
const int FILE_HEADER_NUM_FRAMES     = 3;
const int FILE_HEADER_USEC_PER_FRAME = 4;

// Buffers
//
static u_int8 blockBuf[FILE_BLOCK_SIZE];
static u_int8 udpPayload[UDP_PAYLOAD_SIZE];
static   char fileName[FILE_NAME_SIZE + 1];

// Status info and flags
//
static AveragerT timeDiff;        // tLaptopReconstructed = tLocal + timeDiff
static ClipID    playingClipID;   // set to CLIPID_NONE when not playing frames
static u_int32   numFramesRemaining;
static u_int32   tFrameInterval;  // Units of 4 usec: tUsec = tFrameInterval * 4
static u_int32   tNextFrameDue;   // Units of 4 usec: tUsec = tFrameInterval * 4
static    bool   transferBlackFrame; // Flag to request LEDs go dark

// HACK for making this work on a PC
//
static File dataFile;     // For the open clip movement data file

typedef struct
{
  u_int8 payload[UDP_PAYLOAD_SIZE];
} UdpPacketT;

UdpPacketT packet;
UdpPacketT *pPacket;

// CC3000 variables
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "belkin.d5c"        // cannot be longer than 32 characters!
#define WLAN_PASS       "baeba66a"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//char ANIM_NUM = '0';

#define BUFSIZE 150

// Store other needed variables here…
int port = 8888;
int skt; 
long bind_var;
short sin_family;
unsigned short sin_port;
unsigned long s_addr;
char sin_zero[8];
uint32_t ip_holder; 

sockaddr_in SOCK_ADDER;
sockaddr_in SOCK_ADDER_test;

socklen_t SOCK_LENGTH;

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
// PUBLIC HACK 
void ArduinoPushUdpPacket(u_int8 *buf)
{
  pPacket = &packet;

  for (int i = 0; i < UDP_PAYLOAD_SIZE; ++i)
  {
    pPacket->payload[i] = buf[i];
  }
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE  HACK 
static bool OpenDataFile(char *fileName)
{ 
  File dataFile = SD.open("datalog.txt");
  return (bool) dataFile;
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE  HACK 
static void CloseDataFile()
{
  dataFile.close();
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE  HACK 
static bool GetNextBlock()
{
  // KLUDGE This is how the host does it.  Replace with Arduino code.
  // Assumed that this is only called with a valid dataFile present.
  //
  for(int i = 0; i < FILE_BLOCK_SIZE; ++i)
  {
    while (!dataFile.available());
    blockBuf[i] = dataFile.read();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static void OpenClip(int clipId, u_int32 tBegin)
{
  // Is there something open now?
  //
  if (playingClipID != CLIPID_NONE)
  {
    if (playingClipID == (ClipID) clipId)
    {
      // Already got the news and started this clip.
      //
      return;
    }
    else  // Close the other one.  Time to start this one.
    {
      CloseDataFile();
    }
  }

  // Open the next file, if possible.
  //
  int upper = clipId / 10; // Do these together to give the compiler a chance
  int lower = clipId % 10; // to notice that we want both results from division.
  fileName[0] = '0' + upper;
  fileName[1] = '0' + lower;

  if (OpenDataFile(fileName))
  {
    if (GetNextBlock())
    {
      // It's open and we got the header.  Let's go.  Read info from the header.
      // Int[3] = Num image frames. One or more. (Does not include header.)
      // Int[4] = Microseconds per frame default playback rate.
      // Int[5] = Number of repetitions for the set of frames (0 = "forever").
      // Int[6] = Number of frames to skip at the beginning when looping back.
      // Int[7] = What to do when the last frame is played
      //
      u_int32 *pIntegers = (u_int32*) &(blockBuf[0]);

      // Convert the given usecPerFrame to units of 4 usec
      //
      tFrameInterval     = pIntegers[FILE_HEADER_USEC_PER_FRAME] >> 2;
      numFramesRemaining = pIntegers[FILE_HEADER_NUM_FRAMES];
      tNextFrameDue      = tBegin;
      playingClipID      = (ClipID) clipId;
    }
    else  // Not even the first block was in the file
    {
      CloseDataFile();
      playingClipID = CLIPID_NONE;
    }
  }

  if (playingClipID == CLIPID_NONE)
  {
    numFramesRemaining = 0;
    tFrameInterval     = 0;
    tNextFrameDue      = 0;
    transferBlackFrame = true;  // Make the LEDs dark
  }
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static bool CloseClip()
{
  bool retval = (playingClipID != CLIPID_NONE);

  if (retval)
  {
    CloseDataFile();
  }

  playingClipID      = CLIPID_NONE;
  numFramesRemaining = 0;
  tFrameInterval     = 0;
  tNextFrameDue      = 0;
  transferBlackFrame = false;

  return retval;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void ArduinoInit()
{
  AveragerReset(&timeDiff);

  playingClipID      = CLIPID_NONE;
  numFramesRemaining = 0;
  tFrameInterval     = 0;
  tNextFrameDue      = 0;
  transferBlackFrame = false;

  // Fill in the constant portion of the file name
  // "NN.dat"
  //
  fileName[2] = '.';
  fileName[3] = 'd';
  fileName[4] = 'a';
  fileName[5] = 't';
  fileName[6] = '\0';

  //CC3000 initialization
  sockaddr_in SOCK_ADDER;
  memset(sin_zero,0,sizeof(sin_zero));

  in_addr sinaddr;
  sinaddr.s_addr;

  SOCK_ADDER.sin_family = AF_INET;
  SOCK_ADDER.sin_port = htons(port);

  ip_holder = 0;

  SOCK_ADDER.sin_addr.s_addr = htonl(ip_holder);
  SOCK_ADDER.sin_zero[8] = 0;

    
    
  // SOCK_ADDER *addr;
    
  Serial.begin(9600);

  bool ip_connected = false;
  while(!ip_connected) {
    cc3000.begin();
    cc3000.deleteProfiles();
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);

    while (!cc3000.checkDHCP())
    {
      delay(100);
    } 
    while(!(skt = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP)) > 0 );

    uint32_t *ipAddress, *netmask, *gateway, *dhcpserv, *dnsserv;
    if(cc3000.getIPAddress(ipAddress, netmask, gateway, dhcpserv, dnsserv)) {
      Serial.println(*ipAddress);
      ip_connected = true;
    }
    else
    {
      Serial.println("couln't find an IP address");
      ip_connected = false;
    }
   }
    
  while( (bind_var = bind( skt, (sockaddr *)&SOCK_ADDER , sizeof(SOCK_ADDER))) == -1 ) { Serial.println("here"); };
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void ArduinoIdleFunction()
{
  u_int32 *pTime;
  u_int32 delta;
  u_int32 tNow;

  int recvlen = recvfrom(skt, udpPayload, UDP_PAYLOAD_SIZE, 0, (sockaddr *)&SOCK_ADDER_test, &SOCK_LENGTH);
  if (recvlen == UDP_PAYLOAD_SIZE)
  {
    // Newly arrived UDP packet
    //
    pTime = (u_int32*) &(udpPayload[UDP_PAYLOAD_TIME_OFFSET]);
    switch (udpPayload[UDP_PAYLOAD_CMD_OFFSET])
    {
      case UDPCMD_START:
        OpenClip((int) (udpPayload[UDP_PAYLOAD_ID_OFFSET]), *pTime);
        break;

      case UDPCMD_STOP:
        transferBlackFrame = CloseClip();
        //
        // FALL-THROUGH to use the time info for laptop synch
        //
      case UDPCMD_SYNCH:
        // tLaptopReconstructed          = tLocal + timeDiff
        // tLaptopReconstructed - tLocal = timeDiff
        //
        AveragerAddNearbyValue(&timeDiff, *pTime - GetArduinoTime());
        break;

      case UDPCMD_TEMPO:
        // An update to the frame interval. Only process if the command matches
        // the currently running clip.
        //
        if (playingClipID == (ClipID) (udpPayload[UDP_PAYLOAD_ID_OFFSET]))
        {
          delta = tFrameInterval - *pTime;
          tNextFrameDue  -= delta;
          tFrameInterval -= delta;
        }
        break;
    }
  }

  // If playing, wait until the next frame time, then transfer it.
  //
  if (playingClipID != CLIPID_NONE)
  {
    // Get the next block from the data file before waiting so the time spent
    // getting the data is part of the wait for the time the next frame is due
    // to be transferred.
    //
    GetNextBlock();

    // Busy wait
    //
    do
    {
      if (AverageIsValid(&timeDiff))
      {
        // Calculate the current reconstructed laptop time.
        //
        // tLaptopReconstructed = tLocal + timeDiff
        //
        tNow = GetArduinoTime() + timeDiff.average;
      }
      else
      {
        // Since we have no idea of laptop time yet, just play the frames right
        // away by making it look like it's time right now.
        //
        tNow = tNextFrameDue;
      }
    } while (tNow < tNextFrameDue);

    // send begin timing byte
    Serial.write(2);
    
    for(int i = 0; i < FILE_BLOCK_SIZE; ++i) 
    {  
      while(!dataFile.available());
      byte frame_b = dataFile.read();
      if (frame_b == 1 || frame_b == 2) 
      {
        frame_b = 3;
      }
      Serial.write(frame_b);
    }

    // Decrement frame count and close up if done.
    //
    if (--numFramesRemaining == 0)
    {
      CloseClip();
    }
    else
    {
      tNextFrameDue += tFrameInterval;
    }
  }

  if (transferBlackFrame)
  {
    transferBlackFrame = false;

    for(int i = 0; i < FILE_BLOCK_SIZE; ++i) 
    {  
      Serial.write(0);
    }
  }
}
