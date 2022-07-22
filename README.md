# Iono RP D16 - Arduino IDE libraries and examples

Arduino IDE libraries and examples for [Iono RP D16](https://www.sferalabs.cc/product/iono-rp-d16/) - the multi-purpose digital I/O module with a Raspberry Pi RP2040 (Pico) computing core.

## Arduino IDE setup

Install the Arduino IDE version 1.8.13 or newer

Install the Raspberry Pi RP2040 Arduino core available here:
https://github.com/earlephilhower/arduino-pico

## Library installation

- Open the Arduino IDE
- Download this repo: `git clone --depth 1 --recursive https://github.com/sfera-labs/iono-rp-d16-arduino`
- Go to the menu *Sketch* > *Include Library* > *Add .ZIP Library...*
- Select the downloaded folder

After installation you will see the example sketches under the menu *File* > *Examples* > *Iono RP D16*.

## Uploading a sketch

Select "Generic RP2040" from the menu *Tools* > *Board* > *Raspberry Pi RP2040 Boards*.
You will see additional menu entries under *Tools*, set Flash size to 16MB (with or without FS) and leave the other entries unchanged.

The **first** time you upload a sketch:
- Remove power to Iono RP D16 (disconnect main power supply and USB)
- Hold the BOOTSEL button down
- Connect the USB cable to Iono RP D16
- Release the BOOTSEL button

Hit the Arduino IDE's upload button, the sketch will be transferred and start to run.

After the first upload, the board will appear under the standard Serial ports list and will automatically reset and switch to bootloader mode when hitting the IDE's upload button as with any other Arduino boards.

## Library usage

For a quick start, check out the [examples](./examples).

Following some more details:

You can include the library in your sketch with:

```C++
#include <IonoD16.h>
```

It exports a `Iono` object which has the methods described below.

It defines the constants `D1` ... `D16` and `DT1` ... `DT4`, corresponding to Iono RP D16's I/Os, to be used as the `pin` parameter of the `Iono` object methods.

A reference to the serial port connected to the RS-485 interface is available as `IONO_RS485`.     
Usage example:

```C++
IONO_RS485.begin(19200, SERIAL_8N1);
```

The TX-enable pin should be controlled when sending or receiving data on the RS-485 interface, use the rs485TxEn() method.

### **`Iono`'s methods:**

### `bool setup()`
Call this function in your `setup()`, before using any other functionality of the library. It initializes the used pins and peripherals.
#### Returns
`true` upon success.

<br/>

### `bool ready()`
Useful for synchronization between the two cores.
#### Returns
whether or not the `setup()` method has been executed.
#### Example
```C++
void setup() {
  Iono.setup();
  // ...
}

void setup1() {
  // wait for setup() to complete on the other core
  while (!Iono.ready()) ;
  // ...
}
```

<br/>

### `void process()`
Call this function periodically, with a maximum interval of 20ms. It performs the reading of the input/output peripherals' state, moreover it checks for fault conditions, enables safety routines and updates the outputs' watchdog. It is recommended to reserve one core of the RP2040 for calling this function, while performing your custom logic on the other core.
#### Example
```C++
void loop1() {
    Iono.process();
    delay(10);
}

void loop() {
    // read/write Iono's I/O and perform your other tasks
}
```

<br/>

### `bool pinMode(int pin, int mode, bool wbol=false)`
Initializes a pin as input or output. To be called before any other operation on the same pin.
#### Parameters
**`pin`**: `D1` ... `D16`, `DT1` ... `DT4`

**`mode`**:
- `INPUT`: use pin as input
- `OUTPUT`: use pin as input (only for `DT1` ... `DT4`)
- `OUTPUT_HS`: use pin as high-side output (only for `D1` ... `D16`)
- `OUTPUT_PP`: use pin as push-pull output (only for `D1` ... `D16`)

**`wbol`**: enable (`true`) or disable (`false`) wire-break (for inputs) or open-load (for high-side outputs) detection (only for `D1` ... `D16`)
#### Returns
`true` upon success.

<br/>

### `bool outputsJoin(int pin, bool join=true)`
Joins two high-side outputs to be used as a single output. Outputs can be joined in specific pairs: `D1`-`D2`, `D3`-`D4`, ..., `D15`-`D16`.
When a pair is joined, the other pair of the same group of four is also joined, e.g. joining `D3`-`D4` results in `D1`-`D2` being joined too if used as outputs.
Therefore, joining a pair requires the pins of the other pair of the same group of four to be initialized as `OUTPUT_HS` or `INPUT`. In the latter case the two pins can be used as independent inputs.
#### Parameters
**`pin`**: one pin of the pair: `D1` ... `D16`

**`join`**: `true` to join, `false` to un-join
#### Returns
`true` upon success.

<br/>

### `int read(int pin)`
Returns the state of a pin.    
For `D1` ... `D16` pins the returned value corresponds to the reading performed during the latest `process()` call.    
For `DT1` ... `DT4` pins the returned value corresponds to the instant reading of the corresponding GPIO.
#### Parameters
**`pin`**: `D1` ... `D16`, `DT1` ... `DT4`
#### Returns
`HIGH`, `LOW`, or `-1` upon error.

<br/>

### `bool write(int pin, int val)`
Sets the value of an output pin.
#### Parameters
**`pin`**: `D1` ... `D16`, `DT1` ... `DT4`

**`val`**: `HIGH` or `LOW`
#### Returns
`true` upon success.

<br/>

### `bool flip(int pin)`
Flips the value of an output pin.
#### Parameters
**`pin`**: `D1` ... `D16`, `DT1` ... `DT4`
#### Returns
`true` upon success.

<br/>

### `int wireBreakRead(int pin)`
Returns the wire-break fault state of an input pin with wire-break detection enabled.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if wire-break detected, `LOW` if wire-break not detected, or `-1` upon error.

<br/>

### `int openLoadRead(int pin)`
Returns the open-load fault state of an input pin with open-load detection enabled.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if open-load detected, `LOW` if open-load not detected, or `-1` upon error.

<br/>

### `int overVoltageRead(int pin)`
Returns the over-voltage fault state of a pin.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if over-voltage detected, `LOW` if over-voltage not detected, or `-1` upon error.

<br/>

### `int overVoltageLockRead(int pin)`
Returns whether or not an output pin is temporarily locked due to an over-voltage condition.    
The output cannot be set when locked.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if locked, `LOW` if not locked, or `-1` upon error.

<br/>

### `int thermalShutdownRead(int pin)`
Returns the thermal shutdown fault state of a pin.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if thermal shutdown active, `LOW` if thermal shutdown not active, or `-1` upon error.

<br/>

### `int thermalShutdownLockRead(int pin)`
Returns whether or not an output pin is temporarily locked due to a thermal shutdown condition.    
The output cannot be set when locked.    
The fault state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if locked, `LOW` if not locked, or `-1` upon error.

<br/>

### `int alarmT1Read(int pin)`
Returns whether or not the temperature alarm 1 threshold has been exceeded on the input peripheral the pin belongs to.    
The state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if threshold exceeded, `LOW` if threshold not exceeded, or `-1` upon error.

<br/>

### `int alarmT2Read(int pin)`
Returns whether or not the temperature alarm 2 threshold has been exceeded on the input peripheral the pin belongs to.    
The state is updated on each `process()` call and set `HIGH` when detected. It is cleared (set `LOW`) only after calling this method.
#### Parameters
**`pin`**: `D1` ... `D16`
#### Returns
`HIGH` if threshold exceeded, `LOW` if threshold not exceeded, or `-1` upon error.

<br/>

### `void subscribe(int pin, unsigned long debounceMs, void (*cb)(int, int))`
Set a callback function to be called upon input state change with a debounce filter.    
The callback function is called within `process()` execution, it is therefore recommended to execute only quick operations.
#### Parameters
**`pin`**: `D1` ... `D16`, `DT1` ... `DT4`

**`debounceMs`**: debounce time in milliseconds

**`cb`**: callback function

<br/>

### `void link(int inPin, int outPin, int mode, unsigned long debounceMs)`
Links the state of two pins. when the `inPin` pin changes state (with the specified debounce filter), the `outPin` output is set according to the specified mode.
#### Parameters
**`pin`**: `D1` ... `D16`

**`mode`**:
- `LINK_FOLLOW`: `outPin` is set to the same state of `inPin`
- `LINK_INVERT`: `outPin` is set to the opposite state of `inPin`
- `LINK_FLIP_H`: `outPin` is flipped upon each low-to-high transition of `inPin`
- `LINK_FLIP_L`: `outPin` is flipped upon each high-to-low transition of `inPin`
- `LINK_FLIP_T`: `outPin` is flipped upon each state transition of `inPin`

**`debounceMs`**: debounce time in milliseconds

<br/>

### `void ledSet(bool on)`
Sets the blue 'ON' LED state.
#### Parameters
**`on`**: `true` LED on, `false` LED off

<br/>

### `bool pwmSet(int pin, int freqHz, uint16_t dutyU16)`
Sets a soft-PWM on a push-pull output.    
The maximum frequency is determined by the frequency of `process()` calls.
#### Parameters
**`pin`**: `D1` ... `D16`

**`freqHz`**: frequency in Hz

**`dutyU16`**: duty cycle as a ratio `dutyU16 / 65535`
#### Returns
`true` upon success.

<br/>

### `void rs485TxEn(bool enabled)`
Controls the TX-enable line of the RS-485 interface.    
Call `Iono.serialTxEn(true)` before writing to the IONO_RS485 serial. When incoming data is expected, call `Iono.serialTxEn(false)` before. Good practice is to call `Iono.serialTxEn(false)` as soon as data has been written and flushed to the serial port.
#### Parameters
**`enabled`**: `true` to enable the TX-enable line, `false` to disable it
#### Example
```C++
Iono.rs485TxEn(true);
IONO_RS485.write(data);
IONO_RS485.write(more_data);
IONO_RS485.flush();
Iono.rs485TxEn(false);
IONO_RS485.readBytes(buffer, length);
```
