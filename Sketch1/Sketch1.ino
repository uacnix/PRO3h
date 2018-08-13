#include <ov5642_regs.h>
#include <SPI.h>
#include <GSM.h>
#include <Wire.h>
#include <ArduCAM.h>
#include "memorysaver.h"

//  GSM network basics
#define SIMPIN ""
GSMClient client;
GPRS gprs;
GSM gsmAccess;

//  GPRS network basics
#define GPRS_A "internet"

//  network basics
char server[] = "mail.vc0.pl";
int i = 0;

//  CAM settings
#define   FRAMES_NUM    0x00
const int CS = 4;
const int CLED = 8;
bool is_header = false;
ArduCAM myCAM(OV5642, CS);
uint8_t read_fifo_burst(ArduCAM myCAM);
/*
void setup()
{
Serial.begin(115200);
Serial.println(F("INIT"));

pinMode(CS, OUTPUT);
digitalWrite(CS, HIGH);
SPI.begin();
myCAM.write_reg(0x07, 0x80);
delay(100);
myCAM.write_reg(0x07, 0x00);
delay(100);
myCAM.set_format(JPEG);
myCAM.InitCAM();
myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
myCAM.OV5642_set_JPEG_size(OV5642_1024x768); delay(2000);
myCAM.clear_fifo_flag();
myCAM.write_reg(ARDUCHIP_FRAMES, FRAMES_NUM);

// call the camera to take the shot and write it onto SD card.
*/

void setup() {
	// put your setup code here, to run once:
	uint8_t vid, pid;
	uint8_t temp;
	Wire.begin();
	Serial.begin(57600);
	Serial.println(F("INIT"));
	// set the CS as an output:
	pinMode(CS, OUTPUT);
	pinMode(CLED, OUTPUT);
	digitalWrite(CS, HIGH);
	// initialize SPI:
	SPI.begin();
	//Reset the CPLD
	myCAM.write_reg(0x07, 0x80);
	delay(100);
	myCAM.write_reg(0x07, 0x00);
	delay(100);
	//Change to JPEG capture mode and initialize the OV5642 module
	myCAM.set_format(JPEG);
	myCAM.InitCAM();
	myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
	myCAM.OV5642_set_JPEG_size(OV5642_1024x768);
	delay(1000);
	myCAM.clear_fifo_flag();
	myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);

	Serial.println(F("takePic"));
	takePic();
	digitalWrite(CLED, HIGH);
	read_fifo_burst(myCAM);
	digitalWrite(CLED, LOW);
	//Clear the capture done flag
	myCAM.clear_fifo_flag();
	delay(5000);
	client.stop();
	Serial.println(F("HTTP200"));
}

void loop() {
	delay(10000);
}

void takePic() {
	myCAM.flush_fifo();
	myCAM.clear_fifo_flag();
	//Start capture
	myCAM.start_capture();
	while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
}

uint8_t read_fifo_burst(ArduCAM myCAM)
{
	uint8_t temp = 0, temp_last = 0;
	uint32_t length = 0;
	static int i = 0;
	byte buf[32];
	length = myCAM.read_fifo_length();

	myCAM.CS_LOW();
	myCAM.set_fifo_burst();//Set fifo burst mode
	i = 0;

	boolean dc = true;
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
	Serial.println(F("GSM_OK"));
	client.connect(server, 80);
	uint32_t len = length + 139;
	Serial.print(len);
	if (client.connected())
	{
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
		Serial.println(F("HTTPOST_OK"));
		if (1)
		{
			while (length--)
			{
				//Serial.print(F("W:"));
				//Serial.println(length);
				temp_last = temp;
				temp = SPI.transfer(0x00);
				//Read JPEG data from FIFO
				if ((temp == 0xD9) && (temp_last == 0xFF)) //If find the end ,break while,
				{
					buf[i++] = temp;  //save the last  0XD9     
									  //Write the remain bytes in the buffer
					myCAM.CS_HIGH();
					client.write(buf, i);
					//Close the file
					//Serial.println(F("OK"));
					is_header = false;
					myCAM.CS_LOW();
					myCAM.set_fifo_burst();
					i = 0;
					break;
				}
				if (is_header == true)
				{
					//Write image data to buffer if not full
					if (i < 32)
						buf[i++] = temp;
					else
					{
						//Write 32 bytes image data to file
						myCAM.CS_HIGH();
						client.write(buf, 32);
						i = 0;
						buf[i++] = temp;
						myCAM.CS_LOW();
						myCAM.set_fifo_burst();
					}
				}
				else if ((temp == 0xD8) & (temp_last == 0xFF))
				{
					is_header = true;
					myCAM.CS_HIGH();
					myCAM.CS_LOW();
					myCAM.set_fifo_burst();
					buf[i++] = temp_last;
					buf[i++] = temp;
				}
			}
			client.println();
			client.println(F("--AaB03x--"));
			client.println();
		}
		else
		{
			Serial.println(F("F404"));
		}
	}
	else
	{
		Serial.println(F("WS_FAIL"));
	}
	myCAM.CS_HIGH();
	return 1;
}