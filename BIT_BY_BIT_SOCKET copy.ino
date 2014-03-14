#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include "utility/socket.h"


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
int port = 8888;
int skt; 
long bind_var;
short sin_family;
unsigned short sin_port;
unsigned long s_addr;
char sin_zero[8];
uint32_t ip_holder; 

//sockaddr_in SOCK_ADDER_test;
// Keith was testing something here, not sure what, commenting out for now


void setup (){
  
sockaddr_in SOCK_ADDER;
memset(sin_zero,0,sizeof(sin_zero));

in_addr sinaddr;
sinaddr.s_addr;

SOCK_ADDER.sin_family = AF_INET;
SOCK_ADDER.sin_port = htons(port);

ip_holder = 0;

SOCK_ADDER.sin_addr.s_addr = htonl(ip_holder);
SOCK_ADDER.sin_zero[8] = 0;

  
  
// SOCK_ADDER *addr;
  
  Serial.begin(9600);
  cc3000.begin();
  cc3000.deleteProfiles();
  cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  } 
  while(!(skt = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP)) > 0 );
  Serial.println(skt);
  Serial.println("while loop 1 passed");
  // Tried placing a 0 where the (const sockaddr *addr should go based on the tutorial that states we can just place 0.0.0.0 as the IP address... MAY NOT WORK
  
  
   uint32_t *ipAddress, *netmask, *gateway, *dhcpserv, *dnsserv;
   if(cc3000.getIPAddress(ipAddress, netmask, gateway, dhcpserv, dnsserv)) {
     Serial.println(*ipAddress);

   }
   else
   {
     Serial.println("couln't find an IP address");
   }
   
   // trying it backwards because we were receiving a backwards IP on the serial output
   // in_addr sin_addr =  { cc3000.IP2U32((uint8_t)192, (uint8_t)168, (uint8_t)3, (uint8_t)0) };


Serial.println(SOCK_ADDER.sin_addr.s_addr);
//sockaddr_in SOCK_ADDER = { AF_INET,htons(8888), 0 , 0 };
  
  while( (bind_var = bind( skt, (sockaddr *)&SOCK_ADDER , sizeof(SOCK_ADDER))) == -1 ) { Serial.println("here"); };
  Serial.println(bind_var);
  
  
  Serial.println("while loop 2 passed");
}

void loop() {  
  
  char *buf;
  /*
  Serial.println(F("Inside the loop"));
  int size_returned = recv( skt, buf, 120, 0);
  Serial.println(bind_var);
  Serial.println(size_returned);
  delay(1000);*/
  socklen_t SOCK_LENGTH = sizeof(SOCK_ADDER_test);
  
  // printf("waiting on port %d\n", port);
  int recvlen = recvfrom(skt, buf, BUFSIZE, 0, (sockaddr *)&SOCK_ADDER_test, &SOCK_LENGTH);
  // printf("received %d bytes\n", recvlen);
  Serial.println(recvlen);
  if (recvlen > 0) {
      buf[recvlen] = 0;
      printf("received message: \"%s\"\n", buf);
  }
  
  
  
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
  
