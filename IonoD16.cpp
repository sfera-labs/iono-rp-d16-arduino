/*
  IonoD16.cpp - Iono RP base library

    Copyright (C) 2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    http://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "IonoD16.h"
#include <Arduino.h>
#include <Wire.h>

#define MAX22190_REG_WB 0x00
#define MAX22190_REG_FAULT1 0x04
#define MAX22190_REG_FAULT2 0x1C
#define MAX22190_REG_FLT1 0x06
#define MAX22190_REG_FAULT2EN 0x1E

#define MAX14912_REG_IN 0
#define MAX14912_REG_PP 1
#define MAX14912_REG_OL_EN 2
#define MAX14912_REG_WD_JN 3
#define MAX14912_REG_OL 4
#define MAX14912_REG_THSD 5
#define MAX14912_REG_FAULTS 6
#define MAX14912_REG_OV 7

#define _MAX22190_IDX_L 0
#define _MAX22190_IDX_H 1
#define _MAX14912_IDX_L 0
#define _MAX14912_IDX_H 1

#define _MAX14912_CMD_SET_STATE 0b0
#define _MAX14912_CMD_SET_MODE 0b1
#define _MAX14912_CMD_SET_OL_DET 0b10
#define _MAX14912_CMD_SET_CONFIG 0b11
#define _MAX14912_CMD_READ_REG 0b100000
#define _MAX14912_CMD_READ_RT_STAT 0b110000

#define _PROT_OV_LOCK_MS 10000
#define _PROT_THSD_LOCK_MS 10000

IonoD16Class::IonoD16Class() {
}

bool IonoD16Class::_getBit(byte source, int bitIdx) {
  return ((source >> bitIdx) & 1) == 1;
}

void IonoD16Class::_setBit(byte* target, int bitIdx, bool val) {
  byte mask = 1 << bitIdx;
  *target = (*target & ~mask) | (val ? mask : 0);
}

void IonoD16Class::_spiTransaction(
      int cs, byte d2, byte d1, byte d0, byte* r2, byte* r1, byte* r0) {
#ifdef IONO_DEBUG
  Serial.print(">>> ");
  Serial.println(cs);
  Serial.println(d2, HEX);
  Serial.println(d1, HEX);
  Serial.println(d0, HEX);
#endif

  ::digitalWrite(cs, LOW);

  SPI.beginTransaction(_spiSettings);
  *r2 = SPI.transfer(d2);
  *r1 = SPI.transfer(d1);
  *r0 = SPI.transfer(d0);
  SPI.endTransaction();

  ::digitalWrite(cs, HIGH);

#ifdef IONO_DEBUG
  Serial.print("<<< ");
  Serial.println(cs);
  Serial.println(*r2, HEX);
  Serial.println(*r1, HEX);
  Serial.println(*r0, HEX);
#endif
}

// MAX22190 =======================

byte IonoD16Class::_max22190Crc(byte data2, byte data1, byte data0) {
  int length = 19; // 19-bit data
  byte crc_init = 0x07; // 5-bit init word, constant, 00111
  byte crc_poly = 0x35; // 6-bit polynomial, constant, 110101
  byte crc_step;
  byte tmp;

  uint32_t datainput = (unsigned long)((data2 << 16) + (data1 << 8) + data0);
  datainput = (datainput & 0xffffe0) + crc_init;
  tmp = (byte)((datainput & 0xfc0000) >> 18);
  if ((tmp & 0x20) == 0x20)
    crc_step = (byte)(tmp ^ crc_poly);
  else
    crc_step = tmp;

  for (int i = 0; i < length - 1; i++) {
    tmp = (byte)(((crc_step & 0x1f) << 1) + ((datainput >> (length - 2 - i)) & 0x01));
    if ((tmp & 0x20) == 0x20)
      crc_step = (byte)(tmp ^ crc_poly);
    else
      crc_step = tmp;
  }

  return (byte)(crc_step & 0x1f);
}

bool IonoD16Class::_max22190SpiTransaction(struct max22190Str* m, byte* data1, byte* data0) {
  byte r1, r0, rcrc;
  byte crc = _max22190Crc(*data1, *data0, 0);
  bool ok = false;
  for (int i = 0; i < 3; i++) {
    mutex_enter_blocking(&_spiMtx);
    _spiTransaction(m->pinCs, *data1, *data0, crc, &r1, &r0, &rcrc);
    mutex_exit(&_spiMtx);
    if ((rcrc & 0x1f) == _max22190Crc(r1, r0, rcrc)) {
      *data1 = r1;
      *data0 = r0;
      ok = true;
      break;
    }
#ifdef IONO_DEBUG
    Serial.println("repeat");
#endif
  }
  return ok;
}

bool IonoD16Class::_max22190ReadReg(byte regAddr, struct max22190Str* m, byte* data) {
  byte data1 = regAddr;
  byte data0 = 0;
  if (_max22190SpiTransaction(m, &data1, &data0)) {
    m->error = false;
    m->inputs = data1;
    *data = data0;
    return true;
  }
  m->error = true;
  return false;
}

bool IonoD16Class::_max22190WriteReg(byte regAddr, struct max22190Str* m, byte data0) {
  byte data1 = 0x80 | regAddr;
  return _max22190SpiTransaction(m, &data1, &data0);
}

bool IonoD16Class::_max22190GetByPin(int pin, struct max22190Str** m, int* inIdx) {
  if (pin >= D1 && pin <= D8) {
    *m = &_max22190[_MAX22190_IDX_L];
    if (inIdx) {
      *inIdx = 8 - pin;
    }
    return true;
  } else if (pin >= D9 && pin <= D16) {
    *m = &_max22190[_MAX22190_IDX_H];
    if (inIdx) {
      *inIdx = 16 - pin;
    }
    return true;
  }
  return false;
}

// MAX14912 =======================

byte IonoD16Class::_max14912CrcLoop(byte crc, byte byte1) {
  for (int i = 0; i < 8; i++) {
    crc <<= 1;
    if (crc & 0x80)
      crc ^= 0xB7; // 0x37 with MSBit on purpose
    if (byte1 & 0x80)
      crc ^=1;
    byte1 <<= 1;
  }
  return crc;
}

byte IonoD16Class::_max14912Crc(byte byte1, byte byte2) {
  byte synd;
  synd = _max14912CrcLoop(0x7f, byte1);
  synd = _max14912CrcLoop(synd, byte2);
  return _max14912CrcLoop(synd, 0x80) & 0x7f;
}

bool IonoD16Class::_max14912SpiTransaction(bool wr, struct max14912Str* m, byte* data1, byte* data0) {
  byte r1, r0, rcrc;
  byte zBit = m->clearFaults ? 0x80 : 0x00;
  byte crc = _max14912Crc(zBit | *data1, *data0);
  bool ok = false;
  for (int i = 0; i < 3; i++) {
#ifdef IONO_DEBUG
    if (i != 0) {
      Serial.println("repeat");
    }
#endif
    mutex_enter_blocking(&_spiMtx);
    _spiTransaction(m->pinCs, zBit | *data1, *data0, crc, &r1, &r0, &rcrc);
#ifdef IONO_DEBUG
    if ((rcrc & 0x80) == 0x80) {
      Serial.println("CRC err 1");
    }
#endif
    if (wr) {
      _spiTransaction(m->pinCs, _MAX14912_CMD_READ_RT_STAT, 0, _max14912ReadStatCrc, &r1, &r0, &rcrc);
      mutex_exit(&_spiMtx);
      if ((rcrc & 0x80) == 0x80) {
#ifdef IONO_DEBUG
        Serial.println("CRC err 2");
#endif
        continue;
      }
    } else {
      mutex_exit(&_spiMtx);
    }
    if ((rcrc & 0x7f) == _max14912Crc(r1, r0)) {
      *data1 = r1;
      *data0 = r0;
      ok = true;
      break;
    }
  }
  if (ok && zBit == 0x80) {
    m->clearFaults = false;
  }
  return ok;
}

bool IonoD16Class::_max14912ReadReg(byte regAddr, struct max14912Str* m, byte* dataA, byte* dataQ) {
  byte data1 = _MAX14912_CMD_READ_REG;
  byte data0 = regAddr;
  if (_max14912SpiTransaction(true, m, &data1, &data0)) {
    m->error = false;
    if (dataA) {
      *dataA = data1;
    }
    if (dataQ) {
      *dataQ = data0;
    }
    return true;
  }
  m->error = true;
  return false;
}

bool IonoD16Class::_max14912Cmd(byte cmd, struct max14912Str* m, byte data) {
  byte data1 = cmd;
  byte data0 = data;
  return _max14912SpiTransaction(false, m, &data1, &data0);
}

bool IonoD16Class::_max14912Config(struct max14912Str* m, byte cmd, byte checkRegAddr, byte val) {
  byte cfg;
  if (!_max14912Cmd(cmd, m, val)) {
    return false;
  }
  if (!_max14912ReadReg(checkRegAddr, m, NULL, &cfg)) {
    return false;
  }
  if (cfg != val) {
    return false;
  }
  return true;
}

bool IonoD16Class::_max14912GetByPin(int pin, struct max14912Str** m, int* outIdx) {
  if (pin >= D1 && pin <= D8) {
    *m = &_max14912[_MAX14912_IDX_L];
    if (outIdx) {
      *outIdx = pin - 1;
    }
    return true;
  } else if (pin >= D9 && pin <= D16) {
    *m = &_max14912[_MAX14912_IDX_H];
    if (outIdx) {
      *outIdx = pin - 9;
    }
    return true;
  }
  return false;
}

bool IonoD16Class::_max14912OutputSet(struct max14912Str* m, int outIdx, bool val) {
  _setBit(&m->outputs, outIdx, val);
  return _max14912Cmd(_MAX14912_CMD_SET_STATE, m, m->outputs);
}

bool IonoD16Class::_max14912ModePPSet(struct max14912Str* m, int outIdx, bool val) {
  _setBit(&m->cfgModePP, outIdx, val);
  return _max14912Config(m, _MAX14912_CMD_SET_MODE, MAX14912_REG_PP, m->cfgModePP);
}

void IonoD16Class::_max14912OverVoltProt(struct max14912Str* m) {
  for (int oi = 0; oi < 8; oi++) {
    if (_getBit(m->ovRT, oi) &&
        !_getBit(m->cfgModePP, oi) &&
        (!_getBit(m->outputs, oi) || _getBit(m->ovLock, oi)) ) {
      _setBit(&m->ovLock, oi, true);
      if (!_getBit(m->outputs, oi)) {
        _max14912OutputSet(m, oi, true);
      }
      m->lockTs[oi] = millis();
    } else if (_getBit(m->ovLock, oi)) {
      if (millis() - m->lockTs[oi] > _PROT_OV_LOCK_MS) {
        _max14912OutputSet(m, oi, _getBit(m->outputsUser, oi));
        _setBit(&m->ovLock, oi, false);
      }
    }
  }
}

void IonoD16Class::_max14912ThermalProt(struct max14912Str* m) {
  for (int oi = 0; oi < 8; oi++) {
    if (_getBit(m->thsdRT, oi) &&
        (_getBit(m->cfgModePP, oi) || _getBit(m->thsdLock, oi)) &&
        (!_getBit(m->outputs, oi) || _getBit(m->thsdLock, oi)) ) {
      _setBit(&m->thsdLock, oi, true);
      if (_getBit(m->cfgModePP, oi)) {
        _max14912ModePPSet(m, oi, false);
      }
      if (_getBit(m->outputs, oi)) {
        _max14912OutputSet(m, oi, false);
      }
      m->lockTs[oi] = millis();
    } else if (_getBit(m->thsdLock, oi)) {
      if (millis() - m->lockTs[oi] > _PROT_THSD_LOCK_MS) {
        _max14912ModePPSet(m, oi, _getBit(m->cfgModePPUser, oi));
        _max14912OutputSet(m, oi, _getBit(m->outputsUser, oi));
        _setBit(&m->thsdLock, oi, false);
      }
    }
  }
}

// ==================================

bool IonoD16Class::_pinModeInput(int pin, bool wbol) {
  struct max22190Str* m;
  int inIdx;
  if (!_max22190GetByPin(pin, &m, &inIdx)) {
    return false;
  }
  byte regAddr = MAX22190_REG_FLT1 + (inIdx * 2);
  m->cfgFlt[inIdx] = (m->cfgFlt[inIdx] & 0x0f) | (wbol ? 0x10 : 0x00);
  return _max22190WriteReg(regAddr, m, m->cfgFlt[inIdx]);
}

bool IonoD16Class::_pinModeOutputProtected(int pin, int mode, bool wbol) {
  struct max14912Str* m;
  int outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return false;
  }
  _setBit(&m->cfgOlDet, outIdx, wbol);
  if (!_max14912Config(m, _MAX14912_CMD_SET_OL_DET, MAX14912_REG_OL_EN, m->cfgOlDet)) {
    return false;
  }
  bool modePP = mode == OUTPUT_PP;
  _setBit(&m->cfgModePPUser, outIdx, modePP);
  if (_getBit(m->thsdLock, outIdx) && modePP) {
    return false;
  }
  return _max14912ModePPSet(m, outIdx, modePP);
}

bool IonoD16Class::_writeOutputProtected(int pin, int val) {
  struct max14912Str* m;
  int outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return false;
  }
  val = val == HIGH;
  _setBit(&m->outputsUser, outIdx, val);
  if (_getBit(m->ovLock, outIdx) && !val) {
    return false;
  }
  return _max14912OutputSet(m, outIdx, val);
}

bool IonoD16Class::_outputsJoinable(int pin) {
  int idx = pin - 1;
  int base4 = (idx / 4) * 4;
  int mod4 = idx % 4;
  int idxHs1;
  int idxHs2;
  int idxHsOrIn1;
  int idxHsOrIn2;
  if (mod4 <= 1) {
    idxHs1 = base4;
    idxHs2 = base4 + 1;
    idxHsOrIn1 = base4 + 2;
    idxHsOrIn2 = base4 + 3;
  } else {
    idxHsOrIn1 = base4;
    idxHsOrIn2 = base4 + 1;
    idxHs1 = base4 + 2;
    idxHs2 = base4 + 3;
  }
  if (_pinMode[idxHs1] != OUTPUT_HS) {
    return false;
  }
  if (_pinMode[idxHs2] != OUTPUT_HS) {
    return false;
  }
  if (_pinMode[idxHsOrIn1] != OUTPUT_HS && _pinMode[idxHsOrIn1] != INPUT) {
    return false;
  }
  if (_pinMode[idxHsOrIn2] != OUTPUT_HS && _pinMode[idxHsOrIn2] != INPUT) {
    return false;
  }
  return true;
}

void IonoD16Class::_subscribeProcess(struct subscribeStr* s) {
  int val = read(s->pin);
  unsigned long ts = millis();
  if (s->value != val) {
    if ((ts - s->lastTs) >= s->debounceMs) {
      s->value = val;
      s->lastTs = ts;
      s->cb(s->pin, val);
    }
  } else {
    s->lastTs = ts;
  }
}

void IonoD16Class::_linkProcess(struct linkStr* l) {
  if (l->inPin == 0 || l->outPin == 0 || l->mode == LINK_NONE) {
    return;
  }
  int val = read(l->inPin);
  unsigned long ts = millis();
  if (l->value != val) {
    if ((ts - l->lastTs) >= l->debounceMs) {
      l->value = val;
      l->lastTs = ts;
      switch (l->mode) {
        case LINK_FOLLOW:
          write(l->outPin, val);
          break;
        case LINK_INVERT:
          write(l->outPin, val == HIGH ? LOW : HIGH);
          break;
        case LINK_FLIP_T:
          flip(l->outPin);
          break;
        case LINK_FLIP_H:
          if (val == HIGH) {
            flip(l->outPin);
          }
          break;
        case LINK_FLIP_L:
          if (val == LOW) {
            flip(l->outPin);
          }
          break;
      }
    }
  } else {
    l->lastTs = ts;
  }
}

void IonoD16Class::_ledCtrl(bool on) {
  byte x;
  mutex_enter_blocking(&_spiMtx);
  ::digitalWrite(IONO_PIN_CS_DOL, on ? LOW : HIGH);
  ::digitalWrite(IONO_PIN_CS_DIH, LOW);
  _spiTransaction(IONO_PIN_CS_DIL, 0, 0, 0, &x, &x, &x);
  ::digitalWrite(IONO_PIN_CS_DIH, HIGH);
  ::digitalWrite(IONO_PIN_CS_DOL, HIGH);
  mutex_exit(&_spiMtx);
}

// Public ==========================

bool IonoD16Class::setup() {
  ::pinMode(IONO_PIN_CS_DOL, OUTPUT);
  ::pinMode(IONO_PIN_CS_DOH, OUTPUT);
  ::pinMode(IONO_PIN_CS_DIL, OUTPUT);
  ::pinMode(IONO_PIN_CS_DIH, OUTPUT);

  ::digitalWrite(IONO_PIN_CS_DOL, HIGH);
  ::digitalWrite(IONO_PIN_CS_DOH, HIGH);
  ::digitalWrite(IONO_PIN_CS_DIL, HIGH);
  ::digitalWrite(IONO_PIN_CS_DIH, HIGH);

  ::pinMode(IONO_PIN_MAX14912_WD_EN, OUTPUT);
  ::digitalWrite(IONO_PIN_MAX14912_WD_EN, HIGH);

  ::pinMode(IONO_PIN_RS485_TXEN_N, OUTPUT);
  rs485TxEn(false);

  IONO_RS485.setRX(IONO_PIN_RS485_RX);
  IONO_RS485.setTX(IONO_PIN_RS485_TX);
  IONO_RS485.begin(9600);
  IONO_RS485.end();

  SPI.setRX(IONO_PIN_SPI_RX);
  SPI.setTX(IONO_PIN_SPI_TX);
  SPI.setSCK(IONO_PIN_SPI_SCK);
  SPI.begin();
  _spiSettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);

  Wire.setSDA(IONO_PIN_I2C_SDA);
  Wire.setSCL(IONO_PIN_I2C_SCL);
  Wire.begin();

  _max22190[_MAX22190_IDX_L].pinCs = IONO_PIN_CS_DIL;
  _max22190[_MAX22190_IDX_H].pinCs = IONO_PIN_CS_DIH;
  _max14912[_MAX14912_IDX_L].pinCs = IONO_PIN_CS_DOL;
  _max14912[_MAX14912_IDX_H].pinCs = IONO_PIN_CS_DOH;

  _max14912ReadStatCrc = _max14912Crc(_MAX14912_CMD_READ_RT_STAT, 0);

  mutex_init(&_spiMtx);

  _max22190WriteReg(MAX22190_REG_FAULT2EN, &_max22190[_MAX22190_IDX_L], 0x3f);
  _max22190WriteReg(MAX22190_REG_FAULT2EN, &_max22190[_MAX22190_IDX_H], 0x3f);

  _ledSet = true;
  _ledVal = false;

  _setupDone = true;

  process();

  return true;
}

bool IonoD16Class::ready() {
  return _setupDone;
}

void IonoD16Class::process() {
  int i, j;
  unsigned long ts, dts;
  struct max22190Str* mi;
  struct max14912Str* mo;

  if (!_setupDone) {
    return;
  }

  // Devices are read alternately to avoid delays between
  // subsequent SPI cycles

  // WB is read always to update the inputs state
  for (i = 0; i < _MAX22190_NUM; i++) {
    mi = &_max22190[i];
    _max22190ReadReg(MAX22190_REG_WB, mi, &mi->wb);
    mi->faultMemWb |= mi->wb;
  }

  if (millis() - _processTs > 20) {
    switch (_processStep++) {
      case 0:
        for (i = 0; i < _MAX22190_NUM; i++) {
          mi = &_max22190[i];
          _max22190ReadReg(MAX22190_REG_FAULT1, mi, &mi->fault1);
          mi->faultMemAlrmT1 |= _getBit(mi->fault1, 3) ? 0xff : 0x00;
          mi->faultMemAlrmT2 |= _getBit(mi->fault1, 4) ? 0xff : 0x00;
        }
        if (_getBit(mi->fault1, 5)) {
          for (i = 0; i < _MAX22190_NUM; i++) {
            mi = &_max22190[i];
            _max22190ReadReg(MAX22190_REG_FAULT2, mi, &mi->fault2);
            mi->faultMemOtshdn |= _getBit(mi->fault2, 4) ? 0xff : 0x00;
          }
        }
        break;

      case 1:
        for (i = 0; i < _MAX14912_NUM; i++) {
          mo = &_max14912[i];
          _max14912ReadReg(MAX14912_REG_OL, mo, &mo->olRT, &mo->ol);
          mo->faultMemOl |= mo->olRT;
        }
        break;

      case 2:
        for (i = 0; i < _MAX14912_NUM; i++) {
          mo = &_max14912[i];
          _max14912ReadReg(MAX14912_REG_OV, mo, &mo->ovRT, NULL);
          mo->faultMemOv |= mo->ovRT;
          if (mo->ovProtEn) {
            _max14912OverVoltProt(mo);
          }
        }
        break;

      case 3:
        for (i = 0; i < _MAX14912_NUM; i++) {
          mo = &_max14912[i];
          _max14912Cmd(_MAX14912_CMD_SET_STATE, mo, mo->outputs);
        }
        break;

      default:
        for (i = 0; i < _MAX14912_NUM; i++) {
          mo = &_max14912[i];
          _max14912ReadReg(MAX14912_REG_OV, mo, &mo->thsdRT, &mo->thsd);
          mo->faultMemThsd |= mo->thsdRT;
          _max14912ThermalProt(mo);
        }
        _processStep = 0;
        break;
    }

    _processTs = millis();
  }

  for (i = 0; i < 16; i++) {
    if (_subscribeD[i].cb != NULL) {
      _subscribeProcess(&_subscribeD[i]);
    }
    for (j = 0; j < 16; j++) {
      _linkProcess(&_linkD[i][j]);
    }
  }
  for (i = 0; i < 4; i++) {
    if (_subscribeDT[i].cb != NULL) {
      _subscribeProcess(&_subscribeDT[i]);
    }
  }

  ts = micros();
  for (i = 0; i < 16; i++) {
    if (_pwm[i].periodUs > 0) {
      dts = ts - _pwm[i].startTs;
      if (dts > _pwm[i].periodUs) {
        _writeOutputProtected(i + 1, HIGH);
        _pwm[i].startTs = micros();
        _pwm[i].on = true;
      } else if (_pwm[i].on && dts > _pwm[i].dutyUs) {
        _writeOutputProtected(i + 1, LOW);
        _pwm[i].on = false;
      }
    }
  }

  if (_ledVal != _ledSet) {
    _ledCtrl(_ledSet);
    _ledVal = _ledSet;
  }
}

bool IonoD16Class::pinMode(int pin, int mode, bool wbol) {
  struct max14912Str* mo;
  int outIdx;

  if (pin >= DT1 && pin <= DT4) {
    if (mode == INPUT) {
      ::pinMode(pin, INPUT);
      return true;
    }
    if (mode == OUTPUT) {
      ::pinMode(pin, OUTPUT);
      return true;
    }
    return false;
  }

  bool ok = false;
  if (mode == INPUT) {
    if (!_pinModeOutputProtected(pin, OUTPUT_HS, false)) {
      return false;
    }
    if (!_writeOutputProtected(pin, LOW)) {
      return false;
    }
    ok = _pinModeInput(pin, wbol);
  } else if (mode == OUTPUT_HS || mode == OUTPUT_PP) {
    if (mode == OUTPUT_PP && wbol) {
      //  Open-load detection works in high-side mode only
      return false;
    }
    if (!_pinModeInput(pin, false)) {
      return false;
    }
    if (!_max14912GetByPin(pin, &mo, &outIdx)) {
      return false;
    }
    mo->ovProtEn = true;
    ok = _pinModeOutputProtected(pin, mode, wbol);
  }
  if (ok) {
    _pinMode[pin - 1] = mode;
  }
  return ok;
}

bool IonoD16Class::outputsJoin(int pin, bool join) {
  struct max14912Str* m;
  int outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return false;
  }
  if (join && !_outputsJoinable(pin)) {
    return false;
  }
  int bitIdx = (outIdx <= 3) ? 2 : 3;
  _setBit(&m->cfgJoin, bitIdx, join);
  if (!_max14912Config(m, _MAX14912_CMD_SET_CONFIG, MAX14912_REG_WD_JN, m->cfgJoin)) {
    return false;
  }
  return true;
}

int IonoD16Class::read(int pin) {
  if (pin >= DT1 && pin <= DT4) {
    return ::digitalRead(pin);
  }
  struct max22190Str* m;
  int inIdx;
  if (!_max22190GetByPin(pin, &m, &inIdx)) {
    return -1;
  }
  return ((m->inputs >> inIdx) & 1) == 1 ? HIGH : LOW;
}

bool IonoD16Class::write(int pin, int val) {
  if (pin >= DT1 && pin <= DT4) {
    ::digitalWrite(pin, val == HIGH ? HIGH : LOW);
    return true;
  }
  if (pin < D1 || pin > D16) {
    return false;
  }
  if (_pinMode[pin - 1] != OUTPUT_HS && _pinMode[pin - 1] != OUTPUT_PP) {
    return false;
  }
  return _writeOutputProtected(pin, val);
}

bool IonoD16Class::flip(int pin) {
  int val = read(pin);
  if (val < 0) {
    return false;
  }
  return write(pin, val == HIGH ? LOW : HIGH);
}

int IonoD16Class::wireBreakRead(int pin) {
  struct max22190Str* m;
  int ret, inIdx;
  if (!_max22190GetByPin(pin, &m, &inIdx)) {
    return -1;
  }
  ret = _getBit(m->faultMemWb, inIdx) ? HIGH : LOW;
  _setBit(&m->faultMemWb, inIdx, false);
  return ret;
}

int IonoD16Class::openLoadRead(int pin) {
  struct max14912Str* m;
  int ret, outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return -1;
  }
  ret = _getBit(m->faultMemOl, outIdx) ? HIGH : LOW;
  _setBit(&m->faultMemOl, outIdx, false);
  return ret;
}

int IonoD16Class::overVoltageRead(int pin) {
  struct max14912Str* m;
  int ret, outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return -1;
  }
  ret = _getBit(m->faultMemOv, outIdx) ? HIGH : LOW;
  _setBit(&m->faultMemOv, outIdx, false);
  return ret;
}

int IonoD16Class::overVoltageLockRead(int pin) {
  struct max14912Str* m;
  int outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return -1;
  }
  return _getBit(m->ovLock, outIdx) ? HIGH : LOW;
}

int IonoD16Class::thermalShutdownRead(int pin) {
  struct max14912Str* mo;
  struct max22190Str* mi;
  int ret, outIdx, inIdx;
  if (!_max14912GetByPin(pin, &mo, &outIdx)) {
    return -1;
  }
  if (!_max22190GetByPin(pin, &mi, &inIdx)) {
    return -1;
  }
  ret = (_getBit(mo->faultMemThsd, outIdx) ||
          _getBit(mi->faultMemOtshdn, inIdx)) ? HIGH : LOW;
  _setBit(&mo->faultMemThsd, outIdx, false);
  _setBit(&mi->faultMemOtshdn, inIdx, false);
  return ret;
}

int IonoD16Class::thermalShutdownLockRead(int pin) {
  struct max14912Str* m;
  int outIdx;
  if (!_max14912GetByPin(pin, &m, &outIdx)) {
    return -1;
  }
  return _getBit(m->thsdLock, outIdx) ? HIGH : LOW;
}

int IonoD16Class::alarmT1Read(int pin) {
  struct max22190Str* m;
  int ret, inIdx;
  if (!_max22190GetByPin(pin, &m, &inIdx)) {
    return -1;
  }
  ret = _getBit(m->faultMemAlrmT1, inIdx) ? HIGH : LOW;
  _setBit(&m->faultMemAlrmT1, inIdx, false);
  return ret;
}

int IonoD16Class::alarmT2Read(int pin) {
  struct max22190Str* m;
  int ret, inIdx;
  if (!_max22190GetByPin(pin, &m, &inIdx)) {
    return -1;
  }
  ret = _getBit(m->faultMemAlrmT2, inIdx) ? HIGH : LOW;
  _setBit(&m->faultMemAlrmT2, inIdx, false);
  return ret;
}

bool IonoD16Class::outputsClearFaults(int pin) {
  struct max14912Str* m;
  if (!_max14912GetByPin(pin, &m, NULL)) {
    return false;
  }
  m->clearFaults = true;
  return true;
}

void IonoD16Class::subscribe(int pin, unsigned long debounceMs, void (*cb)(int, int)) {
  struct subscribeStr* s;
  if (pin >= DT1 && pin <= DT4) {
    s = &_subscribeDT[pin - DT1];
  } else if (pin >= D1 && pin <= D16) {
    s = &_subscribeD[pin - D1];
  } else {
    return;
  }
  s->pin = pin;
  s->cb = cb;
  s->debounceMs = debounceMs;
  s->value = -1;
  s->lastTs = millis();
}

void IonoD16Class::link(int inPin, int outPin, int mode, unsigned long debounceMs) {
  struct linkStr* l;
  if (inPin >= D1 && inPin <= D16 && outPin >= D1 && outPin <= D16) {
    l = &_linkD[inPin - D1][outPin - D1];
  } else {
    return;
  }
  l->inPin = inPin;
  l->outPin = outPin;
  l->mode = mode;
  l->debounceMs = debounceMs;
  l->value = -1;
  l->lastTs = millis();
}

void IonoD16Class::rs485TxEn(bool enabled) {
  ::digitalWrite(IONO_PIN_RS485_TXEN_N, enabled ? LOW : HIGH);
}

void IonoD16Class::serialTxEn(bool enabled) {
  rs485TxEn(enabled);
}

void IonoD16Class::ledSet(bool on) {
  _ledSet = on;
}

bool IonoD16Class::pwmSet(int pin, int freqHz, uint16_t dutyU16) {
  if (pin < D1 || pin > D16) {
    return false;
  }
  if (_pinMode[pin - 1] != OUTPUT_PP) {
    return false;
  }
  _pwm[pin - 1].periodUs = 0;
  if (dutyU16 == 0) {
    _writeOutputProtected(pin, LOW);
    return true;
  }
  _pwm[pin - 1].dutyUs = 1000000ull * dutyU16 / 65535ull;
  _writeOutputProtected(pin, HIGH);
  _pwm[pin - 1].startTs = micros();
  _pwm[pin - 1].on = true;
  _pwm[pin - 1].periodUs = 1000000ull / freqHz;
  return true;
}

IonoD16Class Iono;
