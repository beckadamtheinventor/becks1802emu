
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "1802core.h"

Emu1802 *emu_Init1802(void) {
	Emu1802 *emu;
	if ((emu = malloc(sizeof(Emu1802))) == NULL)
		return NULL;
	memset(emu, 0, sizeof(Emu1802));
	
	if ((emu->mem = malloc(EMU_MEM_SIZE)) == NULL) {
		free(emu);
		return NULL;
	}
	if ((emu->banks = malloc(EMU_NUM_BANKS * sizeof(uint8_t*))) == NULL) {
		free(emu->mem);
		free(emu);
		return NULL;
	}
	for (int i=0; i<EMU_NUM_BANKS; i++) {
		if ((emu->banks[i] = malloc(EMU_BANK_SIZE)) == NULL) {
			for (;i>=0;--i)
				free(emu->banks[i]);
			free(emu->banks);
			free(emu->mem);
			free(emu);
			return NULL;
		}
		memset(emu->banks[i], 0, EMU_BANK_SIZE);
	}
	return emu;
}

void emu_LoadRom(Emu1802 *emu, uint8_t *rom, unsigned int len) {
	emu->rom = rom;
	emu->romlen = len;
	memcpy(emu->mem, rom, len>EMU_MEM_SIZE?EMU_MEM_SIZE:len);
	if (len > EMU_MEM_SIZE) {
		int i, remaining;
		remaining = len - EMU_MEM_SIZE;
		for (i=0; i<256; i++) {
			memcpy(emu->banks[i], &rom[EMU_MEM_SIZE + i*EMU_BANK_SIZE], remaining>EMU_BANK_SIZE?EMU_BANK_SIZE:remaining);
			remaining -= EMU_BANK_SIZE;
		}
		if (remaining < 0)
			memset(&emu->banks[i-1][EMU_BANK_SIZE-remaining], 0xFF, -remaining);
		for (; i<256; i++)
			memset(emu->banks[i], 0xFF, EMU_BANK_SIZE);
	}
}

int emu_LoadRomFile(Emu1802 *emu, const char *path) {
	FILE *fd;
	unsigned int len;
	uint8_t *rom;
	if ((fd = fopen(path, "rb")) == NULL)
		return -1;
	fseek(fd, 0, 2);
	len = ftell(fd);
	fseek(fd, 0, 0);
	if ((rom = malloc(len)) == NULL) {
		return 1;
	}
	fread(rom, len, 1, fd);
	emu->romShouldBeFreed = true;
	emu_LoadRom(emu, rom, len);
	// fread(emu->mem, len>EMU_MEM_SIZE?EMU_MEM_SIZE:len, 1, fd);
	// if (len > EMU_MEM_SIZE) {
		// int i, remaining;
		// remaining = len - EMU_MEM_SIZE;
		// for (i=0; i<256; i++) {
			// fread(emu->banks[i], remaining>EMU_BANK_SIZE?EMU_BANK_SIZE:remaining, 1, fd);
			// remaining -= EMU_BANK_SIZE;
		// }
		// if (remaining < 0)
			// memset(&emu->banks[i-1][EMU_BANK_SIZE-remaining], 0xFF, -remaining);
		// for (; i<256; i++)
			// memset(emu->banks[i], 0xFF, EMU_BANK_SIZE);
	// }
	fclose(fd);
	return 0;
}

void emu_Reset(Emu1802 *emu) {
	emu->D = emu->P = emu->T = emu->X = 0;
	emu->Q = emu->IE = emu->DF = false;
	memset(emu->regs, 0, sizeof(emu->regs));
	emu_LoadRom(emu, emu->rom, emu->romlen);
}

void emu_Input(Emu1802 *emu, uint8_t keycode) {
	emu_WriteByte(emu, EMU_KEYCODE_ADDR, keycode);
}

uint8_t emu_ReadByte(Emu1802 *emu, uint16_t address) {
	if (address >= EMU_BANK1_START && address <= EMU_BANK1_END) {
		return emu->banks[emu->mem[EMU_BANK1_SEL_ADDR]][address - EMU_BANK1_START];
	} else if (address >= EMU_BANK2_START && address <= EMU_BANK2_END) {
		return emu->banks[emu->mem[EMU_BANK2_SEL_ADDR]][address - EMU_BANK2_START];
	} else {
		return emu->mem[address];
	}
}

uint16_t emu_ReadWord(Emu1802 *emu, uint16_t address) {
	return (emu_ReadByte(emu, address) << 8) | emu_ReadByte(emu, address+1);
}

void emu_WriteByte(Emu1802 *emu, uint16_t address, uint8_t value) {
	if (!(address >= EMU_BANK1_START && address <= EMU_BANK2_END))
		emu->mem[address] = value;
}

