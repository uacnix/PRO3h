
#include <SPI.h>
#include<stdlib.h>
#include <SD.h>
#include <GSM.h>

//	GSM network basics
#define SIMPIN ""
GSMClient client;
GPRS gprs;
GSM gsmAccess;

//	GPRS network basics
#define GPRS_A "internet"

//	network basics
char server[] = "mail.vc0.pl";
int i = 0;

void setup()
{

	Serial.begin(115200);
	// Initialise the module
	//Serial.println(F("\nINIT"));
	// Connect to GSM network
	boolean dc = true;

	// After starting the modem with GSM.begin()
	
	//attach the shield to the GPRS network with the APN, login and password 
	while (dc)
	{
		if ((gsmAccess.begin(SIMPIN) == GSM_READY) & (gprs.attachGPRS(GPRS_A, GPRS_A, "") == GPRS_READY))
		{
			dc = false;
		}
		else
		{
			Serial.println(F("GSM_NC"));
			delay(1000);
		}
	}
	
	Serial.println(F("GSM OK"));

	client.connect(server, 80);
	// Prepare HTTP request
	//Initialise SD Card
	while (!SD.begin(9))
	{
		Serial.println(F("SD FAIL!"));
		delay(1000);
	}
	Serial.println(F("SD OK"));

	File myFile = SD.open("1.JPG");
	uint16_t jpglen = myFile.size();
	//Serial.println(jpglen);
	uint16_t extra_length;
	uint16_t len = jpglen + 139;
	
	if (client.connected())
	{
		//Serial.println(F("UP_"));
		//client.flush();
		client.println(F("POST /pro/u.php HTTP/1.1"));
		client.println(F("Host: mail.vc0.pl"));
		client.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
		client.print(F("Content-Length: "));
		client.println(len);
		client.println();
		client.println(F("--AaB03x"));
		client.println(F("Content-Disposition: form-data; name=\"f\"; filename=\"1.JPG\""));
		client.println(F("Content-Type: file"));
		client.println(F("Content-Transfer-Encoding: binary"));
		client.println();
		
		if (myFile)
		{
			byte clientBuf[32];
			int clientCount = 0;

			while (myFile.available())
			{
				clientBuf[clientCount] = myFile.read();
				clientCount++;

				if (clientCount > 31)
				{
					client.write(clientBuf, 32);
					clientCount = 0;
				}
			}
			if (clientCount > 0)
				client.write(clientBuf, clientCount);
			client.println();
			client.println(F("--AaB03x--"));
			client.println();
		}
		else
		{
			Serial.println(F("SD404"));
		}
	}
	else
	{
		Serial.println(F("WS_FAIL"));
	}

	myFile.close();
	client.stop();

	Serial.println(F("HTTP200"));
}

void loop() {
	delay(10000);
}