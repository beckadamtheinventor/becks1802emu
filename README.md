
# Beck's 1802 Emulator

Just a simple emulator for the CDP-1802 microprocessor in a particular configuration.


# Usage

This program must be run from the same directory as the `data` subfolder.

Windows:
`1802emu.exe rom.bin`

Linux:
`./1802emu rom.bin`

Right-click on the screen to reset. Should otherwise be fairly straightforward.

# Compiling

To compile this you will need to install Raylib (https://www.raylib.com) and GNU Make.


# Features

The 1802 could only access 64K of address space at a given time without paging.
This emulated system has two banks of 4Kb each, at addresses 0xC000-0xCFFF and 0xD000 to 0xDFFF respectively.
The bank selection addresses are at address 0xEA17 and 0xEA18 respectively.
This emulated system displays to a TTY of 64x50 characters, pointed to by the word at 0xFFFE-0xFFFF.
Each character of the TTY is two bytes. The first byte is the character code, the second byte is the color.
The character's color is encoded into the byte like this: XXrrggbb. Two bits red, green, and blue.

