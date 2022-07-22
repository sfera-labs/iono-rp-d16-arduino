// == Modbus unit address: 1-247 ==
#define CFG_MB_UNIT_ADDDR   10

// == Modbus baudrate (bit/s) ==
//#define CFG_MB_BAUDRATE     1 // 1200
//#define CFG_MB_BAUDRATE     2 // 2400
//#define CFG_MB_BAUDRATE     3 // 4800
//#define CFG_MB_BAUDRATE     4 // 9600
#define CFG_MB_BAUDRATE     5 // 19200
//#define CFG_MB_BAUDRATE     6 // 38400
//#define CFG_MB_BAUDRATE     7 // 57600
//#define CFG_MB_BAUDRATE     8 // 115200

// == Modbus serial parity and stop bits ==
#define CFG_MB_PARITY       1 // parity even, 1 stop bit
//#define CFG_MB_PARITY       2 // parity odd, 1 stop bit
//#define CFG_MB_PARITY       3 // parity none, 2 stop bits

// == Pin modes ==
// 1: Input, no wire-break detection
// 2: Input with wire-break detection
// 3: High-side output, no open-load detection
// 4: High-side output with open-load detection
// 5: Joined High-side output with previuos pin
// 6: Push-pull output
#define CFG_MODE_D1     3
#define CFG_MODE_D2     3
#define CFG_MODE_D3     3
#define CFG_MODE_D4     3
#define CFG_MODE_D5     3
#define CFG_MODE_D6     3
#define CFG_MODE_D7     3
#define CFG_MODE_D8     3
#define CFG_MODE_D9     3
#define CFG_MODE_D10    3
#define CFG_MODE_D11    3
#define CFG_MODE_D12    3
#define CFG_MODE_D13    3
#define CFG_MODE_D14    3
#define CFG_MODE_D15    3
#define CFG_MODE_D16    3

// == Outputs linked pin ==
// 0: not linked
// 1-16: linked to D1-D16
#define CFG_LINK_D1     0
#define CFG_LINK_D2     0
#define CFG_LINK_D3     0
#define CFG_LINK_D4     0
#define CFG_LINK_D5     0
#define CFG_LINK_D6     0
#define CFG_LINK_D7     0
#define CFG_LINK_D8     0
#define CFG_LINK_D9     0
#define CFG_LINK_D10    0
#define CFG_LINK_D11    0
#define CFG_LINK_D12    0
#define CFG_LINK_D13    0
#define CFG_LINK_D14    0
#define CFG_LINK_D15    0
#define CFG_LINK_D16    0

// == Outputs link rule ==
// F: follow - the output is closed when linked input is high
// I: invert - the output is closed when linked input is low
// H: flip on L>H transition - the output is flipped at any linked input transition from low to high
// L: flip on H>L transition - the output is flipped at any linked input transition from high to low
// T: flip on any transition - the output is flipped at any linked input transition, both high to low and low to high
#define CFG_RULE_D1     'F'
#define CFG_RULE_D2     'F'
#define CFG_RULE_D3     'F'
#define CFG_RULE_D4     'F'
#define CFG_RULE_D5     'F'
#define CFG_RULE_D6     'F'
#define CFG_RULE_D7     'F'
#define CFG_RULE_D8     'F'
#define CFG_RULE_D9     'F'
#define CFG_RULE_D10    'F'
#define CFG_RULE_D11    'F'
#define CFG_RULE_D12    'F'
#define CFG_RULE_D13    'F'
#define CFG_RULE_D14    'F'
#define CFG_RULE_D15    'F'
#define CFG_RULE_D16    'F'

// == Value to be written in modbus register to commit config ==
// (change it to invalidate currently saved configuration)
#define CFG_COMMIT_VAL      0xabcd

// == Wiegand pulses params ==
#define WGND_ITVL_MIN 700
#define WGND_ITVL_MAX 3000
#define WGND_WIDTH_MIN 10
#define WGND_WIDTH_MAX 150

#define LINK_DEBOUNCE_MS 25

#include <EEPROM.h>

bool configReset = false;

void configCommit(uint16_t* data, uint8_t len) {
  byte checksum = len;
  for (int w = 0; w < len; w++) {
    for (int b = 0; b < 2; b++) {
      byte val = (data[w] >> (8 * b)) & 0xff;
      EEPROM.write(w * 2 + b + 2, val);
      checksum ^= val;
    }
  }
  EEPROM.write(0, len);
  EEPROM.write(1, checksum);
  EEPROM.commit();

  configReset = true;
}

bool configRead(uint16_t* data, uint8_t len) {
  EEPROM.begin(256);
  if (EEPROM.read(0) != len) {
    return false;
  }
  byte checksum = len;
  for (int w = 0; w < len; w++) {
    for (int b = 0; b < 2; b++) {
      byte val = EEPROM.read(w * 2 + b + 2);
      checksum ^= val;
      data[w] = data[w] | ((val << (8 * b)) & (0xff << (8 * b)));
    }
  }
  if (data[0] != CFG_COMMIT_VAL) {
    return false;
  }
  if (EEPROM.read(1) != checksum) {
    return false;
  }
  return true;
}