void emu_Step(Emu1802 *emu, unsigned int cycles) {
	for(unsigned int i3 = 0; i3 < cycles; i3++) {
		//Fetch next instruction
		uint8_t opcode, highnibble, N;
		opcode = emu->mem[emu->regs[emu->P]++];

		//Decode and execute
		highnibble = opcode & 0xF0;
		N = opcode & 0x0F;
		if(highnibble == 0x00) {
			if(opcode == 0x00) { //IDL
				emu->status = emu->status | EMU_IDL;
				break;
			} else { //LDN
				emu->D = emu_ReadByte(emu, emu->regs[N]);
			}
		} else if(highnibble == 0x40) { //LDA
			emu->D = emu_ReadByte(emu, emu->regs[N]);
			emu->regs[N]++;
		} else if(opcode == 0xF0) { //LDX
			emu->D = emu_ReadByte(emu, emu->regs[emu->X]);
		} else if(opcode == 0x72) { //LDXA
			emu->D = emu_ReadByte(emu, emu->regs[emu->X]);
			emu->regs[emu->X]++;
		} else if(opcode == 0xF8) { //LDI
			emu->D = emu_ReadByte(emu, emu->regs[emu->P]++);
		} else if(highnibble == 0x50) { //STR
			emu_WriteByte(emu, emu->regs[N], emu->D);
		} else if(opcode == 0x73) { //STXD
			emu_WriteByte(emu, emu->regs[emu->X], emu->D);
			emu->regs[emu->X]--;
		} else if(highnibble == 0x10) { //INC
			emu->regs[N]++;
		} else if(highnibble == 0x20) { //DEC
			emu->regs[N]--;
		} else if(opcode == 0x60) { //IRX
			emu->regs[emu->X]++;
		} else if(highnibble == 0x80) { //GLO
			emu->D = emu->regs[N] & 0xFF;
		} else if(highnibble == 0xA0) { //PLO
			emu->regs[N] = (emu->regs[N] & 0xFF00) | emu->D;
		} else if(highnibble == 0x90) { //GHI
			emu->D = emu->regs[N] >> 8;
		} else if(highnibble == 0xB0) { //PHI
			emu->regs[N] = (emu->regs[N] & 0x00FF) | (emu->D << 8);
		} else if(opcode == 0xF1) { //OR
			emu->D |= emu_ReadByte(emu, emu->regs[emu->X]);
		} else if(opcode == 0xF9) { //ORI
			emu->D |= emu_ReadByte(emu, emu->regs[emu->P]++);
		} else if(opcode == 0xF3) { //XOR
			emu->D ^= emu_ReadByte(emu, emu->regs[emu->X]);
		} else if(opcode == 0xFB) { //XRI
			emu->D ^= emu_ReadByte(emu, emu->regs[emu->P]++);
		} else if(opcode == 0xF2) { //AND
			emu->D &= emu_ReadByte(emu, emu->regs[emu->X]);
		} else if(opcode == 0xFA) { //ANI
			emu->D &= emu_ReadByte(emu, emu->regs[emu->P]++);
		} else if(opcode == 0xF6) { //SHR
			emu->DF = emu->D & 1;
			emu->D >>= 1;
		} else if(opcode == 0x76) { //SHRC,RSHR
			unsigned int z = emu->D & 1;
			emu->D >>= 1;
			emu->D |= emu->DF << 7;
			emu->DF = z;
		} else if(opcode == 0xFE) { //SHL
			emu->DF = (emu->D & 128) >> 7;
			emu->D <<= 1;
		} else if(opcode == 0x7E) { //SHLC,RSHL
			int z = (emu->D & 128) >> 7;
			emu->D <<= 1;
			emu->D |= emu->DF;
			emu->DF = z;
		} else if(opcode == 0xF4) { //ADD
			uint16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = emu->D + v;
			emu->DF = (nD > 255);
			emu->D = nD & 0xFF;
		} else if(opcode == 0xFC) { //ADI
			uint16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = emu->D + v;
			emu->DF = (nD > 255);
			emu->D = nD & 0xFF;
		} else if(opcode == 0x74) { //ADC
			uint16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = emu->D + v + emu->DF;
			emu->DF = (nD > 255);
			emu->D = nD & 0xFF;
		} else if(opcode == 0x7C) { //ADCI
			uint16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = emu->D + v + emu->DF;
			emu->DF = (nD > 255);
			emu->D = nD & 0xFF;
		} else if(opcode == 0xF5) { //SD
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = v - emu->D;
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD += 256;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0xFD) { //SDI
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = v - emu->D;
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD += 256;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0x75) { //SDB
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = v - emu->D - (!emu->DF);
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD += 256;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0x7D) { //SDBI
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = v - emu->D - (!emu->DF);
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD += 256;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0xF7) { //SM
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = emu->D - v;
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD = 256 + nD;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0xFF) { //SMI
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = emu->D - v;
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD += 256;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0x77) { //SMB
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->X]);
			nD = emu->D - v - (!emu->DF);
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD = 256 + nD;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0x7F) { //SMBI
			int16_t nD;
			uint8_t v = emu_ReadByte(emu, emu->regs[emu->P]++);
			nD = emu->D - v - (!emu->DF);
			emu->DF = 1;
			if(nD < 0) {
				emu->DF = 0;
				nD = 256 + nD;
			}
			emu->D = nD & 0xFF;
		} else if(opcode == 0xC4) { //NOP
			//Nothing, lol
		} else if(highnibble == 0xD0) { //SEP
			emu->P = N;
		} else if(highnibble == 0xE0) { //SEX
			emu->X = N;
		} else if(opcode == 0x7B) { //SEQ
			emu->Q = true;
		} else if(opcode == 0x7A) { //REQ
			emu->Q = false;
		} else if(opcode == 0x78) { //SAV
			emu_WriteByte(emu, emu->regs[emu->X], emu->T);
		} else if(opcode == 0x79) { //MARK
			emu_WriteByte(emu, emu->regs[2]--, (emu->T = (emu->X << 4) | emu->P));
			emu->X = emu->P;
		} else if(opcode == 0x70 || opcode == 0x71) { //RET,DIS
			emu->T = emu_ReadByte(emu, emu->regs[emu->X]++);
			emu->X = emu->T >> 4;
			emu->P = emu->T & 15;
			emu->IE = opcode == 0x70 ? true : false;
		} else if(opcode >= 0x61 && opcode <= 0x67) { //OUT instructions - currently do nothing but increment the stack pointer
			emu->regs[emu->X]++;
		} else if(opcode == 0x68) { //CDP1805/1806 extended instructions. TODO
			uint8_t ext_opcode = emu_ReadByte(emu, emu->regs[emu->P]++);
			if (ext_opcode == 0xE0) break;
		} else {
			//These next instructions are all control flow instructions
			int br = 4; //Result of branch operation. 0 = skip, 1 = long-skip, 2 = short branch, 3 = long branch, 4 = no change (continue execution at next byte)
			//Short-branches
			if(opcode == 0x30) br = 2; //BR
			else if(opcode == 0x32) { if(emu->D == 0) { br = 2; } else { br = 0; } } //BZ
			else if(opcode == 0x3A) { if(emu->D != 0) { br = 2; } else { br = 0; } } //BNZ
			else if(opcode == 0x33) { if(emu->DF != 0) { br = 2; } else { br = 0; } } //BDF
			else if(opcode == 0x3B) { if(emu->DF == 0) { br = 2; } else { br = 0; } } //BNF
			else if(opcode == 0x31) { if(emu->Q != 0) { br = 2; } else { br = 0; } } //BQ
			else if(opcode == 0x39) { if(emu->Q == 0) { br = 2; } else { br = 0; } } //BNQ
			//Short-skip
			else if(opcode == 0x38) br = 0; //NBR,SKP
			//Long-branches
			else if(opcode == 0xC0) br = 3; //LBR
			else if(opcode == 0xC2) { if(emu->D == 0) { br = 3; } else { br = 1; } } //LBZ
			else if(opcode == 0xCA) { if(emu->D != 0) { br = 3; } else { br = 1; } } //LBNZ
			else if(opcode == 0xC3) { if(emu->DF != 0) { br = 3; } else { br = 1; } } //LBDF
			else if(opcode == 0xCB) { if(emu->DF == 0) { br = 3; } else { br = 1; } } //LBNF
			else if(opcode == 0xC1) { if(emu->Q != 0) { br = 3; } else { br = 1; } } //LBQ
			else if(opcode == 0xC9) { if(emu->Q == 0) { br = 3; } else { br = 1; } } //LBNQ
			//Long-skips
			else if(opcode == 0xC8) br = 1; //NLBR,LSKP
			else if(opcode == 0xCE && emu->D == 0) br = 1; //LSZ
			else if(opcode == 0xC6 && emu->D != 0) br = 1; //LSNZ
			else if(opcode == 0xCF && emu->DF != 0) br = 1; //LSDF
			else if(opcode == 0xC7 && emu->DF == 0) br = 1; //LSNF
			else if(opcode == 0xCD && emu->Q != 0) br = 1; //LSQ
			else if(opcode == 0xC5 && emu->Q == 0) br = 1; //LSNQ
			else if(opcode == 0xCC && emu->IE != 0) br = 1; //LSIE
			else br = 4;

			if(br == 0) emu->regs[emu->P]++;
			else if(br == 1) emu->regs[emu->P] += 2;
			else if(br == 2) {
				emu->regs[emu->P] = (emu->regs[emu->P] & 0xFF00) | emu_ReadByte(emu, emu->regs[emu->P]);
			} else if(br == 3) {
				emu->regs[emu->P] = emu_ReadWord(emu, emu->regs[emu->P]);
			}
		}
	}
}
