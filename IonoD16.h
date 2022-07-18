/*
  IonoD16.h - Iono RP base library

    Copyright (C) 2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    http://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoD16_h
#define IonoD16_h

#include <SPI.h>
#include <mutex>

#define IONO_PIN_DT1 26
#define IONO_PIN_DT2 27
#define IONO_PIN_DT3 28
#define IONO_PIN_DT4 29

#define IONO_PIN_RS485_TX 16
#define IONO_PIN_RS485_RX 17
#define IONO_PIN_RS485_TXEN_N 14

#define IONO_PIN_I2C_SDA 0
#define IONO_PIN_I2C_SCL 1

#define IONO_PIN_SPI_SCK 2
#define IONO_PIN_SPI_TX 3
#define IONO_PIN_SPI_RX 4

#define IONO_PIN_CS_DOL 6
#define IONO_PIN_CS_DOH 5
#define IONO_PIN_CS_DIL 8
#define IONO_PIN_CS_DIH 7

#define IONO_PIN_MAX14912_WD_EN 18

#define IONO_PIN_MAX22190_LATCH 9

#define SERIAL_PORT_MONITOR Serial
#define SERIAL_PORT_HARDWARE Serial1

#define IONO_RS485 SERIAL_PORT_HARDWARE

#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define D14 14
#define D15 15
#define D16 16
#define DT1 IONO_PIN_DT1
#define DT2 IONO_PIN_DT2
#define DT3 IONO_PIN_DT3
#define DT4 IONO_PIN_DT4

#define OUTPUT_HS (OUTPUT + 100)
#define OUTPUT_PP (OUTPUT + 101)

#define _MAX22190_NUM 2
#define _MAX14912_NUM 2

class IonoD16Class {
  public:
    IonoD16Class();
    bool setup();
    bool ready();
    void rs485TxEn(bool);
    void serialTxEn(bool);
    void process();
    int read(int);
    bool write(int, int);
    int wireBreakRead(int);
    int openLoadRead(int);
    int overVoltageRead(int);
    int overVoltageLockRead(int);
    int thermalShutdownRead(int);
    int thermalShutdownLockRead(int);
    int alarmT1Read(int);
    int alarmT2Read(int);
    bool pinMode(int, int, bool wbol=false);
    bool outputsJoin(int, bool join=true);
    bool outputsClearFaults(int);
    void subscribe(int, unsigned long, void (*)(int, int));
    void ledSet(bool);

  private:
    bool _setupDone;
    int _pinMode[16];
    SPISettings _spiSettings;
    mutex_t _spiMtx;
    byte _max14912ReadStatCrc;
    bool _ledSet;
    bool _ledVal;
    struct max22190Str {
      int pinCs;
      bool error;
      byte inputs;
      byte wb;
      byte fault1;
      byte fault2;
      byte cfgFlt[8];
      byte faultMemWb;
      byte faultMemAlrmT1;
      byte faultMemAlrmT2;
      byte faultMemOtshdn;
    } _max22190[_MAX22190_NUM];
    struct max14912Str {
      int pinCs;
      bool error;
      byte outputs;
      byte outputsUser;
      bool clearFaults;
      byte ol;
      byte olRT;
      byte ovRT;
      byte thsd;
      byte thsdRT;
      byte cfgModePP;
      byte cfgModePPUser;
      byte cfgOlDet;
      byte cfgJoin;
      byte ovLock;
      byte thsdLock;
      byte faultMemOl;
      byte faultMemOv;
      byte faultMemThsd;
      unsigned long lockTs[8];
    } _max14912[_MAX14912_NUM];
    struct subscribeStr {
      int pin;
      void (*cb)(int, int);
      unsigned long debounceMs;
      int value;
      unsigned long lastTs;
    } _subscribeD[16], _subscribeDT[4];

    bool _getBit(byte, int);
    void _setBit(byte*, int, bool);
    void _spiTransaction(int, byte, byte, byte, byte*, byte*, byte*);
    byte _max22190Crc(byte, byte, byte);
    bool _max22190SpiTransaction(struct max22190Str*, byte*, byte*);
    bool _max22190ReadReg(byte, struct max22190Str*, byte*);
    bool _max22190WriteReg(byte, struct max22190Str*, byte);
    bool _max22190GetByPin(int, struct max22190Str**, int*);
    byte _max14912CrcLoop(byte, byte);
    byte _max14912Crc(byte, byte);
    bool _max14912SpiTransaction(bool, struct max14912Str*, byte*, byte*);
    bool _max14912ReadReg(byte, struct max14912Str*, byte*, byte*);
    bool _max14912Cmd(byte, struct max14912Str*, byte);
    bool _max14912Config(struct max14912Str*, byte, byte, byte);
    bool _max14912GetByPin(int, struct max14912Str**, int*);
    bool _max14912OutputSet(struct max14912Str*, int, bool);
    bool _max14912ModePPSet(struct max14912Str*, int, bool);
    void _max14912OverVoltProt(struct max14912Str*);
    void _max14912ThermalProt(struct max14912Str*);
    bool _pinModeInput(int, bool);
    bool _pinModeOutputProtected(int, int, bool);
    bool _writeOutputProtected(int, int);
    bool _outputsJoinable(int);
    void _subscribeProcess(struct subscribeStr*);
    void _ledCtrl(bool);
};

extern IonoD16Class Iono;

#endif
