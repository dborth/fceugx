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



DECLFW(Mapper112_write)
{
switch(A)
{
 case 0xe000:MIRROR_SET(V&1);break;
 case 0x8000:mapbyte1[0]=V;break;
 case 0xa000:switch(mapbyte1[0])
            {
            case 0:ROM_BANK8(0x8000,V);break;
            case 1:ROM_BANK8(0xA000,V);break;
                case 2: V&=0xFE;VROM_BANK1(0,V);
                        VROM_BANK1(0x400,(V+1));break;
                case 3: V&=0xFE;VROM_BANK1(0x800,V);
                        VROM_BANK1(0xC00,(V+1));break;
            case 4:VROM_BANK1(0x1000,V);break;
            case 5:VROM_BANK1(0x1400,V);break;
            case 6:VROM_BANK1(0x1800,V);break;
            case 7:VROM_BANK1(0x1c00,V);break;
            }
            break;
 }
}

void Mapper112_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper112_write);
}

