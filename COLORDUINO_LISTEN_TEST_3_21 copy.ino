/* Colorduino "LISTEN " Function for full animations.

    Blue Knights 2014 Bit x Bit 

    This code is designed to "listen" for serial frames from the Arduino Pro Mini,
    and display them 


*/

#include <Colorduino.h>

#define READY_STATE B01010101
#define BUFFER_SIZE 192

int i = 0;
byte b = B00000000;
byte g = B00000000;
byte r = B00000000;
byte buffer[192];
byte temp_byte;
byte start_byte_check;
char test_char;
boolean SYNC_ERROR;


void setup(){
  
  Serial.begin(9600);
  
  Colorduino.Init(); // initialize the board
  unsigned char whiteBalVal[3] = {34,63,63}; // for LEDSEE 6x6cm round matrix
  Colorduino.SetWhiteBal(whiteBalVal);
  
  Colorduino.ColorFill(255,0,0);
  delay(2000);
  Colorduino.ColorFill(0,0,0);
  
  
    // To MANUALLY set colors and test the buffer / fill sequence, use binary numbers here
  
 /* for(i = 0; i < 64; i++ ) {       
          buffer[i*3] = B00000000;
          buffer[i*3+1] = B11111111;
          buffer[i*3+2] = B11111111;
  }
  */
  
  
}

void loop(){
  int frame_byte;
  int start_byte_check;
  
  while (1)
  {
    Serial.println(READY_STATE);
    while ( (start_byte_check = Serial.read()) < 0) 
    {
      Serial.println(READY_STATE);
    }
    
    if (start_byte_check == 2)
    {
      break;
    }
  }
  Colorduino.ColorFill(200,200,0);
  for (int i = 0; i < BUFFER_SIZE; ++i)
  {
    while ((frame_byte = Serial.read()) < 0);
    buffer[i] = frame_byte;       
  }
 

 for(int k = 0; k<8; k++){
     for(int j = 0; j<8; j++){
       Colorduino.SetPixel(k, j, buffer[k*3*8+j*3+0], buffer[k*3*8+j*3+1], buffer[k*3*8+j*3+2]);
     }
  }   
  Colorduino.FlipPage();
//  delay(10000);
   /*
    
    for(i = 0; i < 192; i++  ){
        //while not end of half-frame
          while(Serial.available() < 2) delayMicroseconds(1);
          buffer[i++] = Serial.read();
        }
        Serial.flush();
        //Serial.write(FRAME_END);
    
    */
    
    //rxComplete();

}
/*
void rxComplete(){
  for(int i = 0; i<8; i++){
     for(int j = 0; j<8; j++){
       Colorduino.SetPixel(i, j, buffer[i*3*8+j*3+0], buffer[i*3*8+j*3+1], buffer[i*3*8+j*3+2]);
     }
  }
  Colorduino.FlipPage();
}
*/
