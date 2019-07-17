#include <WioCellLibforArduino.h>
#include <stdio.h>
#include <stm32f4xx_hal.h>
#include <ArduinoJson.h>

#include "sensirion_common.h"
#include "sgp30.h"

#define PIXEL_PIN   (WIO_D38) // Digital IO pin connected to the NeoPixcel
#define PIXEL_COUNT (20)
#define LIGHT_SENSOR_PIN (WIO_A6)
#define MOISTURE_SENSOR_PIN (WIO_A4)
#define TEMP_SENSOR_PIN (WIO_D20)

const int RECEIVE_TIMEOUT = 10000;

const int LOOP_INTERVAL_MS = 10000;
int loopCount = 0;

const int REPORT_INTERVAL = 60; // LOOP_INTERVAL_MS * REPORT_INTERVAL = 10,000 * 60 = 600,000 millisec = 600 seconds = 10 minutes)
//const int REPORT_INTERVAL = 6; // LOOP_INTERVAL_MS * REPORT_INTERVAL = 10,000 * 6 = 60,000 millisec = 60 seconds = 1 minute : for debug)

typedef struct Measurement {
  float temperature;
  float humidity;
  int light;
  int moisture;
  int co2;
  uint32_t co2Baseline;
} Measurement;

typedef struct Response {
  bool ok;
  int light; // 0: off, 1: on
} Response;

Measurement measurements[REPORT_INTERVAL];

WioCellular Wio;

void putFloatBE(byte* p, size_t size, float f) {
  uint32_t* x;
  
  if (size < 4) {
    return;
  }
  
  x = (uint32_t*)(&f);
  p[0] = ((*x) & 0xff000000) >> 24;
  p[1] = ((*x) & 0x00ff0000) >> 16;
  p[2] = ((*x) & 0x0000ff00) >> 8;
  p[3] = ((*x) & 0x000000ff);
}

void putInt16BE(byte* p, size_t size, int16_t n) {
  if (size < 2) {
    return;
  }

  p[0] = (n & 0xff00) >> 8;
  p[1] = (n & 0x00ff);
}

void putUint32BE(byte* p, size_t size, uint32_t n) {
  if (size < 4) {
    return;
  }
  
  p[0] = (n & 0xff000000) >> 24;
  p[1] = (n & 0x00ff0000) >> 16;
  p[2] = (n & 0x0000ff00) >> 8;
  p[3] = (n & 0x000000ff);
}


void serialize(Measurement* m, byte* p, size_t size) {
  putFloatBE(p, size, m->temperature);
  p += 4; size -= 4;
  putFloatBE(p, size, m->humidity);
  p += 4; size -= 4;
  putInt16BE(p, size, m->light);
  p += 2; size -= 2;
  putInt16BE(p, size, m->moisture);
  p += 2; size -= 2;
  putInt16BE(p, size, m->co2);
  p += 2; size -= 2;
  putUint32BE(p, size, m->co2Baseline);
  p += 4; size -= 4;
}

void setup() {
  delay(200);

  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();

  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyGrove(true);
  delay(500);

  SerialUSB.println("Turn on LEDs");
  initPixel();

  SerialUSB.println("Turn on sensors");
  initTemperatureSensor();
  initLightSensor();
  initMoistureSensor();
  initCO2Sensor();
  
  SerialUSB.println("Setup completed.");
}

Measurement measure() {
  Measurement m = {};

  float mt, mh;
  bool ok = measureTemperatureAndHumidity(&mt, &mh);
  if (ok) {
    m.temperature = mt;
    m.humidity = mh;
  }

  m.light = measureLight();
  m.moisture = measureMoisture();
  m.co2 = measureCO2();
  m.co2Baseline = getCO2SensorBaseline();

  return m;
}

Measurement calcMeasurementsAverage(Measurement* measurements, int n) {
  Measurement m;
  m.temperature = 0.0;
  m.humidity = 0.0;
  m.light = 0;
  m.moisture = 0;
  m.co2 = 0;
  
  for (int i = 0; i < n; i++) {
    m.temperature += measurements[i].temperature;
    m.humidity += measurements[i].humidity;
    m.light += measurements[i].light;
    m.moisture += measurements[i].moisture;
    m.co2 += measurements[i].co2;
  }

  m.temperature /= n;
  m.humidity /= n;
  m.light /= n;
  m.moisture /= n;
  m.co2 /= n;
  m.co2Baseline = measurements[n-1].co2Baseline; // use the last one
  
  return m;
}

String hex(const byte* p, size_t size) {
  String s;
  char buf[4] = {};
  for (int i = 0; i < size; i++) {
    sprintf(buf, "%02X ", p[i]);
    s += buf;
  }
  return s;
}

Response report(Measurement* measurements, int n) {
  struct tm t;
  bool ok;
  Measurement m = calcMeasurementsAverage(measurements, n);
  DynamicJsonDocument doc(1024);
  DeserializationError de;
  byte data[1024] = {}; // send & receive buffer
  const byte* p = nullptr;
  
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  delay(1000);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }


  SerialUSB.println("### Connecting to \"soracom.io\".");
  if (!Wio.Activate("soracom.io", "sora", "sora")) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

  serialize(&m, data, sizeof(data));

  SerialUSB.println("### Open.");
  int connectId;
  connectId = Wio.SocketOpen("uni.soracom.io", 23080, WIO_UDP);
  if (connectId < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

  SerialUSB.println("### Send.");
  SerialUSB.print("Send:");
  SerialUSB.print(hex(data, sizeof(data)));
  SerialUSB.println("");
  if (!Wio.SocketSend(connectId, data, 18)) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

  SerialUSB.println("### Receive.");
  int length;
  length = Wio.SocketReceive(connectId, data, sizeof (data), RECEIVE_TIMEOUT);
  if (length < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }
  if (length == 0) {
    SerialUSB.println("### RECEIVE TIMEOUT! ###");
    goto err;
  }
  SerialUSB.print("Receive:");
  SerialUSB.print((const char*)data);
  SerialUSB.println("");

  p = data;
  if (data[0] != '{') {
    p = &data[4]; // skip http status code (e.g. "200") and a white space)
  }
  de = deserializeJson(doc, p);
  if (de) {
    SerialUSB.println("### Deserialization error ###");
    SerialUSB.println(de.c_str());
    goto err;
  }

  Response res;
  res.ok = false;
    
  if (doc["statusCode"].as<int>() == 200) {
    SerialUSB.println("Received ok response.");
    res.ok = true;
    res.light = doc["body"]["light"].as<int>();  
    SerialUSB.print("light == ");
    SerialUSB.println(res.light);
  }
  
  SerialUSB.println("### Power supply OFF.");
  Wio.PowerSupplyCellular(false);
  return res;
  
err:
  SerialUSB.println("### Power supply OFF.");
  Wio.PowerSupplyCellular(false);
  res.ok = false;
  return res;
}


void loop() {
  SerialUSB.print("Loop #");
  SerialUSB.println(loopCount);
  
  Measurement m = measure();
  measurements[loopCount] = m;

  loopCount++;

  if (loopCount >= REPORT_INTERVAL) {
    loopCount = 0;
    Response res = report(measurements, REPORT_INTERVAL);
    SerialUSB.print("res.ok == ");
    SerialUSB.println(res.ok);
    SerialUSB.print("res.light == ");
    SerialUSB.println(res.light);
    if (res.ok) {
      if (res.light == 0) {
        turnOffPixel();
      } else {
        turnOnPixel();
      }
    }
  }
  
  delay(LOOP_INTERVAL_MS);
}
