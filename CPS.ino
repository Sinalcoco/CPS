/*
 Name:		CPS.ino
 Created:	16.04.2017 14:37:17
 Author:	Simon
*/

#include <TinyGPS++.h>

#define A7Serial Serial1
#define GpsSerial Serial2

enum STATUS
{
	OK,
	NOTOK,
	TIMEOUT
};

#define SERIALTIMEOUT 3000



// The TinyGPS++ object
TinyGPSPlus gps;
char end_c[2];
bool printGpsData;

int led = 13;

void setup() {
	pinMode(led, OUTPUT);
	// initialize the serial ports:
	Serial.begin(9600);
	A7Serial.begin(115200);
	GpsSerial.begin(9600);

	// initialize the global variables
	end_c[0] = 0x1a;
	end_c[1] = '\0';
	printGpsData = true;

	while (A7softBegin() != OK)
	{
		//digitalWrite(led, HIGH);
		Serial.println("The A7 is not yet connected or isn't responding to AT commands!");
		delay(500);
		//digitalWrite(led, LOW);
	}
	A7command("AT+AGPS=0", "OK", "yy", 500, 1);
	delay(500);
	Serial.println("The A7 is ready to use.");

	printHelp();

	blink(3, 200);
	
	delay(5000);

	if (A7command("AT", "OK", "yy", 1000, 1) != OK)
	{
		//digitalWrite(led, HIGH);
		//delay(20000);
	}

	blink(5, 200);
	A7command("AT+AGPS=1", "OK", "yy", 10000, 1);

	blink(10, 1000);
}

void blink(int amount, int d)
{
	for (int i = 0; i < amount; i++)
	{
		digitalWrite(led, HIGH);
		delay(d/2);
		digitalWrite(led, LOW);
		delay(d/2);
	}
}

void loop() {
	// read from GPS pin and process the data. If the sentence is complete print it out if printGpsData is set to true
	while (Serial2.available() > 0)
	{
		if (gps.encode(Serial2.read()))
		{
			displayInfo(!printGpsData);
		}
	}
	// read from port 1, send to port 0:
	if (Serial1.available()) {
		int inByte = Serial1.read();
		Serial.write(inByte);
	}

	// read from port 0, send to port 1:
	if (Serial.available()) {
		char c = Serial.peek();
		int inByte = Serial.read();
		String data, gpsData, dateTimeData;
		int sep;
		switch (c)
		{
		case 'p':
			printGpsData = !printGpsData;
			break;
		case 's':
			data = displayInfo(false);
			sep = data.indexOf('*');
			if (sep < 0)
				break;
			gpsData = data.substring(0, sep);
			dateTimeData = data.substring(sep + 1);
			sendSparkfunGSM(100, gpsData, dateTimeData);
			break;
		default:
			Serial1.write(inByte);
			break;
		}
	}
}
void printHelp()
{
	Serial.println("/------------------------------Command List-------------------------------\\");
	Serial.println("|p\t\t\t|enable/disable Gps messages to Serial\t\t|");
	Serial.println("|s\t\t\t|send last Gps report to data.sparkfun\t\t|");
	Serial.println("|other command\t\t|forward to A7\t\t\t\t\t|");
	Serial.println("\\-------------------------------------------------------------------------/");
}

String displayInfo(bool silent)
{
	String gpsData;
	if (!silent)
	{
		Serial.print(F("Location: "));
	}
	if (gps.location.isValid())
	{
		if (!silent)
		{
			Serial.print(gps.location.lat(), 6);
			Serial.print(F(","));
			Serial.print(gps.location.lng(), 6);
		}
		gpsData = String(gps.location.lat(), 7) + "," + String(gps.location.lng(), 7);
	}
	else
	{
		if (!silent)
		{
			Serial.print(F("INVALID"));
		}
		gpsData = "INVALID_GPS";
	}

	if (!silent)
	{
		Serial.print(F("  Date/Time: "));
	}
	if (gps.date.isValid())
	{
		if (!silent)
		{
			Serial.print(gps.date.month());
			Serial.print(F("/"));
			Serial.print(gps.date.day());
			Serial.print(F("/"));
			Serial.print(gps.date.year());
		}
		gpsData += "*" + String(gps.date.day()) + "/" + gps.date.month() + "/" + gps.date.year();
	}
	else
	{
		if (!silent)
		{
			Serial.print(F("INVALID"));
		}
		gpsData += "*INVALID_DATE";
	}

	if (!silent)
	{
		Serial.print(F(" "));
	}
	if (gps.time.isValid())
	{
		if (!silent)
		{
			if (gps.time.hour() < 10) Serial.print(F("0"));
			Serial.print(gps.time.hour());
			Serial.print(F(":"));
			if (gps.time.minute() < 10) Serial.print(F("0"));
			Serial.print(gps.time.minute());
			Serial.print(F(":"));
			if (gps.time.second() < 10) Serial.print(F("0"));
			Serial.print(gps.time.second());
			Serial.print(F("."));
			if (gps.time.centisecond() < 10) Serial.print(F("0"));
			Serial.print(gps.time.centisecond());
		}
		gpsData += "|" + String(gps.time.hour()) + ":" + gps.time.minute() + ":" + gps.time.second() + "." + gps.time.centisecond();
	}
	else
	{
		if (!silent)
		{
			Serial.print(F("INVALID"));
		}
		gpsData += "|INVALID_TIME";
	}

	if (!silent)
	{
		Serial.println();
	}
	return gpsData;
}

