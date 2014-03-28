import hypermedia.net.*;

UDP director;

String ip_address = "236.255.255.250";
int portnumber = 8888;
boolean log = true;
boolean broadcast = true;

void setup() {
  director = new UDP(this,8888,ip_address);
  director.log(log);
  director.broadcast(broadcast);
}

void draw(){
  
}

void keyPressed() {
  // Delay time so that multiple commands dont get recieved in the same
  //   packet on the arduino.
  
  // key: The key that is pressed
  // Animation number
  int animNumber = 9999;
  String command;
  
  command = "" + key;
  // command += animNumber;
  
  if (key == 'i')
  {
    director.send(command);
    print("sent" + command);
  }
  else
  {
    director.send(command);
  }
}

/*String getWritable(int animNumber) 
{
  char command[];
  int commandSize = 0;
  animSize = Math.log(animNumber);
  command[0] = key;
  commandSize++;
  
  for (int i = 0; i < animSize; i++)
  {
    command[i + commandSize] = animNumber % 10;
    animNumber /= 10;
  }
}*/
