/*
 * IonoRPD16WiegandRead.ino - Using Iono RP D16 as a Wiegand reader
 * 
 *   Copyright (C) 2022 Sfera Labs S.r.l. - All rights reserved.
 * 
 *   For information, see:
 *   http://www.sferalabs.cc/
 * 
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See file LICENSE.txt for further informations on licensing terms.
 * 
 */
 
#include <IonoD16.h>
#include <Wiegand.h>

#define PIN_D0 DT1
#define PIN_D1 DT2

void setup1() {
  Iono.setup();
}

void loop1() {
  Iono.process();
  delay(1);
}

Wiegand w(PIN_D0, PIN_D1);

uint64_t data;
int bits;
int noise;

char buff[20];

void onData0() {
  w.onData0();
}

void onData1() {
  w.onData1();
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  while (!Iono.ready()) ;

  w.setup(
    onData0,
    onData1,
    false,  // disable pins internal pull-up, use external
    700,    // min pulse interval (usec)
    2700,   // max pulse interval (usec)
    10,     // min pulse width (usec)
    150     // max pulse width (usec)
  );

  Serial.println("== Ready ==");
}

void loop() {
  bits = w.getData(&data);
  noise = w.getNoise();

  if (bits > 0) {
    Serial.print("Bits: ");
    Serial.println(bits);
    Serial.print("Data: ");
    println64bitHex(data);
    Serial.println();
  }
  if (noise != 0) {
    Serial.print("Noise: ");
    Serial.println(noise);
    Serial.println();
  }

  delay(100);
}

void println64bitHex(uint64_t val) {
  sprintf(buff, "0x%08lx%08lx",
    ((uint32_t) ((val >> 32) & 0xFFFFFFFF)),
    ((uint32_t) (val & 0xFFFFFFFF)));
  Serial.println(buff);
}
