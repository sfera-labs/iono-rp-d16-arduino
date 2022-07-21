/*
 * IonoD16ModbusRtu.ino - Using Iono RP D16 as a Modbus RTU slave unit
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

#define IONORPD16_MODBUS_RTU_VERSION 0x0100

#include <IonoD16.h>
#include "config.h"
#include "modbus.h"
#include "hardware/watchdog.h"

void setup1() {
  Iono.setup();
}

void loop1() {
  Iono.process();
  delay(1);
}

void setup() {
  watchdog_enable(2000, 1);

  while (!Iono.ready()) ;

  loadConfig();

  modbusBegin(_cfgRegisters[MB_REG_CFG_OFFSET_MB_ADDR],
              _cfgRegisters[MB_REG_CFG_OFFSET_MB_BAUD],
              _cfgRegisters[MB_REG_CFG_OFFSET_MB_PARITY]);

  Iono.outputsJoin(D1, false);
  Iono.outputsJoin(D5, false);
  Iono.outputsJoin(D9, false);
  Iono.outputsJoin(D13, false);

  for (int d = D1; d <= D16; d++) {
    word mode = _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D1 + (d - D1)];
    switch (mode) {
      case 1:
        Iono.pinMode(d, INPUT, false);
        break;
      case 2:
        Iono.pinMode(d, INPUT, true);
        break;
      case 3:
        Iono.pinMode(d, OUTPUT_HS, false);
        break;
      case 4:
        Iono.pinMode(d, OUTPUT_HS, true);
        break;
      case 6:
        Iono.pinMode(d, OUTPUT_PP);
        break;
    }
  }

  for (int d = D2; d <= D16; d += 2) {
    word modePre = _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D1 + (d - D1) - 1];
    word mode = _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D1 + (d - D1)];
    if (mode == 5) {
      if (modePre == 3) {
        Iono.pinMode(d, OUTPUT_HS, false);
      } else if (modePre == 4) {
        Iono.pinMode(d, OUTPUT_HS, true);
      }
      Iono.outputsJoin(d);
    }
  }
}

void loop() {
  modbusProcess();

  if (configReset) {
    // let watchdog expire
    while(true) {
      delay(100);
    }
  }

  for (int d = 0; d < 16; d++) {
    if (_doEnabled[d] && millis() - _doStart[d] > _doTime[d]) {
      Iono.write(d + 1, LOW);
      _doEnabled[d] = false;
    }
  }

  watchdog_update();
}

void loadConfig() {
  if (!configRead(_cfgRegisters, MB_REG_CFG_OFFSET_MAX + 1)) {
    _cfgRegisters[MB_REG_CFG_OFFSET_MB_ADDR] = CFG_MB_UNIT_ADDDR;
    _cfgRegisters[MB_REG_CFG_OFFSET_MB_BAUD] = CFG_MB_BAUDRATE;
    _cfgRegisters[MB_REG_CFG_OFFSET_MB_PARITY] = CFG_MB_PARITY;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D1] = CFG_MODE_D1;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D2] = CFG_MODE_D2;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D3] = CFG_MODE_D3;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D4] = CFG_MODE_D4;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D5] = CFG_MODE_D5;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D6] = CFG_MODE_D6;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D7] = CFG_MODE_D7;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D8] = CFG_MODE_D8;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D9] = CFG_MODE_D9;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D10] = CFG_MODE_D10;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D11] = CFG_MODE_D11;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D12] = CFG_MODE_D12;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D13] = CFG_MODE_D13;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D14] = CFG_MODE_D14;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D15] = CFG_MODE_D15;
    _cfgRegisters[MB_REG_CFG_OFFSET_MODE_D16] = CFG_MODE_D16;
  }
}
