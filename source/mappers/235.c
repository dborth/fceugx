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

static DECLFW(Mapper235_write)
{
  uint32 m;
  int z;

  if(A&0x400)
   onemir(0);
  else
   setmirror(((A>>13)&1)^1);
  m=A&0x1f;

  z=(A>>8)&3;

   if(A&0x800)
   {
    setprg16r(0x10|z,0x8000,(m<<1)|((A>>12)&1));
    setprg16r(0x10|z,0xC000,(m<<1)|((A>>12)&1));
   }
   else
    setprg32r(0x10|z,0x8000,m);
}

void Mapper235_init(void)
{
 /* Fixme:  Possible crashy bug if header is bad. */
 SetupCartPRGMapping(0x10,PRGptr[0],1024*1024,0);
 SetupCartPRGMapping(0x12,PRGptr[0]+1024*1024,1024,0);
 setprg32r(0x10,0x8000,0);
 SetReadHandler(0x8000,0xffff,CartBROB);
 SetWriteHandler(0x8000,0xffff,Mapper235_write);
}
