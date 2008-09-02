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

static uint8 cmd;
static uint8 latch[8];
#define CHRRAM (GameMemBlock)

static void S74LS374NSynco(void)
{
 setprg32(0x8000,latch[0]);
 setchr8(latch[1]);
 setmirror(latch[2]&1);
// setchr8(6);
}

static DECLFW(S74LS374NWrite)
{
 //printf("$%04x:$%02x\n",A,V);
 A&=0x4101;
 if(A==0x4100)
  cmd=V&7;
 else 
 {
  switch(cmd)
  {
   case 0:latch[0]=0;latch[1]=3;break;
   case 4:latch[1]&=3;latch[1]|=(V<<2);break;
   case 5:latch[0]=V&0x7;break;
   case 6:latch[1]&=0x1C;latch[1]|=V&3;break;
   case 7:latch[2]=V&1;break;
  }
  S74LS374NSynco();
 }
}

static void S74LS374NReset(void)
{
 latch[0]=latch[2]=0;
 latch[1]=3;
 S74LS374NSynco();
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x4100,0x7FFF,S74LS374NWrite);
}

static void S74LS374NRestore(int version)
{
 S74LS374NSynco();
}

void S74LS374N_Init(CartInfo *info)
{
 info->Power=S74LS374NReset;
 GameStateRestore=S74LS374NRestore;
 AddExState(latch, 3, 0, "LATC");
 AddExState(&cmd, 1, 0, "CMD");
}

static int type;
static void S8259Synco(void)
{
 int x;

 setprg32(0x8000,latch[5]&7);

 if(!UNIFchrrama)	// No CHR RAM?  Then BS'ing is ok.
 {
  if(!type)
  {
   for(x=0;x<4;x++)
    setchr2(0x800*x,(x&1)|((latch[x]&7)<<1)|((latch[4]&7)<<4));
  }
  else
  {
   for(x=0;x<4;x++)
    setchr2(0x800*x,(latch[x]&0x7)|((latch[4]&7)<<3));
  }
 }
 switch((latch[7]>>1)&3)
 {
  case 0:setmirrorw(0,0,0,1);break;
  case 1:setmirror(MI_H);break;
  case 2:setmirror(MI_V);break;
  case 3:setmirror(MI_0);break;
 }
}

static DECLFW(S8259Write)
{
 A&=0x4101;
 if(A==0x4100) cmd=V;
 else 
 {
  latch[cmd&7]=V;
  S8259Synco();
 }
}

static void S8259Reset(void)
{
 int x;
 cmd=0;

 for(x=0;x<8;x++) latch[x]=0;
 if(UNIFchrrama) setchr8(0);

 S8259Synco();
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x4100,0x7FFF,S8259Write);
}

static void S8259Restore(int version)
{
 S8259Synco();
}

void S8259A_Init(CartInfo *info)
{
 info->Power=S8259Reset;
 GameStateRestore=S8259Restore;
 AddExState(latch, 8, 0, "LATC");
 AddExState(&cmd, 1, 0, "CMD");
 type=0;

 //if(!CHRsize[0])
 //{
 // SetupCartCHRMapping(0,CHRRAM,8192,1);
 // AddExState(CHRRAM, 8192, 0, "CHRR");
 //}
}

void S8259B_Init(CartInfo *info)
{
 info->Power=S8259Reset;
 GameStateRestore=S8259Restore;
 AddExState(latch, 8, 0, "LATC");
 AddExState(&cmd, 1, 0, "CMD");
 type=1;
}

static void(*WSync)(void);

static void SA0161MSynco()
{
 setprg32(0x8000,(latch[0]>>3)&1); 
 setchr8(latch[0]&7);
}

static DECLFW(SAWrite)
{
 if(A&0x100)
 {
  latch[0]=V;
  WSync();
 }
}

static void SAReset(void)
{
 latch[0]=0;
 WSync();
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x4100,0x5FFF,SAWrite);
}

static void SA0161MRestore(int version)
{
 SA0161MSynco();
}

void SA0161M_Init(CartInfo *info)
{
 WSync=SA0161MSynco;
 GameStateRestore=SA0161MRestore;
 info->Power=SAReset;
 AddExState(&latch[0], 1, 0, "LATC");
}

static void SA72007Synco()
{
 setprg32(0x8000,0);
 setchr8(latch[0]>>7);
}

static void SA72007Restore(int version)
{
 SA72007Synco();
}

void SA72007_Init(CartInfo *info)
{
 WSync=SA72007Synco;
 GameStateRestore=SA72007Restore;
 info->Power=SAReset;
 AddExState(&latch[0], 1, 0, "LATC");
}

static void SA72008Synco()
{
 setprg32(0x8000,(latch[0]>>2)&1);
 setchr8(latch[0]&3);
}

static void SA72008Restore(int version)
{
 SA72008Synco();
}

void SA72008_Init(CartInfo *info)
{
 WSync=SA72008Synco;
 GameStateRestore=SA72008Restore;
 info->Power=SAReset;
 AddExState(&latch[0], 1, 0, "LATC");
}

static DECLFW(SADWrite)
{
 latch[0]=V;
 WSync();
}

static void SADReset(void)
{
 latch[0]=0;
 WSync();
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x8000,0xFFFF,SADWrite);
}

static void SA0036Synco()
{
 setprg32(0x8000,0);
 setchr8(latch[0]>>7);
}

static void SA0036Restore(int version)
{
 SA0036Synco();
}

static void SA0037Synco()
{
 setprg32(0x8000,(latch[0]>>3)&1);
 setchr8(latch[0]&7);
}

static void SA0037Restore(int version)
{
 SA0037Synco();
}

void SA0036_Init(CartInfo *info)
{
 WSync=SA0036Synco;
 GameStateRestore=SA0036Restore;
 info->Power=SADReset;
 AddExState(&latch[0], 1, 0, "LATC");
}

void SA0037_Init(CartInfo *info)
{
 WSync=SA0037Synco;
 GameStateRestore=SA0037Restore;
 info->Power=SADReset;
 AddExState(&latch[0], 1, 0, "LATC");
}

static void TCU01Synco()
{
 setprg32(0x8000,(latch[0]>>2)&1);
 setchr8((latch[0]>>3)&0xF);
}

static DECLFW(TCWrite)
{
 if((A&0x103)==0x102)
  latch[0]=V;
 TCU01Synco();
}

static void TCU01Reset(void)
{
 latch[0]=0;
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0x4100,0xFFFF,TCWrite);
 TCU01Synco();
}

static void TCU01Restore(int version)
{
 TCU01Synco();
}

void TCU01_Init(CartInfo *info)
{
 GameStateRestore=TCU01Restore;
 info->Power=TCU01Reset;
 AddExState(&latch[0], 1, 0, "LATC");
}

