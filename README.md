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

You can inport the library in your sketch with:

    #include <IonoD16.h>

Check out the [examples](./examples) for a quick start.

Here some more details on the available functions:

### `Iono.setup()`
Call this function in your `setup()`, before using any other functionality of the library. It initializes the used pins and peripherals.

### `Iono.process()`
Call this function periodically, with a maximum interval of 100ms. It performs the reading of the input/output peripherals' state, moreover it checks for error conditions, enables safety routines and updates the outputs' watchdog. It is recommended to reserve one core of the RP2040 for calling this function, while performing your custom logic on the other core.

Example:
```
void loop1() {
    Iono.process();
}

void loop() {
    // read/write Iono's I/O and perform your other tasks
}
```



