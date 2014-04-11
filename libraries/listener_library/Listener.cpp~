/* Colorduino "LISTEN " Function for full animations.

    Blue Knights 2014 Bit x Bit 

    This code is designed to "listen" for serial frames from the Arduino Pro Mini,
    and display them 
*/

#include <Listener.h>

typedef unsigned char uint8_t;

const     int BUFFER_SIZE = 192;
const uint8_t FRAME_START = 2;
const uint8_t FRAME_DONE  = 1;

uint8_t buffer[BUFFER_SIZE];

void ListenerInit()
{
  Serial.begin(57600); //  115200);
  
  Colorduino.Init(); // initialize the board
  unsigned char whiteBalVal[3] = {34,63,63}; // for LEDSEE 6x6cm round matrix
  Colorduino.SetWhiteBal(whiteBalVal);
  
  Colorduino.ColorFill(255,0,0);
  delay(2000);
  Colorduino.ColorFill(0,0,0);
  
  
  // To MANUALLY set colors and test the buffer / fill sequence, use binary numbers here
  
  /*
  for(i = 0; i < 64; i++ )
  {
    buffer[i*3] = B00000000;
    buffer[i*3+1] = B11111111;
    buffer[i*3+2] = B11111111;
  }
  */
}

void ListenerIdleFunction()
{
  while (Serial.read() != FRAME_START)
  {
    // Wait for the FRAME_START
  }
    
  int val;
  for (int i = 0; i < BUFFER_SIZE; ++i)
  {
    while ((val = Serial.read()) < 0)
    {
      // Wait for the next byte
    }

    buffer[i] = (uint8_t) val;       
  }

  // Send the "all clear", releasing the other processor while the colors are transferred here
  //
  Serial.write(FRAME_DONE);

  // Transfer the colors
  //
  int led = 0;
  int index;
  for(int k = 0; k < 8; ++k)
  {
    for(int j = 0; j < 8; ++j, ++led)
    {
      index = led * 3;
      Colorduino.SetPixel(k, j, buffer[index + 2], buffer[index + 1], buffer[index]);
    }
  }   

  Colorduino.FlipPage();
}
