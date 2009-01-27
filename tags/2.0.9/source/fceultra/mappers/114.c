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

#define master mapbyte3[0]
#define incmd mapbyte3[1]

static void dochr(void)
{
  VROM_BANK2(0x0000,(mapbyte2[0]>>1));
  VROM_BANK2(0x0800,(mapbyte2[2]>>1));
  VROM_BANK1(0x1000,mapbyte2[6]); 
  VROM_BANK1(0x1400,mapbyte2[1]); 
  VROM_BANK1(0x1800,mapbyte2[7]);
  VROM_BANK1(0x1c00,mapbyte2[3]);
}

void doprg()
{
 if(master&0x80)
 {
  ROM_BANK16(0x8000,master&0x1F);
 }
 else
 {
  ROM_BANK8(0x8000,mapbyte2[4]);
  ROM_BANK8(0xa000,mapbyte2[5]);
 }

}

static DECLFW(Mapper114_write)
{
 if(A<=0x7FFF)
 {
  master=V;
  doprg();
 }
 else if(A==0xe003) IRQCount=V;
 else if(A==0xe002) X6502_IRQEnd(FCEU_IQEXT);
 else switch(A&0xE000)
 {
  case 0x8000:MIRROR_SET(V&1);break;
  case 0xa000:mapbyte1[0]=V;incmd=1;break;
  case 0xc000:
	      if(!incmd) break;
	      mapbyte2[mapbyte1[0]&0x7]=V;
	      switch(mapbyte1[0]&0x7)
	      {
	       case 0x0: case 1: case 2: case 3: case 6: case 7: 
		dochr();break;
	       case 0x4:
	       case 0x5:doprg();break;
	      }
		incmd=0;
		break;
 }

}

static void Mapper114_hb(void)
{
 if(IRQCount)
 {
  IRQCount--;
  if(!IRQCount)
  {
   X6502_IRQBegin(FCEU_IQEXT);
   //printf("IRQ: %d\n",scanline);
  }
 }

}

void Mapper114_init(void)
{
  GameHBIRQHook=Mapper114_hb;
  SetWriteHandler(0x6000,0xffff,Mapper114_write);
  SetReadHandler(0x4020,0x7fff,0);
}

