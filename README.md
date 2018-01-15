## Serperf serial device test tool

[![Build Status](https://travis-ci.org/HernanG234/sertool.svg?branch=master)](https://travis-ci.org/HernanG234/sertool#)

### Intro

The tool consists on a program that sets or gets the different parameters
of a serialX device.

The tool can:
1. Set parameters
2. Get parameters
3. Clear the rx software fifo
4. Send and rcv msgs through ioctl (instead of standard read(), write())

### Installation

Clone from [here](https://github.com/HernanG234/sertool). Run make.

### Usage

Arguments:

```
-s | --set -> set parameters
-g | --get -> get parameters
-c | --rx-buf-clear -> clear rx sw fifo
-n | --send-msg -> send-msg through ioctl
-v | --rcv-msg -> rcv-msg thourgh ioctl

When sending a msg, specify the msg to send.
When receiving a msg, specify the amount of bytes to rcv.

When -s is set, user has to define what to set:
-b | --baudrate -> set baudrate
-d | --data-bits -> set databits
-p | --parity -> set parity
-o | --stop-bits -> set stop-bits
-r | --rcv-timeout -> set rcv-timeout (in ms)
-x | --xmit-timeout -> set xmit-timeout (in ms)
```

### Examples

Set baudrate:

	> #sertool -s --baudrate 115200 /dev/serial0

Get parameters:

	> #sertool -g /dev/serial0

Clear rx software fifo:

	> #sertool -c /dev/serial0

Send msg:

	> #sertool --send-msg Thisisamsg /dev/serial0

Receive msg (receive 8 bytes):

	> #sertool --rcv-msg 8 /dev/serial0

### TO DO

- [x] Readme.md
- [x] Makefile portable (should work native and cross build)
- [x] travis.yml
- [ ] Add fifo configuration
