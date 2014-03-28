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

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
#define WLAN_SSID       "belkin.d5c"        // cannot be longer than 32 characters!
#define WLAN_PASS       "baeba66a"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//char ANIM_NUM = '0';

#define BUFSIZE 150

// Store other needed variables here…
int port;
int skt; 
long bind_var;
short sin_family;
unsigned short sin_port;
unsigned long s_addr;
char sin_zero[8];
uint32_t ip_holder; 
socklen_t sockLen;
unsigned char buf[BUFSIZE];
socklen_t* addrlen;
int sizeReturned;


// Keith was testing something here, not sure what, commenting out for now
//sockaddr_in SOCKET_ADDRESS_test;

//sockaddr_in SOCKET_ADDRESS, from;
sockaddr_in SOCKET_ADDRESS;
sockaddr_in remaddr;

// Testing constants
boolean wifiConnected = false;
boolean wifiRecieving = false;
boolean wifiSerial = false;

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
  // Declaration and organization of the socket's address information, to be used in the bind command later  

 

  // declared this another way, we may not need these two anymore 
  //in_addr sinaddr;
  //sinaddr.s_addr;

  ip_holder = 0;

  SOCKET_ADDRESS.sin_addr.s_addr = htonl(ip_holder);
  SOCKET_ADDRESS.sin_zero[8] = 0;

  // SOCKET_ADDRESS *addr;
    
  Serial.begin(9600);
  Serial.println('2');

  // Initialization of the CC3000 wifi chip, connection to the network, etc.
  cc3000.begin();
  cc3000.deleteProfiles();


  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    // Serial.println(F("Failed!"));
    while(1);
  }

  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  } 
  Serial.println('1');



  // Create the socket, setting the parameters for UDP listening
  while(!(skt = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP)) > 0 );
  memset(&SOCKET_ADDRESS, 0, sizeof(sockaddr_in));
  port = 8888;
  SOCKET_ADDRESS.sin_family = AF_INET;
  SOCKET_ADDRESS.sin_addr.s_addr = 0;
  SOCKET_ADDRESS.sin_port = htons(port);

   // trying it backwards because we were receiving a backwards IP on the serial output
   // in_addr sin_addr =  { cc3000.IP2U32((uint8_t)192, (uint8_t)168, (uint8_t)3, (uint8_t)0) };

  // declared the socket address another way so I don't think we need these next two anymore
  //Serial.println(SOCKET_ADDRESS.sin_addr.s_addr);
  //sockaddr_in SOCKET_ADDRESS = { AF_INET,htons(8888), 0 , 0 };
    
   // while( (bind_var = bind( skt, (sockaddr *)&SOCKET_ADDRESS , sizeof(SOCKET_ADDRESS))) == -1 ) { Serial.println("here"); };
  bind_var = bind( skt, (sockaddr*)&SOCKET_ADDRESS, sizeof(sockaddr_in));
  Serial.println(bind_var);

  Serial.print('3');

  // a check to verify that the socket is created successfully
  // Serial.println(skt);
  // Serial.println("while loop 1 passed");  

  socklen_t addrlen = sizeof(remaddr);
  sockLen = sizeof(sockaddr_in);
  delay(200);

  Serial.println('0');
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void ArduinoIdleFunction()
{
  u_int32 *pTime;
  u_int32 delta;
  u_int32 tNow;


  int recvlen = recvfrom( skt, udpPayload, UDP_PAYLOAD_SIZE, 0, (sockaddr*)&remaddr, addrlen);
  // int recvlen = recvfrom(skt, udpPayload, UDP_PAYLOAD_SIZE, 0, (sockaddr *)&SOCK_ADDER_test, &SOCK_LENGTH);
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
