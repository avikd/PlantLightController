
// CONNECTIONS:
// DS1307 SDA --> SDA
// DS1307 SCL --> SCL
// DS1307 VCC --> 5v
// DS1307 GND --> GND

#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <Wire.h>  // must be incuded here so that Arduino library object file references work
#include <RtcDS1307.h>
#include <ESP8266WiFi.h>
#include "DHT.h"

#define DHTPIN D5     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321


RtcDS1307 Rtc;
DHT dht(DHTPIN, DHTTYPE);

int timeNow=0;

int relayPin = D0;

int updateDelay = 120000;
/*
45!! days in 12/12 : 0615 to 1815
10 in 13.5/10.5 : 0700 to 1730
10 in 15/9 : ??
*/

int onTime = 700;
int offTime = 1730;

int lightStatus = 0;

const char* ssid     = "myssid";
const char* password = "mypassword";

const char* host = "api.thingspeak.com";
const char* thingspeak_key = "mykey";


#define countof(a) (sizeof(a) / sizeof(a[0]))


void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.println(datestring);
}


void checkTime(const RtcDateTime& dt){
//  if ( ((dt.Hour() < 18) && ((dt.Hour() >= 6)) )  lightStatus = 1;

  timeNow = dt.Hour()*100 + dt.Minute();
  if ( (timeNow > onTime) && (timeNow < offTime))  lightStatus = 1;
  else lightStatus = 0;
  Serial.println(timeNow);
  }

 void uploadData(){
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

/*  String temp = String(dht.readTemperature());
  String humidity = String(dht.readHumidity());
  String voltage = String(system_get_free_heap_size());
  String url = "/update?key=";
  url += thingspeak_key;
  url += "&field1=";
  url += temp;
  url += "&field2=";
  url += humidity;
  url += "&field3=";
  url += voltage;

  */
//  String lightSensor = analogRead(A0);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  h = -1;t=-1;
  }
  int lightLevel = analogRead(A0);
  String lightVal = String(lightStatus);
  String url = "/update?key=";
  url += thingspeak_key;
  url += "&field1=";
  url += lightVal;
  url += "&field2=";
  url += timeNow;
  url += "&field3=";
  url += String(h);
  url += "&field4=";
  url += String(t);
  url += "&field5=";
  url += String(lightLevel);
  
//  url += "&field2=";
//  url += lightSensor;
//  url += "&field3=";
//  url += tempSensor;
//  url += "&field4=";
//  url += humiditySensor;
  

  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  }

void setup () 
{
    Serial.begin(115200);
     Serial.println("Started");
       dht.begin();
//--------RTC SETUP ------------
    Rtc.Begin();
    pinMode(relayPin,OUTPUT);
    pinMode(A0,INPUT);
    Serial.print("compiled: ");
//    Serial.print(__DATE__);
//    Serial.println(__TIME__);

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low); 
      WiFi.begin(ssid, password);
  
  while ((WiFi.status() != WL_CONNECTED) && (millis()<(200*1000))) {
    delay(500);
    Serial.print(".");
    Serial.println(millis());
  }
  
    
}

void loop () 
{
    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }

    RtcDateTime now = Rtc.GetDateTime();

//    printDateTime(now);

    checkTime(now);
    delay(1000);
    if (lightStatus == 1) digitalWrite(relayPin,HIGH);
      else digitalWrite(relayPin,LOW);
    Serial.println(lightStatus);

    delay(updateDelay); 
    
    uploadData();
}




