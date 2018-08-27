

void sensorRead(){
  readTime = millis();
 // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;  }
  reconnect();
  char buffer[10];
  dtostrf(t,0, 0, buffer);
  client.publish("state/Aviary/temperature",buffer);
  dtostrf(h,0, 0, buffer);
  client.publish("state/Aviary/humidity",buffer);
  }
