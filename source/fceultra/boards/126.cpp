/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mapinc.h"
#include "mmc3.h"

static uint16 GetExChrExBank() {
	uint16 bank = (uint16)EXPREGS[0];
	return ((~bank << 0 & 0x080 & ((uint16)EXPREGS[2])) | (bank << 4 & 0x080 & bank) |
			(bank << 3 & 0x100) | (bank << 5 & 0x200));
}

static void UpdateChrBank(void) {
	uint16 bank = (uint16)EXPREGS[2] & 0x0F;
	bank |= GetExChrExBank() >> 3;
	setchr8(bank);
}

static void M126CW(uint32 A, uint8 V) {
	if (!(EXPREGS[3] & 0x10)) {
		uint16 bank = (uint16)V & (((uint16)EXPREGS[0] & 0x80) - 1);
		bank |= GetExChrExBank();
		setchr1(A, bank);
	}
}

static void M126PW(uint32 A, uint8 V) {
	uint16 bank = (uint16)V;
	uint16 preg = (uint16)EXPREGS[0];
	bank &= ((~preg >> 2) & 0x10) | 0x0F;
	bank |= ((preg & (0x06 | (preg & 0x40) >> 6)) << 4) | ((preg & 0x10) << 3);
	if (!(EXPREGS[3] & 0x03))
		setprg8(A, bank);
	else if ((A - 0x8000) == ((MMC3_cmd << 8) & 0x4000)) { /* TODO: Clean this */
		if ((EXPREGS[3] & 0x3) == 0x3)
			setprg32(0x8000, (bank >> 2));
		else {
			setprg16(0x8000, (bank >> 1));
			setprg16(0xc000, (bank >> 1));
		}
	}
}

static DECLFW(M126Write) {
	A &= 0x03;
	if(A == 0x01 || A == 0x02 || ((A == 0x00 || A == 0x03) && !(EXPREGS[3] & 0x80))) {
		if (EXPREGS[A] != V) {
			EXPREGS[A] = V;
			if (EXPREGS[3] & 0x10)
				UpdateChrBank();
			else
				FixMMC3CHR(MMC3_cmd);
			FixMMC3PRG(MMC3_cmd);
		}
	}
}

void M126StateRestore(int v) {
	FixMMC3CHR(MMC3_cmd);
	FixMMC3PRG(MMC3_cmd);

	if (EXPREGS[3] & 0x10)
		UpdateChrBank();
}

static void M126Reset(void) {
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	MMC3RegReset();
}

static void M126Power(void) {
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M126Write);
}

void Mapper126_Init(CartInfo *info) {
	GenMMC3_Init(info, 2048, 1023, 0, 0);
	cwrap = M126CW;
	pwrap = M126PW;
	info->Power = M126Power;
	info->Reset = M126Reset;
	GameStateRestore = M126StateRestore;
	AddExState(EXPREGS, 4, 0, "EXPR");
}
