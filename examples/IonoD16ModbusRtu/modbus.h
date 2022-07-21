#include <ModbusRtuSlave.h>
#include <Wiegand.h>

#define MB_REG_CFG_START               1000
#define MB_REG_CFG_OFFSET_COMMIT       0
#define MB_REG_CFG_OFFSET_MB_ADDR      1
#define MB_REG_CFG_OFFSET_MB_BAUD      2
#define MB_REG_CFG_OFFSET_MB_PARITY    3
#define MB_REG_CFG_OFFSET_MODE_D1      4
#define MB_REG_CFG_OFFSET_MODE_D2      (MB_REG_CFG_OFFSET_MODE_D1 + 1)
#define MB_REG_CFG_OFFSET_MODE_D3      (MB_REG_CFG_OFFSET_MODE_D1 + 2)
#define MB_REG_CFG_OFFSET_MODE_D4      (MB_REG_CFG_OFFSET_MODE_D1 + 3)
#define MB_REG_CFG_OFFSET_MODE_D5      (MB_REG_CFG_OFFSET_MODE_D1 + 4)
#define MB_REG_CFG_OFFSET_MODE_D6      (MB_REG_CFG_OFFSET_MODE_D1 + 5)
#define MB_REG_CFG_OFFSET_MODE_D7      (MB_REG_CFG_OFFSET_MODE_D1 + 6)
#define MB_REG_CFG_OFFSET_MODE_D8      (MB_REG_CFG_OFFSET_MODE_D1 + 7)
#define MB_REG_CFG_OFFSET_MODE_D9      (MB_REG_CFG_OFFSET_MODE_D1 + 8)
#define MB_REG_CFG_OFFSET_MODE_D10     (MB_REG_CFG_OFFSET_MODE_D1 + 9)
#define MB_REG_CFG_OFFSET_MODE_D11     (MB_REG_CFG_OFFSET_MODE_D1 + 10)
#define MB_REG_CFG_OFFSET_MODE_D12     (MB_REG_CFG_OFFSET_MODE_D1 + 11)
#define MB_REG_CFG_OFFSET_MODE_D13     (MB_REG_CFG_OFFSET_MODE_D1 + 12)
#define MB_REG_CFG_OFFSET_MODE_D14     (MB_REG_CFG_OFFSET_MODE_D1 + 13)
#define MB_REG_CFG_OFFSET_MODE_D15     (MB_REG_CFG_OFFSET_MODE_D1 + 14)
#define MB_REG_CFG_OFFSET_MODE_D16     (MB_REG_CFG_OFFSET_MODE_D1 + 15)
#define MB_REG_CFG_OFFSET_MAX          MB_REG_CFG_OFFSET_MODE_D16

static word _cfgRegisters[MB_REG_CFG_OFFSET_MAX + 1];

bool _doEnabled[16];
unsigned long _doStart[16];
unsigned long _doTime[16];

static word _pwmFreq[16];

static bool _debounce[16];
static word _counters[16];

static void _debounceCb(int pin, int val) {
  _debounce[pin - 1] = val == HIGH;
  if (_debounce[pin - 1]) {
    _counters[pin - 1]++;
  }
}

static Wiegand _wgnd[] = {Wiegand(DT1, DT2), Wiegand(DT3, DT4)};
static uint64_t _wgndData[2];
static bool _wgndInit[2];

static void _wgnd0OnData0() {
  _wgnd[0].onData0();
}

static void _wgnd0OnData1() {
  _wgnd[0].onData1();
}

static void _wgnd1OnData0() {
  _wgnd[1].onData0();
}

static void _wgnd1OnData1() {
  _wgnd[1].onData1();
}

static void (*_wgndOnData0[])(void) = {&_wgnd0OnData0, &_wgnd1OnData0};
static void (*_wgndOnData1[])(void) = {&_wgnd0OnData1, &_wgnd1OnData1};

static bool _checkAddrRange(word regAddr, word qty, word min, word max) {
  return regAddr >= min && regAddr <= max && regAddr + qty <= max + 1;
}

