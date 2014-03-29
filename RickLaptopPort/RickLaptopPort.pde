import hypermedia.net.*;

UDP          director;
LaptopStateT laptop;

String  ip_address = "236.255.255.250";
int     portnumber = 8888;
boolean log = false; // true;
boolean broadcast = true;

static final String path = "testMovements/";


  

//--- Java quarks---//
// Java handles function call parameters as exclusively pass by value IF 
//   the parameters are a non object.
// If a parameter is an object the object referance is passed to the function
//   and the parameter's values have been changed.
//
// Java does not allow for unsigned primitive types. All of the unsigned ints
//   should be handled as abs(long)
void setup() 
{
  director = new UDP(this,8888,ip_address);
  director.log(log);
  director.broadcast(broadcast);
 
  laptop = new LaptopStateT(director);
}

void draw()
{
  laptop.LoopFunction('\0');
}


void keyPressed()
{
  laptop.LoopFunction(key);
}
