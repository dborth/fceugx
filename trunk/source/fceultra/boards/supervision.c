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

#include "mapinc.h"

#define CHRRAM (GameMemBlock+16)

static void DoSuper(void)
{
 setprg8r((GameMemBlock[0]&0xC)>>2,0x6000,((GameMemBlock[0]&0x3)<<4)|0xF);
 if(GameMemBlock[0]&0x10)
 {
  setprg16r((GameMemBlock[0]&0xC)>>2,0x8000,((GameMemBlock[0]&0x3)<<3)|(GameMemBlock[1]&7));
  setprg16r((GameMemBlock[0]&0xC)>>2,0xc000,((GameMemBlock[0]&0x3)<<3)|7);
 }
 else
  setprg32r(4,0x8000,0);

 setmirror(((GameMemBlock[0]&0x20)>>5)^1);
}

static DECLFW(SuperWrite)
{
 if(!(GameMemBlock[0]&0x10))
 {
  GameMemBlock[0]=V;
  DoSuper();
 }
}

static DECLFW(SuperHi)
{
 GameMemBlock[1]=V; 
 DoSuper();
}

static void SuperReset(void)
{
  SetWriteHandler(0x6000,0x7FFF,SuperWrite);
  SetWriteHandler(0x8000,0xFFFF,SuperHi);
  SetReadHandler(0x6000,0xFFFF,CartBR);  
  GameMemBlock[0]=GameMemBlock[1]=0;
  setprg32r(4,0x8000,0);
  setvram8(CHRRAM);
  FCEU_dwmemset(CHRRAM,0,8192);
}

static void SuperRestore(int version)
{
 DoSuper();
}

void Supervision16_Init(CartInfo *info)
{
  AddExState(&GameMemBlock[0], 1, 0,"L1");
  AddExState(&GameMemBlock[1], 1, 0,"L2");
  AddExState(CHRRAM, 8192, 0, "CHRR");
  info->Power=SuperReset;
  GameStateRestore=SuperRestore;
}
