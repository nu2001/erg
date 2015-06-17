
# Erg project

A display for Concept 2 Model C rowing machine.

A lot of stuf here is stolen directly from libopencm3-examples.
Namely the following ones:
- adc-dac-printf
- usb_cdcacm
- tick_blink
- lcd-dma
- lcd-serial (font)


# Setup instructions

## udev rule for stlink

    ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"

## openocd

    sudo apt-get install openocd
    sudo emacs /etc/udev/rules.d/99-stlink.rules
    sudo udevadm control --reload-rules
    sudo apt-get install autoconf pkg-config libusb-1.0 git
    git clone https://github.com/texane/stlink.git
    cd stlink/
    ls
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

