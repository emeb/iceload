# iceload - host application
STM32-based USB loader for iCE40 FPGA

This directory contains the Linux host-based application source for
communicating with the embedded application. This is a quick-n-dirty
POSIX application that uses the standard TTY driver to send raw binary
data with a header down a USB CDC class connection.

To build, just execute the "doit.sh" shell script - if this gets more
love I'll write a real makefile, but for now this works fine.

To run, execute the command "./cdc_prog <filename>" where <filename" is
the bitstream file for an iCE40 FPGA. The command assumes that the
embedded device is at /dev/ttyACM1 - if your device is different then
you can use the -p <device> option to override the default.


