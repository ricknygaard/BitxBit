import hypermedia.net.*;

UDP director;

/* constants */
String IP = "236.255.255.250";
int PORT = 8888;
boolean LOG = true;
boolean BROADCAST = true;

void setup() 
{
  director = new UDP(this, PORT, IP);
  director.log(LOG);
  director.broadcast(BROADCAST);
}

void draw()
{
  director.send("This statement is false");
}
