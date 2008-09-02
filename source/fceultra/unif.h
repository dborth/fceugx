/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

void TCU01_Init(CartInfo *info);
void S8259B_Init(CartInfo *info);
void S8259A_Init(CartInfo *info);
void S74LS374N_Init(CartInfo *info);
void SA0161M_Init(CartInfo *info);

void SA72007_Init(CartInfo *info);
void SA72008_Init(CartInfo *info);
void SA0036_Init(CartInfo *info);
void SA0037_Init(CartInfo *info);

void H2288_Init(CartInfo *info);
void UNL8237_Init(CartInfo *info);

void HKROM_Init(CartInfo *info);

void ETROM_Init(CartInfo *info);
void EKROM_Init(CartInfo *info);
void ELROM_Init(CartInfo *info);
void EWROM_Init(CartInfo *info);

void SAROM_Init(CartInfo *info);
void SBROM_Init(CartInfo *info);
void SCROM_Init(CartInfo *info);
void SEROM_Init(CartInfo *info);
void SGROM_Init(CartInfo *info);
void SKROM_Init(CartInfo *info);
void SLROM_Init(CartInfo *info);
void SL1ROM_Init(CartInfo *info);
void SNROM_Init(CartInfo *info);
void SOROM_Init(CartInfo *info);

void NROM_Init(CartInfo *info);
void NROM256_Init(CartInfo *info);
void NROM128_Init(CartInfo *info);
void MHROM_Init(CartInfo *info);
void UNROM_Init(CartInfo *info);
void MALEE_Init(CartInfo *info);
void Supervision16_Init(CartInfo *info);
void Super24_Init(CartInfo *info);
void Novel_Init(CartInfo *info);
void CNROM_Init(CartInfo *info);
void CPROM_Init(CartInfo *info);
void GNROM_Init(CartInfo *info);

void TEROM_Init(CartInfo *info);
void TFROM_Init(CartInfo *info);
void TGROM_Init(CartInfo *info);
void TKROM_Init(CartInfo *info);
void TSROM_Init(CartInfo *info);
void TLROM_Init(CartInfo *info);
void TLSROM_Init(CartInfo *info);
void TKSROM_Init(CartInfo *info);
void TQROM_Init(CartInfo *info);
void TQROM_Init(CartInfo *info);

#define UNIFMemBlock (GameMemBlock+32768)

extern uint8 *UNIFchrrama;	// Meh.  So I can stop CHR RAM 
	 			// bank switcherooing with certain boards...