///sparkfun()///
void sendSparkfunGSM(float theBattery, String thePosition, String theTime) {
	String host = "data.sparkfun.com";
	String publicKey = "xRnG5O0m4lf7vGjEo121";
	String privateKey = "Za0RXWegwBs1rj5dXYGY";
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);	//returns the current connection status
	A7command("AT+CGATT?", "OK", "yy", 20000, 2);		//Check the status of Packet service attach
	A7command("AT+CGATT=1", "OK", "yy", 20000, 2);		//Perform a GPRS Attach
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CGDCONT=1,\"IP\",\"http://wap.o2active.de\"", "OK", "yy", 20000, 2); //Define a PDP Context. First parameter is the Context ID, second is the type of IP connection and third is the APN
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CGACT=1,1", "OK", "yy", 10000, 2);	//activate the PDP context
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIFSR", "OK", "yy", 20000, 2); //get local IP adress
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIPSTART=\"TCP\",\"" + host + "\",80", "CONNECT OK", "yy", 25000, 2); //start up the connection
																						// A7input();
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIPSEND", ">", "yy", 10000, 1); //begin send data to remote server
	delay(500);
	A7Serial.print("GET /input/");
	A7Serial.print(publicKey);
	A7Serial.print("?private_key=");
	A7Serial.print(privateKey);
	A7Serial.print("&battery=");
	A7Serial.print(theBattery, 2);
	A7Serial.print("&position=");
	A7Serial.print(thePosition);
	A7Serial.print("&time=");
	A7Serial.print(theTime);
	A7Serial.print(" HTTP/1.1");
	A7Serial.print("\r\n");
	A7Serial.print("HOST: ");
	A7Serial.print(host);
	A7Serial.print("\r\n");
	A7Serial.print("\r\n");

	Serial.print("GET /input/");
	Serial.print(publicKey);
	Serial.print("?private_key=");
	Serial.print(privateKey);
	Serial.print("&battery=");
	Serial.print(theBattery, 2);
	Serial.print("&position=");
	Serial.print(thePosition);
	Serial.print("&time=");
	Serial.print(theTime);
	Serial.print(" HTTP/1.1");
	Serial.print("\r\n");
	Serial.print("HOST: ");
	Serial.print(host);
	Serial.print("\r\n");
	Serial.print("\r\n");

	A7command(end_c, "HTTP/1.1", "yy", 30000, 1); //begin send data to remote server
												  //A7Serial.println(end_c); //sending ctrlZ
	unsigned long   entry = millis();
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIPCLOSE", "OK", "yy", 15000, 1); //sending
	A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	delay(100);
	Serial.println("-------------------------End------------------------------");
}


byte A7waitFor(String response1, String response2, int timeOut) {
	unsigned long entry = millis();
	int count = 0;
	String reply = A7read();
	byte retVal = 99;
	do {
		reply = A7read();
		if (reply != "") {
			Serial.print((millis() - entry));
			Serial.print(" ms ");
			Serial.println(reply);
		}
	} while ((reply.indexOf(response1) + reply.indexOf(response2) == -2) && millis() - entry < timeOut);
	if ((millis() - entry) >= timeOut) {
		retVal = TIMEOUT;
	}
	else {
		if (reply.indexOf(response1) + reply.indexOf(response2) > -2) retVal = OK;
		else retVal = NOTOK;
	}
	//  Serial.print("retVal = ");
	//  Serial.println(retVal);
	return retVal;
}

byte A7command(String command, String response1, String response2, int timeOut, int repetitions) {
	byte returnValue = NOTOK;
	byte count = 0;
	while (count < repetitions && returnValue != OK) {
		A7Serial.println(command);
		Serial.print("Command: ");
		Serial.println(command);
		if (A7waitFor(response1, response2, timeOut) == OK) {
			//     Serial.println("OK");
			returnValue = OK;
		}
		else returnValue = NOTOK;
		count++;
	}
	return returnValue;
}




void A7input() {
	String hh;
	char buffer[100];
	while (1 == 1) {
		if (Serial.available()) {
			hh = Serial.readStringUntil('\n');
			hh.toCharArray(buffer, hh.length() + 1);
			if (hh.indexOf("ende") == 0) {
				A7Serial.write(end_c);
				Serial.println("ende");
			}
			else {
				A7Serial.write(buffer);
				A7Serial.write('\n');
			}
		}
		if (A7Serial.available()) {
			Serial.write(A7Serial.read());
		}
	}
}


byte A7begin() {
	A7Serial.println("AT+CREG?");
	byte hi = A7waitFor("1,", "5,", 1500);  // 1: registered, home network ; 5: registered, roaming
	while (hi != OK) {
		A7Serial.println("AT+CREG?");
		hi = A7waitFor("1,", "5,", 1500);
	}

	if (A7command("AT&F0", "OK", "yy", 5000, 2) == OK) {   // Reset to factory settings
		if (A7command("ATE0", "OK", "yy", 5000, 2) == OK) {  // disable Echo
			if (A7command("AT+CMEE=2", "OK", "yy", 5000, 2) == OK) return OK;  // enable better error messages
			else return NOTOK;
		}
	}
}

byte A7softBegin() {
	if (A7command("AT+CREG?", "1,", "5,", 1500, 1) != OK)  // 1: registered, home network ; 5: registered, roaming
		return NOTOK;

	// disable Echo									// enable better error messages
	if (A7command("ATE0", "OK", "yy", 5000, 2) != OK || A7command("AT+CMEE=2", "OK", "yy", 5000, 2) != OK)
	{
		return NOTOK;
	}

	return OK;
}

void ShowSerialData()
{
	unsigned long entry = millis();
	while (A7Serial.available() != 0 && millis() - entry < SERIALTIMEOUT)
		Serial.println(A7Serial.readStringUntil('\n'));
}

String A7read() {
	String reply = "";
	if (A7Serial.available()) {
		reply = A7Serial.readString();
	}
	//  Serial.print("Reply: ");
	//  Serial.println(reply);
	return reply;
}
