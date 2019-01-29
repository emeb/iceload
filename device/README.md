# iceload - embedded device appliction
STM32-based USB loader for iCE40 FPGA

This directory contains the source code for an STM32F042 application that
creates a USB CDC class connection to a host PC and accepts binary data
which is loaded into an iCE40 FPGA. The data is packaged with minimal
formatting consisting of:

Byte 0: letter 's'
Bytes 1-4: 32-bit data length (in bytes) with MS-byte first
Bytes 5-N: raw binary data of the iCE40 bitstream

After detecting the initial 's' and length header the application begins
by issuing a CRST signal to the FPGA, then toggling the CS line to flag
that it will load into SRAM. Subsequently, the received bytes are sent
via SPI at roughly 6Mbps. After all received data is sent via SPI then an
additional 56 bits of '1' data are transmitted and the CS line is raised.
The state of the 'CDN' pin is monitored to determine if the configuration
was successful and error status is sent back to the host application with
the following meanings:

* No bits set - success
* bit 0 set - CDN didn't deassert at initial reset.
* bit 1 set - CDN didn't assert after configuration.
* bit 2 set - Received length did not match header length.

This application currently is set up to only load into the on-chip volatile
configuration RAM. It is anticipated that additional commands could be added
to support programming the NVROM, as well as external SPI Flash devices. I
also have plans to support communication with the configured FPGA via SPI
port.

Hardware for this project is a DIY breakout board. More details on the board
are here:

http://ebrombaugh.studionebula.com/embedded/stm32f042breakout/index.html

The source code is based on the ST Microelectronics HAL library and uses USB
drivers generated via the CubeMX program. Application code resides at the top
level directory, while supplied driver code is in lower-level directories.

To build the application you'll need the Gnu ARM Embedded toolchain, OpenOCD
for flashing and make. Simply get into the 'device' directory and run 'make'.
Once that has completed, use 'make flash' to load code via OpenOCD and an
ST-Link v2.1 programmer (any ST Nucleo board will do).

Once programmed, connect the STM32F042 to an iCE40 FPGA:

* PA0 = CRST
* PA1 = CDN
* PA2 = diagnostic serial port
* PA4 = CS
* PA5 = SCK
* PA6 = MISO (connect to SO)
* PA7 = MOSI (connect to SI)

Using the host application (elsewhere in this repository) download a 
bitstream:

./cdc_prog -p <tty port for USB device> <bitstream file>

