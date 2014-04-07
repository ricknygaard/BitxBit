#include <Arduino.h>

#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>

#include <utility/socket.h>

#include "Averager.h"
#include "BKProtocol.h"
#include "GetTime.h"
#include <SimpleFile.h>
#include "ArduinoIdle.h"

extern int __bss_end;

#define USE_WIFI
#define USE_SD
// #define USE_PRINTS

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
//
const int FILE_NAME_SIZE  = 6;    // "NN.dat"
const int FILE_BLOCK_SIZE = 192;
const int FILE_NUM_PIXELS = FILE_BLOCK_SIZE / 3;
const int FILE_HEADER_NUM_FRAMES     = 3;
const int FILE_HEADER_USEC_PER_FRAME = 4;

const int NUM_LED_ROWS = 8;

const u_int8 FRAME_START = 2;
const u_int8 FRAME_DONE  = 1;

#define DELAY_USEC(a)  delayMicroseconds(a)
const int SER_DELAY = 350;  // microseconds

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
static    bool   useLaptopTime;
static    bool   transferBlackFrame; // Flag to request LEDs go dark
static    bool   transferColorFrame; // Flag to request LED color fill
static  u_int8   colorB;
static  u_int8   colorG;
static  u_int8   colorR;
static  u_int8   numRowsInFillerFrame;
static  u_int8   numRowsCounter;

#ifdef USE_SD
static SimpleFile dataFile;      // For the open clip movement data file
#endif

#ifdef USE_WIFI
// CC3000 variables
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

Adafruit_CC3000 cc3000 = Adafruit_CC3000(
  ADAFRUIT_CC3000_CS,
  ADAFRUIT_CC3000_IRQ,
  ADAFRUIT_CC3000_VBAT,
  SPI_CLOCK_DIVIDER
  ); // you can change this clock speed but DI

#define WLAN_SSID       "Skyhook"  // cannot be longer than 32 characters!
#define WLAN_PASS       "CelestDSN:!+++++"
#define WLAN_SECURITY   WLAN_SEC_WPA

/*
#define WLAN_SSID       "belkin.d5c"  // cannot be longer than 32 characters!
#define WLAN_PASS       "baeba66a"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
*/

int fdSocket; 
#endif

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
#ifdef USE_PRINTS
  Serial.print(F("Clip "));
  Serial.print(clipId);
  Serial.println(F(" already playing"));
#endif
      return;
    }
    else  // Close the other one.  Time to start this one.
    {
      dataFile.Close();
#ifdef USE_PRINTS
      Serial.print(F("Stopping clip "));
      Serial.print(playingClipID);
      Serial.print(F(" to play clip "));
      Serial.println(clipId);
#endif
    }
  }

  // Open the requested file.
  //
  int upper = clipId / 10; // Do these together to give the compiler a chance
  int lower = clipId % 10; // to notice that we want both results from division.
  fileName[0] = '0' + upper;
  fileName[1] = '0' + lower;

#ifdef USE_PRINTS
  Serial.print(F("Open "));
  Serial.print(clipId);
  Serial.print(F(" "));
  Serial.println(fileName);
#endif

#ifdef USE_SD

  if (dataFile.Open(fileName))
  {
    if (dataFile.ReadBuf(blockBuf, FILE_BLOCK_SIZE) == FILE_BLOCK_SIZE)
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
      tFrameInterval     = 50000 >> 2; // pIntegers[FILE_HEADER_USEC_PER_FRAME] >> 2;
      numFramesRemaining = pIntegers[FILE_HEADER_NUM_FRAMES];
      playingClipID      = (ClipID) clipId;
      useLaptopTime      = AverageIsValid(&timeDiff);
      tNextFrameDue      = useLaptopTime ? tBegin : GetArduinoTime();

#ifdef USE_PRINTS
      Serial.print(F("tFrame "));
      Serial.print(tFrameInterval);
      Serial.print(F("  numFrames "));
      Serial.print(numFramesRemaining);
      Serial.print(F("  clipID "));
      Serial.println(playingClipID);
#endif
    }
    else  // Not even the first block was in the file
    {
      dataFile.Close();
      playingClipID = CLIPID_NONE;
    }
  }
