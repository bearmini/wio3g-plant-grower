uint32_t getStoredBaselineValue() {
  /*
  String v;
  uint32_t baseline;
  
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  delay(1000);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }
  v = getTagValue(Wio, "co2baseline");
  SerialUSB.print("co2baseline tag value == ");
  SerialUSB.println(v);
  baseline = strtol(v.c_str(), NULL, 16);
  return baseline;

err:
  SerialUSB.println("### Power supply OFF.");
  Wio.PowerSupplyCellular(false);
  */
  return 0;
}

uint32_t getCO2SensorBaseline() {
  uint32_t iaq_baseline = 0;
  if (sgp_get_iaq_baseline(&iaq_baseline) != STATUS_OK) {
    SerialUSB.println("sqg_get_iaq_baseline() failed.");
    return 0;
  }

  SerialUSB.print("Got iaq_baseline: ");
  SerialUSB.println(iaq_baseline,HEX);
  return iaq_baseline;
}

void setBaselineToCO2Sensor() {
  uint32_t baseline = getStoredBaselineValue();
  if (baseline > 0) {
    sgp_set_iaq_baseline(baseline);
    Serial.println(baseline,HEX);
  }
}



int initCO2Sensor() {
  int16_t err;
  uint16_t scaled_ethanol_signal, scaled_h2_signal;

  // Init module, reset all baseline, the initialization takes up to around 15 seconds,
  // during which all APIs measuring IAQ (Indoor air quality) output will not change.
  // Default value is 400(ppm) for co2, 0(ppb) for tvoc
  while (sgp_probe() != STATUS_OK) {
    SerialUSB.println("SGP failed");
    return -1;
  }
  
  // Read H2 and Ethanol signal in the way of blocking
  err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal, &scaled_h2_signal);
  if (err != STATUS_OK) {
    SerialUSB.println("error reading signals"); 
    return -1;
  }
  SerialUSB.println("get ram signal!");

  sgp_iaq_init();
  
  setBaselineToCO2Sensor();

  return 0;
}

int measureCO2() {
  int16_t err;
  uint16_t tvoc_ppb, co2_eq_ppm;
  
  err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
  if (err != STATUS_OK) {
    SerialUSB.println("error reading IAQ values");
    return 0;
  } 
 
  SerialUSB.print("tVOC  Concentration:");
  SerialUSB.print(tvoc_ppb);
  SerialUSB.println("ppb");

  SerialUSB.print("CO2eq Concentration:");
  SerialUSB.print(co2_eq_ppm);
  SerialUSB.println("ppm");
  return co2_eq_ppm;
}
