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

## Authors
The following code was written by [Jessica Hannebert](https://github.com/Jessicat-H), [Dylan Chapell](https://github.com/dylanchapell), and [Tony Mastromarino](https://github.com/tonydoesathing).
