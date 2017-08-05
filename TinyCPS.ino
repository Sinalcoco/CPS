/*
Name:		CPS.ino
Created:	16.04.2017 14:37:17
Author:	Simon
*/

#define A7Serial Serial1
#define GpsSerial Serial2

enum STATUS
{
	OK,
	NOTOK,
	TIMEOUT
};

#define SERIALTIMEOUT 3000

char end_c[2];
bool printGpsData;


void setup() {
	// initialize the serial ports:
	A7Serial.begin(115200);
	GpsSerial.begin(9600);

	// initialize the global variables
	end_c[0] = 0x1a;
	end_c[1] = '\0';

	A7softBegin();

	A7command("AT+AGPS=1", "OK", "yy", 5000, 5);
	A7Serial.end();
}

void loop() {
	A7Serial.begin(115200);
	if (A7Serial.available())
	{
		sendSparkfunGSM(100, A7Serial.readString(), "testTime");
	}
	A7Serial.end();
	delay(60000);
}

///sparkfun()///
void sendSparkfunGSM(float theBattery, String thePosition, String theTime) {
	String host = "data.sparkfun.com";
	String publicKey = "xRnG5O0m4lf7vGjEo121";
	String privateKey = "Za0RXWegwBs1rj5dXYGY";
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);	//returns the current connection status
	//A7command("AT+CGATT?", "OK", "yy", 20000, 2);		//Check the status of Packet service attach
	A7command("AT+CGATT=1", "OK", "yy", 20000, 2);		//Perform a GPRS Attach
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CGDCONT=1,\"IP\",\"http://wap.o2active.de\"", "OK", "yy", 20000, 2); //Define a PDP Context. First parameter is the Context ID, second is the type of IP connection and third is the APN
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CGACT=1,1", "OK", "yy", 10000, 2);	//activate the PDP context
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIFSR", "OK", "yy", 20000, 2); //get local IP adress
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIPSTART=\"TCP\",\"" + host + "\",80", "CONNECT OK", "yy", 25000, 2); //start up the connection
																						// A7input();
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
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

	A7command(end_c, "HTTP/1.1", "yy", 30000, 1); //begin send data to remote server
												  //A7Serial.println(end_c); //sending ctrlZ
	unsigned long   entry = millis();
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
	A7command("AT+CIPCLOSE", "OK", "yy", 15000, 1); //sending
	//A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2);
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
