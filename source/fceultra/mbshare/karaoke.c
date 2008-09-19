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

static uint8 DReg;
static SFORMAT StateRegs[]=
{
	{&DReg, 1, "DREG"},
	{0}
};

static void Sync(void)
{
 //if(!DReg)
 //printf("%02x\n",DReg);
 if(DReg)
 {
  if(DReg & 0x10)
   setprg16(0x8000, (DReg & 0x7));
  else
   setprg16(0x8000, (DReg&0x7) | 0x8);
 }
 else
  setprg16(0x8000, 0x7);
}

static void StateRestore(int version)
{
 Sync();
}

static DECLFW(M188Write)
{
 DReg = V;
 Sync();
}

static DECLFR(testr)
{
 return(3);
}


static void Power(void)
{
 setchr8(0);
 setprg8(0xc000,0xE);
 setprg8(0xe000,0xF);
 DReg = 0;
 Sync();
 SetReadHandler(0x6000,0x7FFF,testr);
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x8000,0xFFFF,M188Write);
}


void Mapper188_Init(CartInfo *info)
{
 info->Power=Power;
 GameStateRestore=StateRestore;
 AddExState(&StateRegs, ~0, 0, 0);
}
