# nic_link
The link layer for CP341: Computer Networking.

## Introduction
This C project establishes the link layer for the custom CP341 network.
- `nic_lib.c` is the library file that should be imported to provide sending and receiving functions.
- `nic_lib.h` is the header file for the above module.
- `multiuser_library_chat.c` is an example CLI that makes use of the module to interactively send and receive between the four Raspberry Pis.

## Building and Running
Keeping with the standards of the class, this code should be built and run on a Raspberry Pi 3B+ with [pigpio](https://abyz.me.uk/rpi/pigpio/index.html) installed. 
To build the CLI, run `gcc -pthread -o multichat multiuser_library_chat.c nic_lib.c -lpigpiod_if2`.
To run the CLI, then call `./multichat`.

## Bit sending and receiving
The bits in the messages are encoded using Manchester code. This technique consists of changing the signal from low to high for a 1, and high to low for a 0, at regular intervals, with additional changes in between those intervals as needed. Because of the way the bits are encoded, a long pulse signifies a change in level (1 to 0 or 0 to 1), whereas a short pulse signifies staying at the same level. This technique is especially useful because the clock timings are encoded in the signal, thus reducing the difficulty of clock synchronization.

## Bit organization
Each message is transmitted byte by byte. Before each byte is transmitted, it is preceded by a brief handshake (half an interval of high signal, a whole interval of low signal, and another half interval of high signal) to calibrate the receiver. The first byte of each message is the length of the rest of the message.

## Wiring for 4-user chat
In order to wire together four Raspberry Pis with four NICs to be able to communicate between all of them, do the following:
- Connect each Raspberry Pi to an NIC via a ribbon cable, and to a keyboard, monitor and power source via their respective cables.
- Connect each NIC to each other NIC. The easiest method for this is as follows:
	- Number the NICs with the numbers 1-4.
	- Connect each NIC to each other NIC using the port corresponding to its number. For example, connect the recv side of Port 1 on Pi #4 to the send side of Port 4 on Pi #1, and vice versa.
	- Each NIC should have nothing connected to the port matching its own number.
- Finally, start the aforementioned `multichat` executable on each Pi, type a username to send messages from, and start sending!

## Authors
The following code was written by [Jessica Hannebert](https://github.com/Jessicat-H), [Dylan Chapell](https://github.com/dylanchapell), and [Tony Mastromarino](https://github.com/tonydoesathing).
