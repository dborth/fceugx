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



static DECLFW(Mapper57_write)
{
 A&=0x8800;
 if(A==0x8800)
 {
  mapbyte1[0]=V;
  if(V&0x80)
   ROM_BANK32(2|(V>>6));
  else
  {
   ROM_BANK16(0x8000,(V>>5)&3);
   ROM_BANK16(0xc000,(V>>5)&3);
  }
  MIRROR_SET((V&0x8)>>3);
 }
 else
  mapbyte1[1]=V;
 VROM_BANK8((mapbyte1[1]&3)|(mapbyte1[0]&7)|((mapbyte1[0]&0x10)>>1));
 //printf("$%04x:$%02x\n",A,V);
}

void Mapper57_init(void)
{
 SetWriteHandler(0x8000,0xffff,Mapper57_write);
}
