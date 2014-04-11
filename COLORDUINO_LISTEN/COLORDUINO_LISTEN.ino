/* Colorduino "LISTEN " Function for full animations.

    Blue Knights 2014 Bit x Bit 

    This code is designed to "listen" for serial frames from the Arduino Pro Mini,
    and display them 
*/

#include <Colorduino.h>
#include <Listener.h>

void setup()
{
  ListenerInit();
}

void loop()
{
  ListenerIdleFunction();
}
