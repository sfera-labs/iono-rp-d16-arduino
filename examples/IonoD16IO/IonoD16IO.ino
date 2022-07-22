/*
 * IonoD16IO.ino - Using I/Os on Iono RP D16
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

bool flip;

void setup1() {
  Iono.setup();
}

void loop1() {
  Iono.process();
  delay(1);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  // Make sure Iono.setup() is complete
  while (!Iono.ready()) ;
  
  Serial.println("Ready!");

  // Setup D1 as push-pull output
  if (!Iono.pinMode(D1, OUTPUT_PP)) {
    Serial.println("D1 setup error");
  }

  // Setup D2 as high-side output with open-load detection enabled
  if (!Iono.pinMode(D2, OUTPUT_HS, true)) {
    Serial.println("D2 setup error");
  }

  // Setup D3 as input with wire-break detection enabled
  if (!Iono.pinMode(D3, INPUT, true)) {
    Serial.println("D3 setup error");
  }

  // Setup D5 as high-side output
  if (!Iono.pinMode(D5, OUTPUT_HS)) {
    Serial.println("D5 setup error");
  }

  // Setup D6 as high-side output
  if (!Iono.pinMode(D6, OUTPUT_HS)) {
    Serial.println("D6 setup error");
  }

  // Join D5 and D6 to be used as single output
  if (!Iono.outputsJoin(D5)) {
    Serial.println("D5/D6 join error");
  }

  // Setup DT1 as output
  if (!Iono.pinMode(DT1, OUTPUT)) {
    Serial.println("DT1 setup error");
  }

  // Setup DT2 as input
  if (!Iono.pinMode(DT2, INPUT)) {
    Serial.println("DT2 setup error");
  }

  // Setup D4 as push-pull output 
  // with 1Hz frequency, 50% duty-cycle PWM
  if (!Iono.pinMode(D4, OUTPUT_PP)) {
    Serial.println("D4 setup error");
  }
  if (!Iono.pwmSet(D4, 1, 65535 / 2)) {
    Serial.println("D4 PWM setup error");
  }

  // Set callback function on D3 state change
  // with 100ms debounce
  Iono.subscribe(D3, 100, onDebounce);

  // Flip D2 on every low-to-high transition of D3
  // after a 50ms debounce
  Iono.link(D3, D2, LINK_FLIP_H, 50);

  Serial.println("I/O setup done.");
}

void loop() {
  delay(1000);
  Serial.println("----------");

  flip = !flip;
  Serial.println(flip ? "HIGH" : "LOW");

  Iono.ledSet(flip);

  if (!Iono.write(D1, flip ? HIGH : LOW)) {
    Serial.println("D1 write error");
  }
  // Writing D5 will drive D6 too
  if (!Iono.write(D5, flip ? HIGH : LOW)) {
    Serial.println("D5/D6 write error");
  }
  if (!Iono.write(DT1, flip ? HIGH : LOW)) {
    Serial.println("DT1 write error");
  }

  delay(10);

  for (int d = D1; d <= D6; d++) {
    Serial.print("D");
    Serial.print(d);
    Serial.print(" = ");
    Serial.print(Iono.read(d));
    Serial.print("\tWB = ");
    Serial.print(Iono.wireBreakRead(d));
    Serial.print("\tOL = ");
    Serial.print(Iono.openLoadRead(d));
    Serial.print("\tOV = ");
    Serial.print(Iono.overVoltageRead(d));
    Serial.print("\tOVL = ");
    Serial.print(Iono.overVoltageLockRead(d));
    Serial.print("\tTS = ");
    Serial.print(Iono.thermalShutdownRead(d));
    Serial.print("\tTSL = ");
    Serial.print(Iono.thermalShutdownLockRead(d));
    Serial.print("\tAT1 = ");
    Serial.print(Iono.alarmT1Read(d));
    Serial.print("\tAT2 = ");
    Serial.print(Iono.alarmT2Read(d));
    Serial.println();
  }

  Serial.print("DT1 = ");
  Serial.println(Iono.read(DT1));
  Serial.print("DT2 = ");
  Serial.println(Iono.read(DT2));
}

void onDebounce(int pin, int val) {
  Serial.print("D");
  Serial.print(pin);
  Serial.print(" debounce = ");
  Serial.println(val);
}
