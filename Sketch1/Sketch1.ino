#include <ov5642_regs.h>
#include <SPI.h>
#include <GSM.h>
#include <Wire.h>
#include <ArduCAM.h>
#include "memorysaver.h"

//  GSM network basics
#define SIMPIN ""
GSMClient cl;
GPRS gprs;
GSM gsmAccess;

//  GPRS network basics
#define GPRS_A "internet"

//  network basics
char server[] = "mail.vc0.pl";
int i = 0;

//  CAM settings
#define   FRAMES_NUM    0x00
const int CS = 8;
const int CVOL = 4;
bool is_header = false;
ArduCAM cam(OV5642, CS);
uint8_t read_fifo_burst(ArduCAM cam);
uint32_t len;
bool vals[9];

void setup() {
	// KEEP ALIVE VOLTAGE
	pinMode(CVOL, OUTPUT);
	digitalWrite(CVOL, HIGH);
	// --KEEP ALIVE VOLTAGE

	uint8_t vid, pid;
	uint8_t temp;
	Wire.begin();
	Serial.begin(115200);
	Serial.println(F("INIT"));

	// cam setup
	pinMode(CS, OUTPUT);
	digitalWrite(CS, HIGH);
	// initialize SPI:
	SPI.begin();
	//Reset the CPLD
	cam.write_reg(0x07, 0x80);
	delay(100);
	cam.write_reg(0x07, 0x00);
	delay(100);

	cam.set_format(JPEG);
	cam.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
	cam.InitCAM();
	cam.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
	cam.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
	cam.OV5642_set_JPEG_size(OV5642_640x480);
	delay(1000);
	cam.clear_fifo_flag();
	cam.write_reg(ARDUCHIP_FRAMES, 0x00);

	Serial.print(F("CAM"));
	takePic();
	Serial.print(F("-OK\n"));
	read_fifo_burst(cam);
	cam.clear_fifo_flag();
	
	delay(5000);
	cam.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
	cl.stop();
	Serial.println(F("HTTP200"));
	gsmAccess.shutdown();
	digitalWrite(CVOL, LOW);
}

void loop() {
	delay(10000);
}

void takePic() {
	cam.flush_fifo();
	cam.clear_fifo_flag();
	//Start capture
	cam.start_capture();
	while (!cam.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
	// Go into low power mode (save power!)
	cam.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
}

uint8_t read_fifo_burst(ArduCAM cam)
{
	uint8_t curr = 0, temp_last = 0;
	uint32_t length = 0;
	static int i = 0;
	byte buf[512];
	length = cam.read_fifo_length();

	cam.CS_LOW();
	cam.set_fifo_burst();//Set fifo burst mode
	i = 0;

	Serial.print(F("GSM"));
		boolean dc = true;
		while (dc)
		{
			if ((gsmAccess.begin(SIMPIN) == GSM_READY) & (gprs.attachGPRS(GPRS_A, GPRS_A, "") == GPRS_READY))
			{
				dc = false;
			}
			else
			{
				Serial.println(F("_NC"));
				delay(1000);
			}
		}
	Serial.print(F("-OK\n"));
	cl.connect(server, 80);
	len = length+139;
	Serial.print(len);

	if (cl.connected())
	{
		cl.println(F("POST /pro/u.php HTTP/1.1"));
		cl.println(F("Host: mail.vc0.pl"));
		cl.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
		cl.print(F("Content-Length: "));
		cl.println(len);
		cl.println();
		cl.println(F("--AaB03x"));
		cl.println(F("Content-Disposition: form-data; name=\"f\"; filename=\"1.JPG\""));
		cl.println(F("Content-Type: file"));
		cl.println(F("Content-Transfer-Encoding: binary"));
		cl.println();
		Serial.println(F("REQ_OK"));
			
		while (length--)
		{
			temp_last = curr;
			curr = SPI.transfer(0x00);
			//Read JPEG data from FIFO
			if ((curr == 0xD9) && (temp_last == 0xFF)) //If its EOF, break while loop
			{
				buf[i++] = curr;  //save the last 0XD9 to buffer and write remaining bytes to client
				cam.CS_HIGH();
				cl.write(buf, i);
				is_header = false;
				cam.CS_LOW();
				cam.set_fifo_burst();
				i = 0;
				break;
			}
			if (is_header == true)
			{
				//Write image data to buffer if not full
				if (i < 512)
					buf[i++] = curr;
				else
				{
					//Write 512 bytes image data to file
					cam.CS_HIGH();
					cl.write(buf, 512);
					i = 0;
					buf[i++] = curr;
					cam.CS_LOW();
					cam.set_fifo_burst();
				}
			}
			else if ((curr == 0xD8) & (temp_last == 0xFF))
			{
				is_header = true;
				cam.CS_HIGH();
				cam.CS_LOW();
				cam.set_fifo_burst();
				buf[i++] = temp_last;
				buf[i++] = curr;
			}
		}
		cl.println();
		cl.println(F("--AaB03x--"));
		cl.println();
	}
	else
	{
		Serial.println(F("WS_FAIL"));
	}
	cam.CS_HIGH();
	return 1;
}