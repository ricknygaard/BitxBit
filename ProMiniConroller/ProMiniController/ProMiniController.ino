#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include "utility/socket.h"
#include <ccspi.h>
#include <SPI.h>
/*
  SD card file dump
 
 This example shows how to read a file from the SD card using the
 SD library and send it over the serial port.
 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created  22 December 2010
 by Limor Fried
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	 
 */

#include <SD.h>
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\ArduinoIdle.h"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\Averager.h"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\BKProtocol.h"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\Common.h"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\GetTime.h"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\GetTime.cpp"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\Averager.cpp"
#include "C:\Users\Nlaptop\Desktop\bkLights\libraries\timing_library\ArduinoIdle.cpp"

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;

void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(115200);

 // Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  //Serial.println("card initialized.");

  ArduinoInit();
}

void loop()
{
  ArduinoIdleFunction();
}

