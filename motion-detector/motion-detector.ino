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
#define bluepin D5
#define speakerpin D8

unsigned long act_milli;
unsigned long last_ping;
unsigned long last_update_attempt;
const unsigned long pause_between_update_attempts = 86400000;

String esp_chipid;

char wlanssid[65] = "foo"; //Todo Change
char wlanpwd[65] = "bar"; //Todo Change
String data = "{\"ping\": \"ping\"}";
const char* host = "www.foo.de"; //Todo Change
const char* url_motion = "/Motion/Create";
const char* url_wlan = "/Wlan/Create";
const char* authstring = "foobar=="; //Todo Change
int port = 443; //Todo Change

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

void alarmSound(){
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
	
	analogWrite (speakerpin, 255);
	delay (500);
	analogWrite (speakerpin, 0);
	delay (500);
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
			if (retry_count == 20){
				analogWrite(bluepin, 250);
			}
			
			debug_out("",1);
		}
	}
	WiFi.softAPdisconnect(true);
	debug_out(F("WiFi connected\nIP address: "),0);
	debug_out(IPAddress2String(WiFi.localIP()),1);
#endif
}

/*****************************************************************
/* send data to rest api                                         *
/*****************************************************************/
void sendData(const String& data, const char* host, const int httpPort, const char* url, const String& contentType, const char* basic_auth_string) {
#if defined(ESP8266)

	debug_out(F("Start connecting to "),0);
	debug_out(host,1);
	
	String request_head = F("POST "); request_head += String(url); request_head += F(" HTTP/1.1\r\n");
	request_head += F("Host: "); request_head += String(host) + "\r\n";
	request_head += F("Content-Type: "); request_head += contentType + "\r\n";
	if (basic_auth_string != "") { request_head += F("Authorization: Basic "); request_head += String(basic_auth_string) + "\r\n";}
	request_head += F("Content-Length: "); request_head += String(data.length(),DEC) + "\r\n";
	request_head += F("Connection: close\r\n\r\n");

	// Use WiFiClient class to create TCP connections

	String result = F(" ");
	
	if (httpPort == 443) {

		WiFiClientSecure client_s;
		
		client_s.setNoDelay(true);
		client_s.setTimeout(20000);

		if (!client_s.connect(host, httpPort)) {
			debug_out(F("connection failed"),1);
			return;
		}

		debug_out(F("Requesting URL: "),0);
		debug_out(url,1);
		debug_out(esp_chipid,1);
		debug_out(data,1);

		// send request to the server

		client_s.print(request_head);

		client_s.println(data);

		delay(10);

		// Read reply from server and print them
		while(client_s.available()){
			char c = client_s.read();
			debug_out(String(c),0);
			result += String(c);
		}

		debug_out(F("\nclosing connection\n------\n\n"),1);
	} else {

		WiFiClient client;
		
		client.setNoDelay(true);
		client.setTimeout(20000);

		if (!client.connect(host, httpPort)) {
			debug_out(F("connection failed"),1);
			return;
		}

		debug_out(F("Requesting URL: "),0);
		debug_out(url,1);
		debug_out(esp_chipid,1);
		debug_out(data,1);

		client.print(request_head);

		client.println(data);

		delay(10);

		// Read reply from server and print them
		while(client.available()){
			char c = client.read();
			debug_out(String(c),0);
			result += String(c);
		}

		debug_out(F("\nclosing connection\n------\n\n"),1);

	}

	if (result.contains("ALARM"){
		alarmSound();
	}
	
	debug_out(F("End connecting to "),0);
	debug_out(host,1);

	wdt_reset(); // nodemcu is alive
	yield();
#endif
}

void setup() 
{
  Serial.begin(9600);
  Wire.begin(D3,D4);
  esp_chipid = String(ESP.getChipId());
  WiFi.persistent(false);
  pinMode(pirPin, INPUT);
  pinMode(redpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(speakerpin, OUTPUT);
  connectWifi();  
  delay(10);
  debug_out("\nChipId: ",0);
  debug_out(esp_chipid,1);
  
  wdt_disable();
  wdt_enable(30000);// 30 sec 
  
  last_ping = millis();
  
  debug_out(F("Es geht los."),1);
}

void loop() 
{ 
  act_milli = millis();  
  
  wdt_reset();
  
  if ((act_milli-last_ping) > (10 * 1000)){
	sendData(data, host, port, url_wlan, F("application/json"), authstring);
	last_ping = act_milli;
  }
  
  int actualPirValue = digitalRead(pirPin);
  if (actualPirValue == 1){
    debug_out(F("BEWEGUNG BEWEGUNG BEWEGUNG: "),0);
    debug_out(String(act_milli / 1000),1);
    analogWrite(redpin, 250);
    sendData(data, host, port, url_motion, F("application/json"), authstring);
  }
  else{
    analogWrite(redpin, 0);
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
	
	if (retry_count == 20){
		analogWrite(redpin, 0);
		analogWrite(bluepin, 250);
	}
	else{
		analogWrite(bluepin, 0);
	}
	
	debug_out("",1);
  }
  else{
	  analogWrite(bluepin, 0);
  }
  
  yield();
}