#endif

  if (playingClipID == CLIPID_NONE)
  {
#ifdef USE_PRINTS
    Serial.println(F("Clip was not opened"));
#endif
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
#ifdef USE_SD
  if (retval)
  {
    dataFile.Close();
  }
#endif

  playingClipID      = CLIPID_NONE;
  numFramesRemaining = 0;
  tFrameInterval     = 0;
  tNextFrameDue      = 0;
  transferBlackFrame = false;

  return retval;
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
/*
static bool DisplayConnectionDetails()
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
#ifdef USE_PRINTS
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
#endif
    return false;
  }
  else
  {
#ifdef USE_PRINTS
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
#endif
    return true;
  }
}
*/

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static void SendFillerFrame(int nRows, long color)
{
  // Safety check the values -- cannot match the start code
  //
  u_int8 r = color >> 16;
  u_int8 g = color >> 8;
  u_int8 b = color;
  if (r == FRAME_START) { ++r; }
  if (g == FRAME_START) { ++g; }
  if (b == FRAME_START) { ++b; }

  if (nRows > NUM_LED_ROWS)
  {
    nRows = NUM_LED_ROWS;
  }

#ifdef USE_PRINTS
  Serial.print(F("Fill "));
  Serial.print(nRows);
  Serial.print(F(" "));
  Serial.print(r);
  Serial.print(F(" "));
  Serial.print(g);
  Serial.print(F(" "));
  Serial.println(b);
  delay(20);

#else

  // Send begin timing byte
  //
  DELAY_USEC(SER_DELAY);  Serial.write(FRAME_START);

  int row;
  for (row = 0; row < nRows; ++row)
  {
    for (int col = 0; col < NUM_LED_ROWS; ++col)
    {
      DELAY_USEC(SER_DELAY);  Serial.write(b);
      DELAY_USEC(SER_DELAY);  Serial.write(g);
      DELAY_USEC(SER_DELAY);  Serial.write(r);
    }
  }

  for ( ; row < NUM_LED_ROWS; ++row)
  {
    // Each row has 8 LEDs, each of which gets 3 color bytes = 24 bytes
    //
    const int numValues = NUM_LED_ROWS * 3;
    for (int i = 0; i < numValues; ++i)
    {
      DELAY_USEC(SER_DELAY);  Serial.write(0);
    }
  }

  // Wait for acknowledgement of frame.
  //
  DELAY_USEC(SER_DELAY * 4);
  for (int readVal = -1; readVal != FRAME_DONE; readVal = Serial.read())
  {
    // read() returns -1 if no data available
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE
static bool StartCC3000(int attemptCounter)
{
  SendFillerFrame(attemptCounter, 0xff0000L);

  /*
  switch (attemptCounter)
  {
    case 0:  break;
    case 1:  cc3000.disconnect();  break;
    case 2:  cc3000.disconnect();  cc3000.reboot();  break;

    default:
      cc3000.disconnect();
      cc3000.reboot();
      cc3000 = Adafruit_CC3000(
        ADAFRUIT_CC3000_CS,
        ADAFRUIT_CC3000_IRQ,
        ADAFRUIT_CC3000_VBAT,
        SPI_CLOCK_DIVIDER
        );
      break;
  }
  */

  // Initialization of the CC3000 wifi chip, connection to the network, etc.
  //
  if (! cc3000.begin())
  {
    return false;
  }

  SendFillerFrame(attemptCounter + 1, 0xff0000L);

  const int MAX_TRIES = 6;
  int numTries;
  for (numTries = 1; numTries <= MAX_TRIES; ++numTries)
  {
    if (cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
    {
      break;
    }

    delay(1000);
    SendFillerFrame(numTries, 0x00ff00L);
  }

  if (numTries > MAX_TRIES)
  {
    return false;
  }

  SendFillerFrame(numTries + 1, 0x00ff00L);

  for (numTries = 1; numTries <= MAX_TRIES * attemptCounter; ++numTries)
  {
    if (cc3000.checkDHCP())
    {
      break;
    }

    // Getting the DHCP address can take quite a while
    //
    delay(5000);
    SendFillerFrame(numTries, 0x0000ffL);
  }

  if (numTries > MAX_TRIES)
  {
    return false;
  }

  SendFillerFrame(NUM_LED_ROWS - 1, 0x0000ffL);
  
  fdSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

#ifdef USE_PRINTS
  Serial.print("fdSocket = ");
  Serial.println(fdSocket);
#endif
    
  // Bind to the socket
  //
  sockaddr_in socketAddress;
    
  memset(&socketAddress, 0, sizeof(sockaddr_in));
  socketAddress.sin_family      = AF_INET;
  socketAddress.sin_addr.s_addr = 0;  // INADDR_ANY = 0
  socketAddress.sin_port        = htons(UDP_CMD_PORT);
    
  return bind(fdSocket, (sockaddr*)&socketAddress, sizeof(sockaddr_in)) >= 0;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void ArduinoInit()
{
  // Open serial communications and wait for port to open
  //
  Serial.begin(57600);  // 115200);

  AveragerReset(&timeDiff);

  playingClipID        = CLIPID_NONE;
  numFramesRemaining   = 0;
  tFrameInterval       = 0;
  tNextFrameDue        = 0;
  transferBlackFrame   = false;
  transferColorFrame   = false;
  numRowsInFillerFrame = 0;
  numRowsCounter       = 0;

  // Fill in the constant portion of the file name
  // "NN.dat"
  //
  fileName[2] = '.';
  fileName[3] = 'd';
  fileName[4] = 'a';
  fileName[5] = 't';
  fileName[6] = '\0';

#ifdef USE_PRINTS
  int free_memory = ((int)&free_memory) - ((int)&__bss_end);
  Serial.print(F("freeRAM = "));
  Serial.println(free_memory);
#endif

  // On the Ethernet Shield, CS is pin 4. Note that even if it's not
  // used as the CS pin, the hardware CS pin (10 on most Arduino boards,
  // 53 on the Mega) must be left as an output or the SD library
  // functions will not work.
  //
  const int chipSelectSD     = 4;
  const int chipSelectCC3000 = 10;

  // Serial.print(F("Initializing SD card..."));
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelectSD, OUTPUT);
  pinMode(chipSelectCC3000, OUTPUT);

  pinMode(13, OUTPUT);
  pinMode(12, INPUT);
  pinMode(11, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, INPUT);

  SendFillerFrame(1, 0xffff00L);
  
#ifdef USE_SD
  // See if the SD card is present and can be initialized:
  //
  while (! dataFile.Init(chipSelectSD))
  {
    Serial.println(F("Card failed, or not present"));
  }
#endif

// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
#ifdef USE_WIFI
  int attemptCounter = 0;
  while (! StartCC3000(++attemptCounter))  // Start at 1
  {
    /*
    SendFillerFrame(3, 0x808040L);
    delay(500);
    SendFillerFrame(2, 0x808040L);
    delay(500);
    SendFillerFrame(1, 0x808040L);
    */
    delay(500);
  }
#endif

  SendFillerFrame(NUM_LED_ROWS - 2, 0xffffffL);
  delay(100);

  SendFillerFrame(NUM_LED_ROWS, 0xffffffL);
  delay(500);

  // We're up.  Set the LEDs to black.
  //
  SendFillerFrame(NUM_LED_ROWS, 0x000000L);
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
void ArduinoIdleFunction()
{
  UdpCmd udpCmd = UDPCMD_NONE;

#ifdef USE_WIFI
  if (numRowsCounter == 0)
  {
    // Play the panel ID image the first time through
    //
    udpCmd = UDPCMD_START;
    udpPayload[UDP_PAYLOAD_ID_OFFSET] = 0;
    numRowsCounter = 100;
  }
  else  // Look for UDP command
  {
    // Is anything available from UDP?  This select call returns after about
    // 5 msec and indicates whether a read on the socket would block or not.
    //
    fd_set fd_read;  // The set of file descriptors to check for read-ability.
    memset(&fd_read, 0, sizeof(fd_read));  // Set all to "do not check".
    FD_SET(fdSocket, &fd_read);            // Set the one we care about.

    timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0; // Ask for 0, the driver sets it to 5 msec anyway

    int numReady = select(fdSocket + 1, &fd_read, NULL, NULL, &timeout);
    if (numReady > 0)
    {
      // Read the command packet
      //
      sockaddr_in srcAdrs;
      socklen_t   sockAddrLen = sizeof(sockaddr_in);

      if (recvfrom(
        fdSocket,
        udpPayload,
        UDP_PAYLOAD_SIZE,
        0,
        (sockaddr*) &srcAdrs,
        &sockAddrLen
        ) == UDP_PAYLOAD_SIZE)
      {
        udpCmd = (UdpCmd) udpPayload[UDP_PAYLOAD_CMD_OFFSET];
#ifdef USE_PRINTS
        Serial.print(F("udpCmd = "));
        Serial.println(udpCmd);
#endif
      }
    }
#ifdef USE_PRINTS
    // Look for reset command:  two 'Q' characters in a row on serial in.
    //
    if (Serial.read() == 'Q')
    {
      delay(20);
      if (Serial.read() == 'Q')
      {
        Serial.println(F("RESET"));
        cc3000.disconnect();

        // Soft restart
        //
        asm volatile ("  jmp 0");
      }
    }
#endif
#else
    /*
    udpCmd = UDPCMD_FILL;
    udpPayload[UDP_PAYLOAD_TIME_OFFSET + 0] = 0x80;  // blue
    udpPayload[UDP_PAYLOAD_TIME_OFFSET + 1] = 0x80;  // green
    udpPayload[UDP_PAYLOAD_TIME_OFFSET + 2] = numRowsCounter;  // red
    udpPayload[UDP_PAYLOAD_TIME_OFFSET + 3] = (numRowsCounter % NUM_LED_ROWS)+1;
    ++numRowsCounter;
    */
    if (playingClipID == CLIPID_NONE)
    {
      udpCmd = UDPCMD_START;
      udpPayload[UDP_PAYLOAD_ID_OFFSET] = numRowsCounter++;
    }
#endif
  }

  u_int32 *pTime;
  u_int32  delta;
  u_int32  tNow;

  if (udpCmd != UDPCMD_NONE)
  {
#ifdef USE_PRINTS
    /*
    Serial.print(udpPayload[0]);
    Serial.print(F(" "));
    Serial.print(udpPayload[1]);
    Serial.print(F(" "));
    Serial.print(udpPayload[2]);
    Serial.print(F(" "));
    Serial.print(udpPayload[3]);
    Serial.print(F(" "));
    Serial.print(udpPayload[4]);
    Serial.print(F(" "));
    Serial.println(udpPayload[5]);
    return;
    */
#endif

    pTime = (u_int32*) &(udpPayload[UDP_PAYLOAD_TIME_OFFSET]);

    switch (udpCmd)
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

      case UDPCMD_FILL:
        // Only perform the color fill command if no clip is playing.
        //
        if (playingClipID == CLIPID_NONE)
        {
          transferColorFrame = true;

          // The time value is co-opted for the color info.
          //
          colorB  = udpPayload[UDP_PAYLOAD_TIME_OFFSET + 0];
          colorG  = udpPayload[UDP_PAYLOAD_TIME_OFFSET + 1];
          colorR  = udpPayload[UDP_PAYLOAD_TIME_OFFSET + 2];
          numRowsInFillerFrame = udpPayload[UDP_PAYLOAD_TIME_OFFSET + 3];
        }
        break;

      case UDPCMD_NONE:
        break;
    }
  }

  // If playing, wait until the next frame time, then transfer it.
  //
  if (playingClipID != CLIPID_NONE)
  {
    // Busy wait for the next frame's time of arrival.
    //
    do
    {
      if (useLaptopTime)
      {
        // Calculate the current reconstructed laptop time.
        //
        // tLaptopReconstructed = tLocal + timeDiff
        //
        tNow = GetArduinoTime() + timeDiff.average;
      }
      else
      {
        // We're not using laptop time. The due time is in terms of the Arduino
        // clock and does not need any adjustment.
        //
        tNow = GetArduinoTime();
      }
    } while (tNow < tNextFrameDue);

#ifndef USE_PRINTS
    // Send begin timing byte
    //
    DELAY_USEC(SER_DELAY);  Serial.write(FRAME_START);

    byte frame_b;
    for (int i = 0; i < FILE_BLOCK_SIZE; ++i) 
    {  
      frame_b = dataFile.ReadByte();

      // Safety check -- Cannot match the start code
      //
      if (frame_b == FRAME_START)
      {
        ++frame_b;
      }

      DELAY_USEC(SER_DELAY);  Serial.write(frame_b);
    }

    // Wait for acknowledgement of frame.
    //
    DELAY_USEC(SER_DELAY * 4);
    for (int readVal = -1; readVal != FRAME_DONE; readVal = Serial.read())
    {
      // read() returns -1 if no data available
    }
#endif

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

#ifdef USE_PRINTS
    Serial.print(F("Frames remaining "));
    Serial.println(numFramesRemaining);
#endif
  }

  if (transferBlackFrame)
  {
    transferBlackFrame = false;
    SendFillerFrame(NUM_LED_ROWS, 0x000000L);
  }

  if (transferColorFrame)
  {
    transferColorFrame = false;
    long color = colorR;
    color <<= 8;  color += colorG;
    color <<= 8;  color += colorB;
    SendFillerFrame(numRowsInFillerFrame, color);
  }
}