static byte _modbusOnRequest(byte unitAddr, byte function, word regAddr, word qty, byte *data) {
  switch (function) {
    case MB_FC_READ_DISCRETE_INPUTS:
      if (_checkAddrRange(regAddr, qty, 101, 116)) {
        for (int i = regAddr - 100; i < regAddr - 100 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.wireBreakRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 201, 216)) {
        for (int i = regAddr - 200; i < regAddr - 200 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.openLoadRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 301, 316)) {
        for (int i = regAddr - 300; i < regAddr - 300 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.overVoltageRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 401, 416)) {
        for (int i = regAddr - 400; i < regAddr - 400 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.overVoltageLockRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 501, 516)) {
        for (int i = regAddr - 500; i < regAddr - 500 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.thermalShutdownRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 601, 616)) {
        for (int i = regAddr - 600; i < regAddr - 600 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.thermalShutdownLockRead(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 701, 716)) {
        for (int i = regAddr - 700; i < regAddr - 700 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.alarmT1Read(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 801, 816)) {
        for (int i = regAddr - 800; i < regAddr - 800 + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.alarmT2Read(i));
        }
        return MB_RESP_OK;
      }
      // fall into case MB_FC_READ_COILS to have registers 1-16
      // readable with MB_FC_READ_DISCRETE_INPUTS too

    case MB_FC_READ_COILS:
      if (_checkAddrRange(regAddr, qty, 1, 16)) {
        for (int i = regAddr; i < regAddr + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.read(i));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 3101, 3116)) {
        for (int i = regAddr - 3100; i < regAddr - 3100 + qty; i++) {
          ModbusRtuSlave.responseAddBit(_debounce[i - 1]);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_MULTIPLE_COILS:
    case MB_FC_WRITE_SINGLE_COIL:
      if (_checkAddrRange(regAddr, qty, 1, 16)) {
        bool ok = true;
        for (int i = regAddr; i < regAddr + qty; i++) {
          bool on = ModbusRtuSlave.getDataCoil(function, data, i - regAddr);
          if (!Iono.write(i, on ? HIGH : LOW)) {
            ok = false;
          }
        }
        return ok ? MB_RESP_OK : MB_EX_ILLEGAL_FUNCTION;
      }
      if (regAddr == 3001 && qty == 1) {
        bool on = ModbusRtuSlave.getDataCoil(function, data, 0);
        Iono.ledSet(on);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_INPUT_REGISTER:
      if (_checkAddrRange(regAddr, qty, 3201, 3216)) {
        for (int i = regAddr - 3200; i < regAddr - 3200 + qty; i++) {
          ModbusRtuSlave.responseAddRegister(_counters[i - 1]);
        }
        return MB_RESP_OK;
      }
      if ((regAddr == 4001 || regAddr == 5001) && qty == 1) {
        int idx = regAddr == 4001 ? 0 : 1;
        if (!_wgndInit[idx]) {
          _wgnd[idx].setup(_wgndOnData0[idx], _wgndOnData1[idx], false,
            WGND_ITVL_MIN, WGND_ITVL_MAX, WGND_WIDTH_MIN, WGND_WIDTH_MAX);
          _wgndInit[idx] = true;
        }
        ModbusRtuSlave.responseAddRegister(_wgnd[idx].getData(&_wgndData[idx]));
        return MB_RESP_OK;
      }
      if ((regAddr == 4002 || regAddr == 5002) && qty <= 4) {
        int idx = regAddr == 4002 ? 0 : 1;
        for (int i = 0; i < qty; i++) {
          ModbusRtuSlave.responseAddRegister((_wgndData[idx] >> (i * 16)) & 0xffff);
        }
        return MB_RESP_OK;
      }
      if ((regAddr == 4010 || regAddr == 5010) && qty == 1) {
        int idx = regAddr == 4010 ? 0 : 1;
        ModbusRtuSlave.responseAddRegister(_wgnd[idx].getNoise());
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_HOLDING_REGISTERS:
      if (_checkAddrRange(regAddr, qty, MB_REG_CFG_START, MB_REG_CFG_START + MB_REG_CFG_OFFSET_MAX)) {
        int offset = regAddr - MB_REG_CFG_START;
        int offsetEnd = offset + qty;
        for (int i = offset; i < offsetEnd; i++) {
          ModbusRtuSlave.responseAddRegister(_cfgRegisters[i]);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_SINGLE_REGISTER:
    case MB_FC_WRITE_MULTIPLE_REGISTERS:
      if (_checkAddrRange(regAddr, qty, 2001, 2016)) {
        bool ok = true;
        for (int i = regAddr - 2000; i < regAddr - 2000 + qty; i++) {
          _doTime[i - 1] = 100 * ModbusRtuSlave.getDataRegister(function, data, i - (regAddr - 2000));
          if (_doTime[i - 1] > 0) {
            _doEnabled[i - 1] = true;
            _doStart[i - 1] = millis();
            if (!Iono.write(i, HIGH)) {
              ok = false;
            }
          }
        }
        return ok ? MB_RESP_OK : MB_EX_ILLEGAL_FUNCTION;
      }
      if (_checkAddrRange(regAddr, qty, 2101, 2116)) {
        for (int i = regAddr - 2100; i < regAddr - 2100 + qty; i++) {
          _pwmFreq[i - 1] = ModbusRtuSlave.getDataRegister(function, data, i - (regAddr - 2100));
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, 2201, 2216)) {
        bool ok = true;
        for (int i = regAddr - 2200; i < regAddr - 2200 + qty; i++) {
          word duty = ModbusRtuSlave.getDataRegister(function, data, i - (regAddr - 2200));
          if (!Iono.pwmSet(i, _pwmFreq[i - 1], duty)) {
            ok = false;
          }
        }
        return ok ? MB_RESP_OK : MB_EX_ILLEGAL_FUNCTION;
      }
      if (_checkAddrRange(regAddr, qty, 3001, 3016)) {
        for (int i = regAddr - 3000; i < regAddr - 3000 + qty; i++) {
          word deb = ModbusRtuSlave.getDataRegister(function, data, i - (regAddr - 3000));
          if (deb > 0) {
            Iono.subscribe(i, deb, _debounceCb);
          } else {
            Iono.subscribe(i, 0, NULL);
          }
        }
        return MB_RESP_OK;
      }
      if (_checkAddrRange(regAddr, qty, MB_REG_CFG_START, MB_REG_CFG_START + MB_REG_CFG_OFFSET_MAX)) {
        int offset = regAddr - MB_REG_CFG_START;
        int offsetEnd = offset + qty;
        bool commit = false;

        for (int i = offset; i < offsetEnd; i++) {
          word val = ModbusRtuSlave.getDataRegister(function, data, i - offset);
          switch (i) {
            case MB_REG_CFG_OFFSET_COMMIT:
              if (qty != 1 || val != CFG_COMMIT_VAL) {
                return MB_EX_ILLEGAL_DATA_VALUE;
              }
              commit = true;
              break;
            case MB_REG_CFG_OFFSET_MB_ADDR:
              if (val < 1 || val > 247) {
                return MB_EX_ILLEGAL_DATA_VALUE;
              }
              break;
            case MB_REG_CFG_OFFSET_MB_BAUD:
              if (val < 1 || val > 8) {
                return MB_EX_ILLEGAL_DATA_VALUE;
              }
              break;
            case MB_REG_CFG_OFFSET_MB_PARITY:
              if (val < 1 || val > 3) {
                return MB_EX_ILLEGAL_DATA_VALUE;
              }
              break;
            default: // pin mode
              if (val < 1 || val > 6) {
                return MB_EX_ILLEGAL_DATA_VALUE;
              }
              break;
          }
          _cfgRegisters[i] = val;
        }

        if (commit) {
          configCommit(_cfgRegisters, MB_REG_CFG_OFFSET_MAX + 1);
        }

        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    default:
      return MB_EX_ILLEGAL_FUNCTION;
  }
}

void modbusBegin(byte unitAddr, uint16_t baudIdx, uint16_t parity) {
  uint16_t serCfg;
  switch (parity) {
    case 2:
      serCfg = SERIAL_8O1;
      break;
    case 3:
      serCfg = SERIAL_8N2;
      break;
    default:
      serCfg = SERIAL_8E1;
  }

  unsigned long baud;
  switch (baudIdx) {
    case 1:
      baud = 1200;
      break;
    case 2:
      baud = 2400;
      break;
    case 3:
      baud = 4800;
      break;
    case 5:
      baud = 19200;
      break;
    case 6:
      baud = 38400;
      break;
    case 7:
      baud = 57600;
      break;
    case 8:
      baud = 115200;
      break;
    default:
      baud = 9600;
  }

  IONO_RS485.begin(baud, serCfg);
  ModbusRtuSlave.setCallback(&_modbusOnRequest);
  ModbusRtuSlave.begin(unitAddr, &IONO_RS485, baud, IONO_PIN_RS485_TXEN_N, true);
}

void modbusProcess() {
  ModbusRtuSlave.process();
}
