
#include <SPI.h>
#include <string.h>
#include<stdlib.h>
#include <SD.h>
#include <GSM.h>

//	GSM network basics
#define SIMPIN ""
GSMClient client;
GPRS gprs;
GSM gsmAccess;

//	GPRS network basics
#define GPRS_APN "internet"
#define GPRS_LOG "internet"
#define GPRS_PASS ""

//	network basics
char filename[14] = "1.jpg";
char server[] = "mail.vc0.pl";
int i = 0;
int keyIndex = 1;
int port = 80; //Server Port 
uint32_t MANip = 0;

void setup()
{

	Serial.begin(115200);
	// Initialise the module
	Serial.println(F("\nInitializing..."));
	// Connect to GSM network
	boolean dc = true;

	// After starting the modem with GSM.begin()
	// attach the shield to the GPRS network with the APN, login and password 
	while (dc)
	{
		if ((gsmAccess.begin(SIMPIN) == GSM_READY) & (gprs.attachGPRS(GPRS_APN, GPRS_LOG, GPRS_PASS) == GPRS_READY))
		{
			dc = false;
		}
		else
		{
			Serial.println(F("Not connected"));
			delay(1000);
		}
	}


	Serial.println(F("Connected!"));

	// Display connection details
	Serial.println("Ready");
	Serial.println("-------");

	//Get MAN IP address

	// Prepare HTTP request
	String start_request = "";
	String end_request = "";
	start_request = start_request + "\n" + "--AaB03x" + "\n" + "Content-Disposition: form-data; name=\"fileToUpload\"; filename=" + filename + "\n" + "Content-Type: file" + "\n" + "Content-Transfer-Encoding: binary" + "\n" + "\n";
	end_request = end_request + "\n" + "--AaB03x--" + "\n";



	//Initialise SD Card
	while (!SD.begin(4))
	{
		Serial.println("SD init FAIL!");
		delay(1000);
	}
	Serial.println("SD init OK");

	File myFile = SD.open("1.jpg");
	uint16_t jpglen = myFile.size();
	uint16_t extra_length;
	extra_length = start_request.length() + end_request.length();
	uint16_t len = jpglen + extra_length;

	if (client.connected())
	{
		Serial.println("Start uploading...");

		client.println(F("POST /upload.php HTTP/1.1"));

		client.println(F("Host: www.mail.vc0.pl"));
		client.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
		client.print(F("Content-Length: "));
		client.println(len);
		client.println(start_request);

		Serial.println(F("Host: www.mail.vc0.pl"));
		Serial.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
		Serial.print(F("Content-Length: "));
		Serial.println(len);
		Serial.println(start_request);

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
			client.print(end_request);
			client.println();
		}
		else
		{
			Serial.println("File not found");
		}
	}
	else
	{
		Serial.println("Web server connection failed");
	}

	myFile.close();
	client.stop();

	Serial.println("done...");
}

void loop() {



	delay(10000);

}