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

// you can change this clock speed but DI
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER); 

#define WLAN_SSID       "HOME-5DF2"        // cannot be longer than 32 characters!
#define WLAN_PASS       "C0DB38BC23103E73"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define LOCAL_PORT       8888
#define PACKET_SIZE		650

Adafruit_CC3000_Client lightBox;

boolean setupCC3000() 
{
	Serial.println("Hello, CC3000!\n"); 

	displayDriverMode();

	/* Initialise the module */
	Serial.println("\nInitialising the CC3000 ...");
	if (!cc3000.begin())
	{
		Serial.println("Unable to initialise the CC3000! Check your wiring?");
		return false;
	}
  
	uint16_t firmware = checkFirmwareVersion();
	if ((firmware != 0x113) && (firmware != 0x118)) {
		Serial.println("Wrong firmware version!");
		return false;
	}

	displayMACAddress();

	/* Delete any old connection data on the module */
	Serial.println("\nDeleting old connection profiles");
	if (!cc3000.deleteProfiles()) {
		Serial.println("Failed!");
		return false;
	}

	/* Attempt to connect to an access point */
	char *ssid = WLAN_SSID;             /* Max 32 connecthars */
	Serial.print("Attempting to connect to "); 
	Serial.println(ssid);
  
	/* NOTE: Secure connections are not available in 'Tiny' mode! */
	if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
		Serial.println("Failed");
		return false;
	}
   
  
	/* Wait for DHCP to complete */
	Serial.println("Request DHCP");
	while (!cc3000.checkDHCP())
	{
		delay(100); // ToDo: Insert a DHCP timeout!
	}  

	/* Display the IP address DNS, Gateway, etc. */  
	while (! displayConnectionDetails()) {
		delay(1000);
	}

	Serial.println("Initialised!");
	return true;
}

void setup()
{
	// Setup Serial
	Serial.begin(115200);
	while(!setupCC3000()){ cc3000.reboot(); }

	uint32_t ip = cc3000.IP2U32(224,192,32,255);
	uint16_t port = 22601;

	unsigned long timeout = millis() + 15000;

	do 
	{
		lightBox = cc3000.connectUDP(ip, port);
		if (millis() >= timeout) 
		{
			Serial.println("failed to get a UDP socket from the CC3000");
			cc3000.reboot();
		}
	}
	while (!lightBox.connected());
	Serial.println("end of setup");
}

void loop()
{
	char packetBuffer[PACKET_SIZE] = "this is a message";

	Serial.println("beginning of loop");
	// usual cc3000 initialization, etc., here...
	// n == 650
	int cc = lightBox.write(packetBuffer, PACKET_SIZE);
	// if (cc < 0) { 
	// 	Serial.print("error writing packet: "); Serial.println(PACKET_SIZE);
	// } else if (cc != PACKET_SIZE) {
	// 	Serial.print("wrote "); Serial.print(cc); Serial.print(" octets, but expected to write "); Serial.println(n);
	// }

}

/*	CC3000 helper functions	*/
/**************************************************************************/
/*!
	@brief  Displays the driver mode (tiny of normal), and the buffer
			size if tiny mode is not being used

	@note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
	Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
	Serial.print(F("RX Buffer : "));
	Serial.print(CC3000_RX_BUFFER_SIZE);
	Serial.println(F(" bytes"));
	Serial.print(F("TX Buffer : "));
	Serial.print(CC3000_TX_BUFFER_SIZE);
	Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
	@brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
	Serial.println(F("Unable to retrieve the firmware version!\r\n"));
	version = 0;
  }
  else
  {
	Serial.print(F("Firmware V. : "));
	Serial.print(major); Serial.print(F(".")); Serial.println(minor);
	version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
	@brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
	Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
	Serial.print(F("MAC Address : "));
	cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
	@brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
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