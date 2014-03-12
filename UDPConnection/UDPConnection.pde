//import hypermedia.net.*;
//
//UDP director;
//
///* constants */
//int ip1 = 230, ip2 = 230, ip3 = 230, ip4 = 230;
//String IP = "";
//int PORT = 8888;
//boolean LOG = false;
//boolean BROADCAST = true;
//
//void setup() 
//{
//}
//
//void draw()
//{
//  IP = ip1 + "." + ip2 + "." + ip3 + "." + ip4;
//  director = new UDP(this, PORT, IP);
//  director.log(LOG);
//  director.broadcast(BROADCAST);
//  director.send(IP);
//  if (ip4++ % 256 == 0 ) 
//  {
//    if (++ip3 % 255 == 0) 
//    {
//      if (++ip2 % 255 == 0)
//      {
//        if (++ip1 % 255 == 0)
//        {
//          ip1 = 230;
//          print("I hoped this helped");
//        }
//        ip2 = 230;
//      }
//      ip3 = 230;
//    }
//    ip4 = 230;
//  } 
//  director.close();
//}

//  Processing sketch to run with this example
// =====================================================
 
 // Processing UDP example to send and receive string data from Arduino 
 // press any key to send the "Hello Arduino" message
 
 
 import hypermedia.net.*;
 
 UDP udp;  // define the UDP object
 
 
 void setup() {
 udp = new UDP( this, 8888 );  // create a new datagram connection on port 6000
 udp.log( true );     // <-- printout the connection activity
 udp.listen( true );           // and wait for incoming message  
 }
 
 void draw()
 {
   keyPressed();
 }
 
 void keyPressed() {
 String ip       = "255.255.255.0";  // the remote IP address
 int port        = 8888;    // the destination port
 
 udp.send("Hello World", ip, port );   // the message to send
 
 }
 
 void receive( byte[] data ) {       // <-- default handler
 //void receive( byte[] data, String ip, int port ) {  // <-- extended handler
 
 for(int i=0; i < data.length; i++) 
 print(char(data[i]));  
 println();   
 }
 
