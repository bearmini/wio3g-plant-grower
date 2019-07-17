
void initMoistureSensor() {
  pinMode(MOISTURE_SENSOR_PIN, INPUT_ANALOG);
}

int measureMoisture() {
  int val = analogRead(MOISTURE_SENSOR_PIN);
  SerialUSB.print("Moisture sensor = ");
  SerialUSB.println(val);
  return val;
}
