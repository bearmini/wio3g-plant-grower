void initLightSensor() {
  pinMode(LIGHT_SENSOR_PIN, INPUT_ANALOG);
}

int measureLight() {
  int val = analogRead(LIGHT_SENSOR_PIN);
  SerialUSB.print("Light sensor = ");
  SerialUSB.println(val);
  return val;
}
