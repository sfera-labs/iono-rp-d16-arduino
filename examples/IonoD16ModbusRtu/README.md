# Iono RP D16 Modbus RTU

This sketch turns [Iono RP D16](https://www.sferalabs.cc/product/iono-rp-d16/) into a standard Modbus RTU slave device with access to all its functionalities over RS-485.

It requires the [Modbus RTU Slave library](https://github.com/sfera-labs/arduino-modbus-rtu-slave) and the [Wiegand library](https://github.com/sfera-labs/arduino-wiegand) to be installed.

## Configuration

Before uploading the sketch, you can modify the default configuration setting the `CFG_*` defines in [config.h](config.h), where you find the relative documentation too.

All configuration parameters can then be modified via Modbus using the configuration registers listed below.

## Modbus registers

Refer to the following tables for the list of available registers and corresponding supported Modbus functions.

For the "Functions" column:    
1 = Read coils    
2 = Read discrete inputs    
3 = Read holding registers    
4 = Read input registers    
5 = Write single coil    
6 = Write single register    
15 = Write multiple coils    
16 = Write multiple registers    

### Configuration

|Address|R/W|Functions|Size [words]|Data type|Description|
|------:|:-:|---------|------------|---------|-----------|
|1000|W|6,16|1|unsigned short|Write `0xABCD` (or modified value set in `CFG_COMMIT_VAL` in [config.h](config.h)) to commit the new configuration written in the registers below. This register can only be written individually, i.e. using function 6, or function 16 with a single data value. After positive response the unit is restarted and the new configuration is applied|
|1001|R/W|3,6,16|1|unsigned short|Modbus unit address|
|1002|R/W|3,6,16|1|unsigned short|Modbus baud rate:<br/>`1` = 1200<br/>`2` = 2400<br/>`3` = 4800<br/>`4` = 9600<br/>`5` = 19200<br/>`6` = 38400<br/>`7` = 57600<br/>`8` = 115200|
|1003|R/W|3,6,16|1|unsigned short|Modbus parity and stop bits:<br/>`1` = parity even, 1 stop bit<br/>`2` = parity odd, 1 stop bit<br/>`3` = parity none, 2 stop bits|
|1004&nbsp;...&nbsp;1019|R/W|3,6,16|1|unsigned short|D1 ... D16 pin mode:<br/>`1` = input w/o wire-break detection<br/>`2` = input w/ wire-break detection<br/>`3` = high-side output w/o open-load detection<br/>`4` = high-side output w/ open-load detection<br/>`5` = high-side output joined with preceding pin<br/>`6` = push-pull output|
|1020&nbsp;...&nbsp;1035|R/W|3,6,16|1|unsigned short|D1 ... D16 linked pin: for output pins only, set this register to `1` ... `16` to have this output change state when the corresponding pin (D1 ... D16) transitions as per the link-mode set in the registers below. Set to `0` (default) to disable linking|
|1036&nbsp;...&nbsp;1051|R/W|3,6,16|1|unsigned short|D1 ... D16 link-mode:<br/>`0x46` \(ASCII code for `'F'`\) = follow - the output is set to the same state of the linked pin<br/>`0x49` \(ASCII code for `'I'`\) = invert - the output is set to the opposite state of the linked pin<br/>`0x48` \(ASCII code for `'H'`\) = flip on L>H transition - the output is flipped upon each low-to-high transition of the linked pin<br/>`0x4C` \(ASCII code for `'L'`\) = flip on H>L transition - the output is flipped upon each high-to-low transition of the linked pin<br/>`0x54` \(ASCII code for `'T'`\) = flip on any transition - the output is flipped upon each state transition of the linked pin|

### Monitoring and control

|Address|R/W|Functions|Size|Data type|Description|
|------:|:-:|---------|----|---------|-----------|
|1&nbsp;...&nbsp;16|R/W|1,2,5,15|1 bit|-|D1 ... D16 state|
|101&nbsp;...&nbsp;116|R|2|1 bit|-|D1 ... D16 wire-break fault state|
|201&nbsp;...&nbsp;216|R|2|1 bit|-|D1 ... D16 open-load fault state|
|301&nbsp;...&nbsp;316|R|2|1 bit|-|D1 ... D16 over-voltage fault state|
|401&nbsp;...&nbsp;416|R|2|1 bit|-|D1 ... D16 over-voltage lock state|
|501&nbsp;...&nbsp;516|R|2|1 bit|-|D1 ... D16 thermal shutdown fault state|
|601&nbsp;...&nbsp;616|R|2|1 bit|-|D1 ... D16 thermal shutdown lock state|
|701&nbsp;...&nbsp;716|R|2|1 bit|-|D1 ... D16 temperature alarm 1 threshold exceeded|
|801&nbsp;...&nbsp;816|R|2|1 bit|-|D1 ... D16 temperature alarm 2 threshold exceeded|
|2001&nbsp;...&nbsp;2016|W|6,16|1 word|unsigned short|D1 ... D16 output set high for specified time (s/10)|
|2101&nbsp;...&nbsp;2116|W|6,16|1 word|unsigned short|D1 ... D16 push-pull output soft-PWM frequency (Hz). Enabled only when duty-cycle set (see registers below)|
|2201&nbsp;...&nbsp;2216|W|6,16|1 word|unsigned short|D1 ... D16 enable push-pull output soft-PWM with specified duty-cycle (as ratio value/65535)|
|2301&nbsp;...&nbsp;2316|W|6,16|1 word|unsigned short|D1 ... D16 enable debounce and counters (see registers below) with specified debounce time (ms). Set to `0` (default) to disable|
|2401&nbsp;...&nbsp;2416|R|1,2|1 bit|-|D1 ... D16 debounced state|
|2501&nbsp;...&nbsp;2516|R|4|1 word|unsigned short|D1 ... D16 low-to-high transition counter after debounce filter, rolls back to 0 after 65535|
|3001|W|5,15|1 bit|-|blue 'ON' LED state|

### Wiegand devices

Pins DT1-DT2 and DT3-DT4 can alternatively be used as two separate Wiegand interfaces. Connect the DATA0 and DATA1 wires of the Wiegand device(s) respectively to DT1 and DT2 (interface 1) or DT3 and DT4 (interface 2).

Each Wiegand interface is enabled upon the first reading of registers 4001 and 5001 respectively.

When new data is available on the interface, registers 4001/5001 are updated with the number of bits read. Reading registers 4001/5001 triggers the update of registers 4002/5002 with the read data; after a positive number is returned, subsequent reads will return `0` until new data is available. If `-1` is returned it indicates that data is currently being received so it will be shortly available.

Registers 4002/5002 contain up to 64 bits split in 4 words, starting from the 16 least significant bits. Therefore, to collect Wiegand data, periodically read registers 4001/5001 and when a positive value is returned read the necessary number of words (or all 4) from registers 4002/5002.

|Address|R/W|Functions|Size [words]|Data type|Description|
|------:|:-:|---------|------------|---------|-----------|
|4001|R|4|1|signed short|interface 1 - new data bits number|
|4002|R|4|1 - 4|bits|interface 1 - new data|
|4010|R|4|1|unsigned short|interface 1 - latest noise event (see below), reset to 0 after read|
|5001|R|4|1|signed short|interface 2 - new data bits number|
|5002|R|4|1 - 4|bits|interface 2 - new data|
|5010|R|4|1|unsigned short|interface 2 - latest noise event (see below), reset to 0 after read|

Noise detected on the DATA0/DATA1 lines:

|Noise value|Description|
|:---------:|-----------|
0 | No noise
10 | Fast pulses on lines
11 | Pulses interval too short
12/13 | Concurrent movement on both DATA0/DATA1 lines (possibly short circuit between lines)
14 | Pulse too short
15 | Pulse too long
