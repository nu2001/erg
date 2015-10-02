# Erg project

A display for Concept 2 Model C rowing machine.

Math/physics behind the erg monitor found here:
http://www.atm.ox.ac.uk/rowing/physics/ergometer.html

On concept2.com forum found that moment of inertia for models B, C, and D is
equal to 0.1001 kg/m^2

A lot of code is stolen directly from libopencm3-examples.
Namely the following ones:
- adc-dac-printf
- usb_cdcacm
- tick_blink
- lcd-dma
- lcd-serial (font)

# Setup instructions

## udev rule for stlink

    ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"

## openocd / stlink

    sudo apt-get install autoconf pkg-config libusb-1.0 git
    git clone https://github.com/texane/stlink.git
    cd stlink/
    ./autogen.sh
    ./configure
    make

## flash

    arm-none-eabi-objcopy -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
    arm-none-eabi-objcopy -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
    stlink/st-flash write $(PROJ_NAME).bin 0x8000000

## start debugger

    ~/stlink/st-util
    arm-none-eabi-gdb progname.elf

## in gdb (or .gdbinit)

    define reload
    kill
    monitor jtag_reset
    load
    end

    target extended localhost:4242
    load
