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

#define cmd mapbyte1[0]
#define lpa mapbyte1[1]
#define prgl mapbyte2

static void PRGSynco(void)
{
 if(lpa&0x80)
 {
  ROM_BANK16(0x8000,lpa&0xF);
 }
 else
 {
  ROM_BANK8(0x8000,prgl[0]&0x1F);
  ROM_BANK8(0xa000,prgl[1]&0x1F);
 }
}

static DECLFW(Mapper248_writelow)
{
 lpa=V;
 PRGSynco();
}

static DECLFW(Mapper248_write)
{
 switch(A&0xF001)
 {
  case 0xa000:MIRROR_SET(V&1);break; // Not sure if this is right.  Mirroring may be hard wired...
  case 0xc000:IRQLatch=V;break;
  case 0xc001:IRQCount=IRQLatch;break;
  case 0xe000:IRQa=0;X6502_IRQEnd(FCEU_IQEXT);break;
  case 0xe001:IRQa=1;break;
  case 0x8000:cmd=V;break;
  case 0x8001:switch(cmd&7)
	      {
	       case 0:VROM_BANK2(0x000,V>>1);break;
	       case 1:VROM_BANK2(0x800,V>>1);break;
	       case 2:VROM_BANK1(0x1000,V);break;
	       case 3:VROM_BANK1(0x1400,V);break;
	       case 4:VROM_BANK1(0x1800,V);break;
	       case 5:VROM_BANK1(0x1c00,V);break;
	       case 6:prgl[0]=V;PRGSynco();break;
	       case 7:prgl[1]=V;PRGSynco();break;
	      }
	      break;
 }
}

static void Mapper248_hb(void)
{
 if(IRQa)
 {
         IRQCount--;
         if(IRQCount<0)
         {
	  X6502_IRQBegin(FCEU_IQEXT);
	  IRQCount=IRQLatch;
         }
 }
}

void Mapper248_init(void)
{
  SetWriteHandler(0x6000,0x6fff,Mapper248_writelow);
  SetWriteHandler(0x8000,0xffff,Mapper248_write);
  GameHBIRQHook=Mapper248_hb;
}

