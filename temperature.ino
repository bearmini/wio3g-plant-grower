void initTemperatureSensor() {
  DHT11Init(TEMP_SENSOR_PIN);
}

bool measureTemperatureAndHumidity(float* t, float* h) {
  byte data[5];
  
  DHT11Start(TEMP_SENSOR_PIN);
  for (int i = 0; i < 5; i++) {
    data[i] = DHT11ReadByte(TEMP_SENSOR_PIN);
  }
  DHT11Finish(TEMP_SENSOR_PIN);
  
  if(!DHT11Check(data, sizeof (data))) {
    SerialUSB.println("DHT11 check failed");
    return false;
  }
  if (data[1] >= 10) {
    SerialUSB.println("DHT11 data[1] >= 10");
    return false;
  }
  if (data[3] >= 10) {
    SerialUSB.println("DHT11 data[3] >= 10");
    return false;
  }

  *h = (float)data[0] + (float)data[1] / 10.0f;
  *t = (float)data[2] + (float)data[3] / 10.0f;

  SerialUSB.print("Temperature sensor = ");
  SerialUSB.println(*t);
  SerialUSB.print("Humidity sensor = ");
  SerialUSB.println(*h);
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//

void DHT11Init(int pin) {
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

void DHT11Start(int pin) {
  // Host the start of signal
  digitalWrite(pin, LOW);
  delay(18);
  
  // Pulled up to wait for
  pinMode(pin, INPUT);
  waitForHigh(pin);
  
  // Response signal
  waitForLow(pin);
    
  // Pulled ready to output
  //while (!digitalRead(pin)) ;
  waitForHigh(pin);
}

byte DHT11ReadByte(int pin) {
  byte data = 0;
  
  for (int i = 0; i < 8; i++) {
    //while (digitalRead(pin)) ;
    waitForLow(pin);
    
    //while (!digitalRead(pin)) ;
    waitForHigh(pin);
    unsigned long start = micros();

    //while (digitalRead(pin)) ;
    waitForLow(pin);
    unsigned long finish = micros();

    if ((unsigned long)(finish - start) > 50) data |= 1 << (7 - i);
  }
  
  return data;
}

void DHT11Finish(int pin) {
  // Releases the bus
  //while (!digitalRead(pin)) ;
  waitForHigh(pin);
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

bool DHT11Check(const byte* data, int dataSize) {
  if (dataSize != 5) return false;

  byte sum = 0;
  for (int i = 0; i < dataSize - 1; i++) {
    sum += data[i];
  }

  return data[dataSize - 1] == sum;
}

void waitForLow(int pin) {
  waitForDigitalPinStateWithTimeout(pin, LOW, 1000);
}

void waitForHigh(int pin) {
  waitForDigitalPinStateWithTimeout(pin, HIGH, 1000);
}

void waitForDigitalPinStateWithTimeout(int pin, int state, int timeout_us) {
  unsigned long start = micros();
  while (digitalRead(pin) != state) {
    if (micros() > (start + timeout_us)) {
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
