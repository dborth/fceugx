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

static int mn;
static DECLFW(Mapper88_write)
{
 //if(A>=0x8002 || A<0x8000)
 //if(A==0xc000)
 // printf("$%04x:$%02x\n",A,V);
 switch(A&0x8001) //&0xc001)
 {
  case 0x8000:mapbyte1[0]=V;
	      if(mn)
	       onemir((V>>6)&1);
	      break;
  case 0x8001:
	      switch(mapbyte1[0]&7)
	      {
	       case 0:VROM_BANK2(0,V>>1);break;
	       case 1:VROM_BANK2(0x800,V>>1);break;
	       case 2:VROM_BANK1(0x1000,V|0x40);break;
	       case 3:VROM_BANK1(0x1400,V|0x40);break;
	       case 4:VROM_BANK1(0x1800,V|0x40);break;
	       case 5:VROM_BANK1(0x1c00,V|0x40);break;
	       case 6:ROM_BANK8(0x8000,V);break;
	       case 7:ROM_BANK8(0xA000,V);break;
	      }
	      break;

 }
}

void Mapper88_init(void)
{
  mn=0;
  SetWriteHandler(0x8000,0xffff,Mapper88_write);
}

void Mapper154_init(void)
{
 mn=1;
 SetWriteHandler(0x8000,0xffff,Mapper88_write);
}
