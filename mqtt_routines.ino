
//MQTT relevant routines

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("ESP8266Client")) {
      Serial.println("re-connected");  
      client.subscribe(topic);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {  //MQTT topic routine
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  int i = 0;

  for (i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String msgString = String(message_buff);

  Serial.println("Payload: " + msgString);
  Serial.println();
  Serial.println("-----------------------");


//Deal with different topics. Dusk, Sunset, Enabled, Offset,
 //Sunset
 if (strcmp(topic,"cmnd/Aviary/Sunset")==0) 
   {  if (msgString.equals("0")) 
   {  Sunset = 0;        Serial.print ("Sunset = "); Serial.println (Sunset);      }
      else if (msgString.equals("1")) 
      { Sunset = 1;      Serial.print ("Sunset = "); Serial.println (Sunset);      }
    //SunsetTime();
  }
  
  //Dusk 
  if (strcmp(topic,"cmnd/Aviary/Dusk")==0) 
  {     if (msgString.equals("0")) 
    { Dusk = 0;      Serial.print ("Dusk registered =  "); Serial.println (Dusk);    }
   else if (msgString.equals("1")) 
    { Dusk = 1;      Serial.print ("Dusk registered = "); Serial.println (Dusk);    }
//    DuskTime();
  }
  
  //Enabled
  if (strcmp(topic,"cmnd/Aviary/Enabled")==0) 
    {     if (msgString.equals("0")) { Serial.println ("Light switched off"); Enabled = 0;  }
     else if (msgString.equals("1")) { Serial.println ("Light switched on");  Enabled = 1;  }
//    LightONOFF();
    }
    
  if (strcmp(topic,"cmnd/Aviary/DimTime")==0) 
  {
    //convert string to INT
           if (msgString.equals("15")) {    DimMin = 15; } 
      else if (msgString.equals("30")) {    DimMin = 30; } 
      else if (msgString.equals("45")) {    DimMin = 45; } 
      else if (msgString.equals("60")) {    DimMin = 60; }
    Serial.print ("dimtime is "); Serial.print(DimMin); Serial.println ("minutes");
  }
  //LightONOFF();                             
}


void MQTTsetup() {
     // MQTT - this does not loop until connected - one attempt only, then falls through

    Serial.println("Connecting to MQTT...");
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");
      client.subscribe(topic);  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
    }
}




void MQTTupdate() {
      if (!client.connected()) 
      {   reconnect();      }
      MQTT_Print();

      int tempIllumlevel = int(IllumLevel/MaxIllum * 100);
      dtostrf(tempIllumlevel,0, 0, buffer);      client.publish("state/Aviary/IllumLevel", buffer); 
      dtostrf(Sunset,0, 0, buffer);              client.publish("state/Aviary/Sunset", buffer);
      dtostrf(Dusk,0, 0, buffer);                client.publish("state/Aviary/Dusk", buffer);
      dtostrf(LightState,0, 0, buffer);          client.publish("state/Aviary/Light", buffer);
      dtostrf(Enabled,0, 0, buffer);             client.publish("state/Aviary/Enabled", buffer);
      dtostrf(Enabled,0, 0, buffer);             client.publish("state/Aviary/DimTime", buffer);
}




