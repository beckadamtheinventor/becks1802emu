
#ifndef __1802CORE_h__
#define __1802CORE_h__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define EMU_MEM_SIZE 65536
#define EMU_NUM_BANKS 256
#define EMU_BANK_SIZE 4096

#define EMU_BANK1_SEL_ADDR     0xEA17
#define EMU_BANK1_START        0xC000
#define EMU_BANK1_END          0xCFFF
#define EMU_BANK2_SEL_ADDR     0xEA18
#define EMU_BANK2_START        0xD000
#define EMU_BANK2_END          0xDFFF
#define EMU_KEYCODE_ADDR       0xFFF9
#define EMU_TERMINAL_ADDR_LOC  0xFFFE

#define EMU_INIT 1
#define EMU_RUNNING 2
#define EMU_IDL 4
#define EMU_INT_TRIG 8
#define EMU_STAGE 16

typedef struct Emu1802 {
	uint8_t D, P, T, X, status;
	bool Q, IE, DF, romShouldBeFreed;
	uint16_t regs[16];
	unsigned int romlen;
	uint8_t *rom, *mem, **banks;
} Emu1802;

Emu1802 *emu_Init1802(void);
void emu_LoadRom(Emu1802 *emu, uint8_t *rom, unsigned int len);
int emu_LoadRomFile(Emu1802 *emu, const char *path);
void emu_Reset(Emu1802 *emu);
void emu_Input(Emu1802 *emu, uint8_t keycode);
uint8_t emu_ReadByte(Emu1802 *emu, uint16_t address);
uint16_t emu_ReadWord(Emu1802 *emu, uint16_t address);
void emu_WriteByte(Emu1802 *emu, uint16_t address, uint8_t value);
void emu_Step(Emu1802 *emu, unsigned int cycles);

#endif
