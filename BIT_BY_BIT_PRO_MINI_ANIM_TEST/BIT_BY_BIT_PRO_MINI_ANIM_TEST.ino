//#include <ArduinoIdle.h>
//#include <Averager.h>
//#include <BKProtocol.h>
//#include <Common.h>
//#include <GetTime.h>
//#include <LaptopIdle.h>
//#include <MovementInfo.h>

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



byte start_byte;
byte end_byte;
int i;
int j;
int num_of_frames[4];


// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;

void setup()
{
 // ArduinoInit();
 // Open serial communications and wait for port to open:
  Serial.begin(115200);
   /*while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
*/

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
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.

  
}

void loop()
{
  //ArduinoIdleFunction();
  end_byte = B00000000;
  
  /*
  start_byte = Serial.read();
  if(start_byte == B01010101)
  {
    
    */
    
    File dataFile = SD.open("09.DAT");
    
    dataFile.seek(12);
    for( i=0; i < 4; i++)
    {
    while(dataFile.available()==0);  
    num_of_frames[i] = dataFile.read(); 
    }
    
    int num_frames = (num_of_frames[3] << 24)+(num_of_frames[2] << 16)+(num_of_frames[1] << 8)+(num_of_frames[0]);
    
     // Navigate to the Animation information, skip the buffer
     
    for( int j = 0; j < num_frames; j++)
    {  
     
       dataFile.seek(192 * j + 192);
     //dataFile.seek(192);
    
    //num_of_frames++;
  

      // send begin timing byte
      Serial.write('2');
      
      
      for(i = 0; i < 192; ++i) 
      {  
        while(dataFile.available()==0);
        byte frame_b = dataFile.read();
        if (frame_b == 1 || frame_b == 2) 
        {
          frame_b = 3;
        }
        Serial.write(frame_b);
        delayMicroseconds(200);
      }
      //Serial.flush();

      
      while(Serial.available()==0);
         end_byte = 0;
        while(end_byte != '1')
        {
          while(Serial.available()==0);
          end_byte = Serial.read();
        }
        
      delay(50);
    }
 
 
 
     dataFile.close();
      
      /*
    
      while(Serial.read() >= 0);
      
      Serial.println( (byte) 1);
      delay(1);
      Serial.println( (byte) 1);
      delay(1);
      Serial.println( (byte) 1);
      delay(1);
      Serial.println( (byte) 1);
      delay(1);
      */
    
   

    
  
}

