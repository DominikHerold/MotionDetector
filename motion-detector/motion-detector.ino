#include <Arduino.h>

#if defined(ESP8266)
#include <FS.h>                     // must be first
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <base64.h>
#endif
#if defined(ARDUINO_SAMD_ZERO)
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#endif
#include <ArduinoJson.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>

#if defined(ESP8266)
ESP8266WebServer server(80);
int TimeZone=1;
#endif

#if defined(ARDUINO_SAMD_ZERO)
RH_RF69 rf69(RFM69_CS, RFM69_INT);
RHReliableDatagram manager(rf69, CLIENT_ADDRESS);
#endif

#define pirPin D1 // Input for HC-SR501
#define redpin D2

int pirValue; // variable to store read PIR Value
unsigned long act_milli;
unsigned long last_update_attempt;
const unsigned long pause_between_update_attempts = 86400000;

String esp_chipid;

char wlanssid[65] = "foo";
char wlanpwd[65] = "bar";

/*****************************************************************
/* Debug output                                                  *
/*****************************************************************/
void debug_out(const String& text, const bool linebreak) {
	if (linebreak) {
		Serial.println(text);
	} else {
		Serial.print(text);
	}
}

/*****************************************************************
/* IPAddress to String                                           *
/*****************************************************************/
String IPAddress2String(const IPAddress& ipaddress) {
	char myIpString[24];
	sprintf(myIpString, "%d.%d.%d.%d", ipaddress[0], ipaddress[1], ipaddress[2], ipaddress[3]);
	return String(myIpString);
}

/*****************************************************************
/* WiFi auto connecting script                                   *
/*****************************************************************/
void connectWifi() {
#if defined(ESP8266)
	int retry_count = 0;
	debug_out(String(WiFi.status()),1);
	WiFi.mode(WIFI_STA);
	WiFi.begin(wlanssid, wlanpwd); // Start WiFI

	debug_out(F("Connecting to "),0);
	debug_out(wlanssid,1);

	while ((WiFi.status() != WL_CONNECTED) && (retry_count < 20)) {
		delay(500);
		debug_out(".",0);
		retry_count++;
	}
	debug_out("",1);
	if (WiFi.status() != WL_CONNECTED) {		
		if (WiFi.status() != WL_CONNECTED) {
			retry_count = 0;
			while ((WiFi.status() != WL_CONNECTED) && (retry_count < 20)) {
				delay(500);
				debug_out(".",0);
				retry_count++;
			}
			debug_out("",1);
		}
	}
	WiFi.softAPdisconnect(true);
	debug_out(F("WiFi connected\nIP address: "),0);
	debug_out(IPAddress2String(WiFi.localIP()),1);
#endif
}

void setup() 
{
  Serial.begin(9600);
  Wire.begin(D3,D4);
  esp_chipid = String(ESP.getChipId());
  WiFi.persistent(false);
  connectWifi();
  pinMode(pirPin, INPUT);
  pinMode(redpin, OUTPUT);
  delay(10);
  debug_out("\nChipId: ",0);
  debug_out(esp_chipid,1);
  pirValue = 0;
  
  wdt_disable();
  wdt_enable(30000);// 30 sec
  
  debug_out(F("Es geht los."),1);
}

void loop() 
{  
  act_milli = millis();  
  
  wdt_reset();
  
  int actualPirValue = digitalRead(pirPin);
  if (actualPirValue != pirValue){
	pirValue = actualPirValue;
	if (pirValue == 1){
		debug_out(F("BEWEGUNG BEWEGUNG BEWEGUNG: "),0);
		debug_out(String(act_milli / 1000),1);
		analogWrite(redpin, 250);
	}
	else{
		analogWrite(redpin, 0);
	}
  }
	
  if ((act_milli-last_update_attempt) > (28 * pause_between_update_attempts)) {
	ESP.restart();
  }
  
  if (WiFi.status() != WL_CONNECTED) {  // reconnect if connection lost
	int retry_count = 0;
	debug_out(F("Connection lost, reconnecting "),0);
	WiFi.reconnect();
	while ((WiFi.status() != WL_CONNECTED) && (retry_count < 20)) {
		delay(500);
		debug_out(".",0);
		retry_count++;
	}
	debug_out("",1);
  }
  
  yield();
}
