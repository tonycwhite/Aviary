#include <ArduinoOTA.h>
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "Adafruit_IO_Client.h"
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <DHT.h>



// wifi credentials
#define WLAN_SSID       "Whitehouse"
#define WLAN_PASS       "knocker1"


//mqtt information
const char* mqttServer = "home-assistant";
const int mqttPort = 1883;
const char* mqttUser = "tony";
const char* mqttPassword = "knocker1";
const char* mqttpayload;
const char* topic = "cmnd/Aviary/#";   //command topic
const char* topic2 = "state/Aviary/#"; //state topic

char message_buff[100];  // somewhere to stow messages
char buffer[10];

WiFiClient espClient;
PubSubClient client(espClient);
 
/* GPIO USAGE
 * * D0 - GPIO 16  
 * D1 - GPIO 5 - Light PWM output
 * D2 - GPIO 4 - 
 * D3 - GPIO 0 - 
 * D4 - GPIO 2 - 
 * D5 - GPIO 14 -
  */
float MaxIllum = 1093;  // maximum PWM value
float MinIllum = 0;    // Min Illum
float Brightness = 1; // float value for storing brightness

#define Light 5 //light connectied to D1 GPIO5

int Sunset  ;  // variables to put Sun state in. 0 = Nighttime 1 = Daytime
int IllumLevel; // variable to place PWM value
int Dusk; // Time to switch light on
int LightState; //1 = on, 0 = off
int Enabled; // 1 - on, 0 = off
int DimMin; // Dimming time in Minutes - used to calculate delay
int stepdelay; //delay between steps for fade
int tempDusk = 0;     //temporary stores for comparison
int tempEnabled = 0;
int tempSunset = 0;





//Temperature monitoring
#define humidity_topic "Aviary/humidity"
#define temperature_topic "Aviary/temperature"
#define DHTPIN 4
#define DHTTYPE DHT11 //21 or 22 also an option
DHT dht(DHTPIN, DHTTYPE);
unsigned long readTime = 0;






void setup() {
  Serial.begin(9600); /* begin serial for debug */
  wifi_init();
  OTAinit();
  pinMode(DHTPIN, INPUT_PULLUP);  
  digitalWrite(Light, 0);  //switch light off
  Sunset = 0;   // At sunset the level rises to 1
  Dusk = 0;
  LightState = 0;
  Enabled = 0;
  DimMin =30;

  MQTTsetup();
  MQTTupdate();
  Serial.println ("Setup Complete");
}


void loop() {
  //calculate delay
  stepdelay = (((DimMin*60)*1000)/MaxIllum);
  
  ArduinoOTA.handle();    // look for OTA
  if (!client.connected()) 
     {    reconnect();  }

  if(millis() > readTime+60000)    //check if 60 seconds has elapsed since the last time we read the sensors. 
    {   sensorRead();  }
         
  client.loop();          // Look for MQTT topics  - Status are loaded but no actions are taken by the routine.
  
  // run comparison to see if any changes have happened thru MQTT
  if (tempDusk    != Dusk)    { 
    tempDusk = Dusk;
      if (Dusk == 0)
      { DuskOff();    }
      else 
      { DuskOn() ;    }  
  }
  if (tempSunset  != Sunset)   { 
    tempSunset = Sunset;
      if (Sunset == 0) 
      { SunsetOff();  }
      else 
      { SunsetOn() ;  }  
  }     
  
  if (tempEnabled != Enabled) { 
    tempEnabled = Enabled;
      if (Enabled == 1)
       { LightON(); }
      else 
      {  LightOFF();}
   }
}





void DuskOn() {
    dtostrf(Dusk,0, 0, buffer);
    client.publish("state/Aviary/Dusk", buffer);  //update to reflect we are in the routine
    
    //Main fade in sequence    
    for (int i = MinIllum; i <= MaxIllum; i++)
    {      
      analogWrite(Light, i);
      IllumLevel = i;
       // due to length of loop, we need to occasionaly check for new override commands via MQTT
      if(millis() > readTime+5000)
       {   
        reconnect();
        client.loop();
        if ((tempDusk != Dusk) || (tempEnabled != Enabled)  || (tempSunset != Sunset))
          {  break;      }
       }

      int tempIllumlevel = int(IllumLevel/MaxIllum * 100);
      if (tempIllumlevel % 5 == 0)//update every 5%
         { 
         reconnect();
         dtostrf(tempIllumlevel,0, 0, buffer);
         client.publish("state/Aviary/IllumLevel", buffer);     
         } 
         
      delay(stepdelay/10);
       if (tempIllumlevel == 100)
          { Enabled = 1;  }
      }
    }



void DuskOff(){
// only other relevant routine is if Dusk gets cancelled
      //Serial.println ("light off routine");  
      digitalWrite(Light, 0);
      IllumLevel = MinIllum;
      reconnect();
      MQTTupdate();
      LoadVar();    
  }


//Sunset On Routine - dim light
void SunsetOn()
    {
    //Serial.println ("Sunset On Routine");
    client.publish("state/Aviary/Sunset", buffer); //publish sunset routine
    for (int i = IllumLevel; i >= MinIllum; i--){      //dim lights  using the illumlevel in case the routine is broken into from Dusk illum rise
      analogWrite(Light, i);
      IllumLevel = i;

      if(millis() > readTime+5000)      
        {   
          reconnect();
          client.loop();
          if ((tempDusk != Dusk) || (tempEnabled != Enabled)  || (tempSunset != Sunset))
            {
            break;
            }
         }
     
      int tempIllumlevel = int(IllumLevel/MaxIllum * 100);
      if (tempIllumlevel % 5 == 0)
         {
         reconnect();
         dtostrf(tempIllumlevel,0, 0, buffer);
         client.publish("state/Aviary/IllumLevel", buffer);     
         }
      delay(stepdelay/10);
    }             
    Serial.println ("end of dimming ");
    //mqtt publish states
    if (IllumLevel == 0){
      Enabled =0;
    }
    //Enabled = 0;  //set lights off variable
    Sunset = 0;
    Dusk = 0;
    MQTTupdate();
  }


//Sunset Off Routine
void SunsetOff()
    {
    Sunset = 0;
    MQTTupdate();  
   }

void LightON(){  
    digitalWrite(Light, 1);
    IllumLevel = MaxIllum;
    Dusk = 1; // as light is on, Dusk is also 1, to ensure sunset will commence.
    Sunset = 0;
    Enabled = 1;
    LoadVar();
    MQTTupdate();
    } 

void LightOFF()     
    { 
    digitalWrite(Light, 0);
    IllumLevel = MinIllum;
    reconnect();
    Dusk = 0;
    Sunset = 0;
    Enabled = 0;
    MQTTupdate();
    LoadVar();  
    } 


void LoadVar() {
  //Load variables with current values
   tempDusk = Dusk;     //temporary stores for comparison
   tempEnabled = Enabled;
   tempSunset = Sunset;
}



void MQTT_Print() {
    Serial.println ("Flag states ");
    Serial.print ("Illum Level = ");  Serial.println (IllumLevel) ; 
    Serial.print ("Sunset State");    Serial.println (Sunset);
    Serial.print ("dusk State");      Serial.println (Dusk);       
    Serial.print ("Lightstate =");    Serial.println (LightState);
    Serial.print ("Enabled State ="); Serial.println (Enabled);
    Serial.print ("Dim Time = ");     Serial.println (DimMin);
}







