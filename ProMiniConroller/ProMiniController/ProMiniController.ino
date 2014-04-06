#include <SD.h>

#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <SPI.h>
#include <utility/Sd2Card.h>
#include <utility/SdFat.h>
#include <utility/SdFatUtil.h>

#include <Averager.h>
#include <BKProtocol.h>
#include <Common.h>
#include <GetTime.h>
#include <SimpleFile.h>
#include <ArduinoIdle.h>

void setup()
{
  ArduinoInit();
}

void loop()
{
  ArduinoIdleFunction();
}

