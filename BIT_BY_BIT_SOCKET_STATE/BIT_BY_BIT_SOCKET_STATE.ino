#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include "utility/socket.h"

/*
  Code to test that rgb box is working correctly. The test goes through seperate stages.
  
    1. Make sure that the wifi shield is connected
    2. Make sure that the wifi is recieving a signal
    3. Make Sure that I/O pins on the pro mini are working correctly. Controlling certain pins
        with the recieved data packet relating to 'r','g','b'. 
*/

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "belkin.d5c"        // cannot be longer than 32 characters!
#define WLAN_PASS       "baeba66a"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

//char ANIM_NUM = '0';

#define BUFSIZE 150

// Store other needed variables here…
int port;
int skt; 
long bind_var;
short sin_family;
unsigned short sin_port;
unsigned long s_addr;
char sin_zero[8];
uint32_t ip_holder; 
socklen_t sockLen;
unsigned char buf[BUFSIZE];
socklen_t* addrlen;
int sizeReturned;


// Keith was testing something here, not sure what, commenting out for now
//sockaddr_in SOCKET_ADDRESS_test;

//sockaddr_in SOCKET_ADDRESS, from;
sockaddr_in SOCKET_ADDRESS;
sockaddr_in remaddr;

// Testing constants
boolean wifiConnected = false;
boolean wifiRecieving = false;
boolean wifiSerial = false;

void setup (){
  
  
// Declaration and organization of the socket's address information, to be used in the bind command later  

 

// declared this another way, we may not need these two anymore 
//in_addr sinaddr;
//sinaddr.s_addr;

ip_holder = 0;

SOCKET_ADDRESS.sin_addr.s_addr = htonl(ip_holder);
SOCKET_ADDRESS.sin_zero[8] = 0;

// SOCKET_ADDRESS *addr;
  
 Serial.begin(9600);
 Serial.print('2');

// Initialization of the CC3000 wifi chip, connection to the network, etc.
  cc3000.begin();
  cc3000.deleteProfiles();
  
 
    if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    // Serial.println(F("Failed!"));
    while(1);
  }
  
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  } 
  Serial.println('1');

//  while (! displayConnectionDetails()) {
//    delay(1000);
//  }




// Create the socket, setting the parameters for UDP listening
  while(!(skt = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP)) > 0 );
  memset(&SOCKET_ADDRESS, 0, sizeof(sockaddr_in));
  port = 8888;
  SOCKET_ADDRESS.sin_family = AF_INET;
  SOCKET_ADDRESS.sin_addr.s_addr = 0;
  SOCKET_ADDRESS.sin_port = htons(port);

   // trying it backwards because we were receiving a backwards IP on the serial output
   // in_addr sin_addr =  { cc3000.IP2U32((uint8_t)192, (uint8_t)168, (uint8_t)3, (uint8_t)0) };

// declared the socket address another way so I don't think we need these next two anymore
//Serial.println(SOCKET_ADDRESS.sin_addr.s_addr);
//sockaddr_in SOCKET_ADDRESS = { AF_INET,htons(8888), 0 , 0 };
  
 // while( (bind_var = bind( skt, (sockaddr *)&SOCKET_ADDRESS , sizeof(SOCKET_ADDRESS))) == -1 ) { Serial.println("here"); };
bind_var = bind( skt, (sockaddr*)&SOCKET_ADDRESS, sizeof(sockaddr_in));
   //Serial.println(bind_var);

  Serial.print('3');

// a check to verify that the socket is created successfully
 // Serial.println(skt);
  // Serial.println("while loop 1 passed");  
  
  socklen_t addrlen = sizeof(remaddr);
  sockLen = sizeof(sockaddr_in);
   delay(200);

  
  
  //Serial.println("while loop 2 passed");
  Serial.println('0');
  
  
}

void loop() {  
    
  // RECEIVE DATA USING RECVFROM
  
   //Serial.println(F("Inside the loop"));
  sizeReturned = recvfrom( skt, buf , BUFSIZE, 0, (sockaddr*)&remaddr, addrlen);
  // Serial.println(bind_var);
   //Serial.println(sizeReturned);
  
  // Variable to be encased in struct
  String command = parseCommand(buf, sizeReturned);
  int commandSize = sizeReturned;    
  
  // Serial.print(command);
  
  if ( commandSize > 0 )
  {
    int pin;
    if(buf[0] == '1')
    {
      pin = 1;
    } 
      else if(buf[0] == '2')
    {
      pin = 2;
    }
      else if(buf[0] == '3')
    {
      pin = 3;
    } 
    else if(buf[0] == '4')
    {
      pin = 4;
    }
    else if(buf[0] == '5')
    {
      pin = 5;
    }  
    
      else
    {
      pin = 0;
    }
    
    Serial.print(pin);
  }
  
  delay(1000);
  
  
  

  
  
  /*
  
  if buf = 'A1' {
    delay(8000);
    Serial.println('1')
  else if buf = 'A2' {
    delay(7000);
    Serial.println('1');
  }
  else if buf = 'B' {
    Serial.println('2');
  }
  else if buf = 'C' { 
    Serial.println('3');
  }
  
  
  */
  
}
  
  
  bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

// This should be changed to return a struct
String parseCommand(unsigned char buf[], int bufSize)
{
  String _return = "";
  for (int i = 0; i < bufSize; i++)
  {
    _return += char(buf[i]); 
  } 
  return _return;
}
