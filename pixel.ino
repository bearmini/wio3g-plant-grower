
const uint32_t MAGIC_OFFSET = -22;

void SysTickWait(uint32_t tick) {
  uint32_t start = SysTick->VAL;
  if (start < tick + 1000) {
    SysTick->VAL = 0;
    start = SysTick->VAL;  
  }
  while ((start - SysTick->VAL) < (tick + MAGIC_OFFSET));
}

const uint32_t grbOn[PIXEL_COUNT] = {
  0x808080, 0x808080, 0x808080, 0x808080, 0x808080,
  0x808080, 0x808080, 0x808080, 0x808080, 0x808080,
  0x808080, 0x808080, 0x808080, 0x808080, 0x808080,
  0x808080, 0x808080, 0x808080, 0x808080, 0x808080,
};

const uint32_t grbOff[PIXEL_COUNT] = {
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
};

// STM32F439VI on Wio3G is working at 180MHz. 1 clock == 1/180MHz == 5.55... nsec
const uint32_t T0H = 40;  // 220~380ns = 39.6~68.4 clocks
const uint32_t T0L = 100; // 580ns~1.6us = 104.4~288 clocks
const uint32_t T1H = 100; // 580ns~1.6us = 104.4~288 clocoks
const uint32_t T1L = 40;  // 220~420ns = 39.6~75.6 clocks
const uint32_t RES = 50400; // 280us = 50,400 clocks


void setPixel(const uint32_t* grb) {
  // https://datasheet.lcsc.com/szlcsc/Worldsemi-WS2813-Mini-WS2813-3535_C189639.pdf
  
  pinMode(PIXEL_PIN, OUTPUT);
  
  digitalWrite(PIXEL_PIN, LOW); // Reset
  SysTickWait(RES);

  //uint32_t history_hs[PIXEL_COUNT * 24];
  //uint32_t history_hm[PIXEL_COUNT * 24];
  //uint32_t history_he[PIXEL_COUNT * 24];
  //uint32_t history_ls[PIXEL_COUNT * 24];
  //uint32_t history_lm[PIXEL_COUNT * 24];
  //uint32_t history_le[PIXEL_COUNT * 24];

  __disable_irq();

  for (int i = 0; i < PIXEL_COUNT; i++) {
    uint32_t v = grb[i];
    for (uint32_t mask = 0x800000, j = 0; mask != 0; mask >>= 1, j++) {
      if ((v & mask) != 0) {
        //history_hs[i*24 + j] = SysTick->VAL;
        digitalWrite(PIXEL_PIN, HIGH);
        //history_hm[i*24 + j] = SysTick->VAL;
        SysTickWait(T1H);
        //history_he[i*24 + j] = SysTick->VAL;
        //history_ls[i*24 + j] = SysTick->VAL;
        digitalWrite(PIXEL_PIN, LOW);
        //history_lm[i*24 + j] = SysTick->VAL;
        SysTickWait(T1L);
        //history_le[i*24 + j] = SysTick->VAL;
      } else {
        //history_hs[i*24 + j] = SysTick->VAL;
        digitalWrite(PIXEL_PIN, HIGH);
        //history_hm[i*24 + j] = SysTick->VAL;
        SysTickWait(T0H);
        //history_he[i*24 + j] = SysTick->VAL;
        //history_ls[i*24 + j] = SysTick->VAL;
        digitalWrite(PIXEL_PIN, LOW);
        //history_lm[i*24 + j] = SysTick->VAL;
        SysTickWait(T0L);
        //history_le[i*24 + j] = SysTick->VAL;
      }
    }
  }

  __enable_irq();

  digitalWrite(PIXEL_PIN, LOW); // Reset
  SysTickWait(RES);


/*
  for (int i = 0; i < PIXEL_COUNT; i++) {
    for (int j = 0; j < 24; j++) {
      uint32_t hs = history_hs[i*24 + j];
      uint32_t hm = history_hm[i*24 + j];
      uint32_t he = history_he[i*24 + j];
      SerialUSB.print(hs);
      SerialUSB.print(" ");
      SerialUSB.print(hm);
      SerialUSB.print(" ");
      SerialUSB.print(he);
      SerialUSB.print(" ");
      SerialUSB.print(hs-he);
      SerialUSB.print(" ");
      SerialUSB.print(hm-he);
      SerialUSB.print(" ");
      uint32_t ls = history_ls[i*24 + j];
      uint32_t lm = history_lm[i*24 + j];
      uint32_t le = history_le[i*24 + j];
      SerialUSB.print(ls);
      SerialUSB.print(" ");
      SerialUSB.print(lm);
      SerialUSB.print(" ");
      SerialUSB.print(le);
      SerialUSB.print(" ");
      SerialUSB.print(ls-le);
      SerialUSB.print(" ");
      SerialUSB.print(lm-le);
      SerialUSB.println();
    }
    SerialUSB.println();
  }
*/
}

void turnOnPixel() {
  setPixel(grbOn);
}


void turnOffPixel() {
  setPixel(grbOff);
}

void initPixel() {
  turnOnPixel();
}
