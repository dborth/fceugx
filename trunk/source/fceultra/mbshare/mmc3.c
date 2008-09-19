/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2003 Xodnizel
 *  Mapper 12 code Copyright (C) 2003 CaH4e3
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

/*  Code for emulating iNES mappers 4, 118,119 */

#include "mapinc.h"

static uint8 resetmode,MMC3_cmd,A000B,A001B;
static uint8 DRegBuf[8];

static uint8 *WRAM;
static uint8 *CHRRAM;
static uint32 CHRRAMSize;

static uint8 PPUCHRBus;
static uint8 TKSMIR[8];
static uint8 EXPREGS[8];	/* For bootleg games, mostly. */
#undef IRQCount
#undef IRQLatch
#undef IRQa
static uint8 IRQCount,IRQLatch,IRQa;
static uint8 IRQReload;

static SFORMAT MMC3_StateRegs[]={
	{DRegBuf, 8, "REGS"},
	{&resetmode, 1, "RMOD"},
	{&MMC3_cmd, 1, "CMD"},
	{&A000B, 1, "A000"},
	{&A001B, 1, "A001"},
	{&IRQReload, 1, "IRQR"},
	{&IRQCount, 1, "IRQC"},
	{&IRQLatch, 1, "IRQL"},
	{&IRQa, 1, "IRQA"},
	{0}
};


static void (*pwrap)(uint32 A, uint8 V);
static void (*cwrap)(uint32 A, uint8 V);
static void (*mwrap)(uint8 V);

static int mmc3opts=0;

void GenMMC3_Init(CartInfo *info, int prg, int chr, int wram, int battery);
static void GenMMC3Power(void);
static void FixMMC3PRG(int V);
static void FixMMC3CHR(int V);

static void FixMMC3PRG(int V)
{
          if(V&0x40)
          {
           pwrap(0xC000,DRegBuf[6]);
           pwrap(0x8000,~1);
          }
          else
          {
           pwrap(0x8000,DRegBuf[6]);
           pwrap(0xC000,~1);
          }
	  pwrap(0xA000,DRegBuf[7]);
	  pwrap(0xE000,~0);
}

static void FixMMC3CHR(int V)
{
           int cbase=(V&0x80)<<5;
           cwrap((cbase^0x000),DRegBuf[0]&(~1));
           cwrap((cbase^0x400),DRegBuf[0]|1);
           cwrap((cbase^0x800),DRegBuf[1]&(~1));
           cwrap((cbase^0xC00),DRegBuf[1]|1);

           cwrap(cbase^0x1000,DRegBuf[2]);
           cwrap(cbase^0x1400,DRegBuf[3]);
           cwrap(cbase^0x1800,DRegBuf[4]);
           cwrap(cbase^0x1c00,DRegBuf[5]);
}

static void MMC3RegReset(void)
{
 IRQCount=IRQLatch=IRQa=MMC3_cmd=0;

 DRegBuf[0]=0;
 DRegBuf[1]=2;
 DRegBuf[2]=4;
 DRegBuf[3]=5;
 DRegBuf[4]=6;
 DRegBuf[5]=7;
 DRegBuf[6]=0;
 DRegBuf[7]=1;

 FixMMC3PRG(0);
 FixMMC3CHR(0);
}

static DECLFW(Mapper4_write)
{
        switch(A&0xE001)
	{
         case 0x8000:
          if((V&0x40) != (MMC3_cmd&0x40))
	   FixMMC3PRG(V);
          if((V&0x80) != (MMC3_cmd&0x80))
           FixMMC3CHR(V);
          MMC3_cmd = V;
          break;

        case 0x8001:
                {
                 int cbase=(MMC3_cmd&0x80)<<5;
                 DRegBuf[MMC3_cmd&0x7]=V;
                 switch(MMC3_cmd&0x07)
                 {
                  case 0: cwrap((cbase^0x000),V&(~1));
			  cwrap((cbase^0x400),V|1);
			  break;
                  case 1: cwrap((cbase^0x800),V&(~1));
			  cwrap((cbase^0xC00),V|1);
			  break;
                  case 2: cwrap(cbase^0x1000,V); break;
                  case 3: cwrap(cbase^0x1400,V); break;
                  case 4: cwrap(cbase^0x1800,V); break;
                  case 5: cwrap(cbase^0x1C00,V); break;
                  case 6: if (MMC3_cmd&0x40) pwrap(0xC000,V);
                          else pwrap(0x8000,V);
                          break;
                  case 7: pwrap(0xA000,V);
                          break;
                 }
                }
                break;

        case 0xA000:
	        if(mwrap) mwrap(V&1);
                break;
	case 0xA001:
		A001B=V;
		break;
 }
}

static DECLFW(MMC3_IRQWrite)
{
        switch(A&0xE001)
        {
         case 0xc000:IRQLatch=V;break;
         case 0xc001:IRQReload = 1;break;
         case 0xE000:X6502_IRQEnd(FCEU_IQEXT);IRQa=0;break;
         case 0xE001:IRQa=1;break;
        }
}



static void ClockMMC3Counter(void)
{
  int count = IRQCount;
  if(!count || IRQReload)
  {
   IRQCount = IRQLatch;
   IRQReload = 0;
  }
  else IRQCount--;
  if(count && !IRQCount)
  {
   if(IRQa)
   {
    X6502_IRQBegin(FCEU_IQEXT);
   }
  }
}

static void MMC3_hb(void)
{
      ClockMMC3Counter();
}

static void MMC3_hb_KickMasterHack(void)
{
	if(scanline==238)
	 ClockMMC3Counter();
	ClockMMC3Counter();
}

static void MMC3_hb_PALStarWarsHack(void)
{
        if(scanline==240)
         ClockMMC3Counter();
        ClockMMC3Counter();
}
/*
static void ClockMMC6Counter(void)
{
  unsigned int count = IRQCount;

  if(!count || IRQReload)
  {
   IRQCount = IRQLatch;
   IRQReload = 0;
  }
  else IRQCount--;
  if(!IRQCount)
  {
   if(IRQa)
    X6502_IRQBegin(FCEU_IQEXT);
  }
}
*/

/*
static uint32 lasta;
static void FP_FASTAPASS(1) MMC3_PPUIRQ(uint32 A)
{
 if(A&0x2000) return;
 if((A&0x1000) && !(lasta&0x1000))
  ClockMMC3Counter();
 lasta = A;
}
*/

static void genmmc3restore(int version)
{
 mwrap(A000B&1);
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void GENCWRAP(uint32 A, uint8 V)
{
 setchr1(A,V);
}

static void GENPWRAP(uint32 A, uint8 V)
{
 setprg8(A,V&0x3F);
}

static void GENMWRAP(uint8 V)
{
 A000B=V;
 setmirror(V^1);
}

static void GENNOMWRAP(uint8 V)
{
 A000B=V;
}



static void M12CW(uint32 A, uint8 V)
{
 setchr1(A,(EXPREGS[(A&0x1000)>>12]<<8)+V);
}

static DECLFW(M12Write)
{
 EXPREGS[0]= V&0x01;
 EXPREGS[1]= (V&0x10)>>4;
}

static void M12_Power(void)
{
 EXPREGS[0]=EXPREGS[1]=0;
 GenMMC3Power();
 SetWriteHandler(0x4100,0x5FFF,M12Write);   
}

void Mapper12_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=M12CW;
 info->Power=M12_Power;
 AddExState(EXPREGS, 2, 0, "EXPR");
}

static void M47PW(uint32 A, uint8 V)
{
 V&=0xF;
 V|=EXPREGS[0]<<4;
 setprg8(A,V);
}

static void M47CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=EXPREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(M47Write)
{
 EXPREGS[0]=V&1;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd); 
}

static void M47_Power(void)
{
 EXPREGS[0]=0;
 GenMMC3Power();
 SetWriteHandler(0x6000,0x7FFF,M47Write);
 SetReadHandler(0x6000,0x7FFF,0);
}

void Mapper47_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 pwrap=M47PW;
 cwrap=M47CW;
 info->Power=M47_Power;
 AddExState(EXPREGS, 1, 0, "EXPR");
}

static void M44PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[0]>=6) NV&=0x1F;
 else NV&=0x0F;
 NV|=EXPREGS[0]<<4;
 setprg8(A,NV);
}

static void M44CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[0]<6) NV&=0x7F;
 NV|=EXPREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper44_write)
{
 if(A&1)
 {
  EXPREGS[0]=V&7;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
 else
  Mapper4_write(A,V);
}

static void M44_Power(void)
{
 EXPREGS[0]=0;
 GenMMC3Power();
 SetWriteHandler(0xA000,0xBFFF,Mapper44_write);
}

void Mapper44_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=M44CW;
 pwrap=M44PW;
 info->Power=M44_Power;
 AddExState(EXPREGS, 1, 0, "EXPR");
}

static void M52PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x1F^((EXPREGS[0]&8)<<1);
 NV|=((EXPREGS[0]&6)|((EXPREGS[0]>>3)&EXPREGS[0]&1))<<4;
 setprg8(A,NV);
}

static void M52CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0xFF^((EXPREGS[0]&0x40)<<1);
 NV|=(((EXPREGS[0]>>3)&4)|((EXPREGS[0]>>1)&2)|((EXPREGS[0]>>6)&(EXPREGS[0]>>4)&1))<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper52_write)
{
 if(EXPREGS[1]) 
 {
  WRAM[A-0x6000]=V;
  return;
 }
 EXPREGS[1]=1;
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M52Reset(void)
{
 EXPREGS[0]=EXPREGS[1]=0;
 MMC3RegReset(); 
}

static void M52Power(void)
{
 M52Reset();
 GenMMC3Power();
 SetWriteHandler(0x6000,0x7FFF,Mapper52_write);
}

void Mapper52_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=M52CW;
 pwrap=M52PW;
 info->Reset=M52Reset;
 info->Power=M52Power;
 AddExState(EXPREGS, 2, 0, "EXPR");
}

static void M45CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(EXPREGS[2]&8)
  NV&=(1<<( (EXPREGS[2]&7)+1 ))-1;
 else
  NV&=0;
 NV|=EXPREGS[0]|((EXPREGS[2]&0xF0)<<4); // &0x10(not 0xf0) is valid given the original
					// description of mapper 45 by kevtris,
					// but this fixes Super 8 in 1.
 setchr1(A,NV);
}

static void M45PW(uint32 A, uint8 V)
{
 //V=((V&(EXPREGS[3]^0xFF))&0x3f)|EXPREGS[1];
 V&=(EXPREGS[3]&0x3F)^0x3F;
 V|=EXPREGS[1];
 //printf("$%04x, $%02x\n",A,V);
 setprg8(A,V);
}

static DECLFW(Mapper45_write)
{
 //printf("$%02x, %d\n",V,EXPREGS[4]);
 if(EXPREGS[3]&0x40) 
 {
  WRAM[A-0x6000]=V;   
  return;
 }
 EXPREGS[EXPREGS[4]]=V;
 EXPREGS[4]=(EXPREGS[4]+1)&3;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M45Reset(void)
{
 FCEU_dwmemset(EXPREGS,0,5);
 MMC3RegReset();
}

static void M45Power(void)
{
 M45Reset();
 GenMMC3Power();
 SetWriteHandler(0x6000,0x7FFF,Mapper45_write);
}

void Mapper45_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);

 cwrap=M45CW;
 pwrap=M45PW;

 info->Reset=M45Reset;
 info->Power=M45Power;

 AddExState(EXPREGS, 5, 0, "EXPR");
}

static void M49PW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&1)
 {
  V&=0xF;
  V|=(EXPREGS[0]&0xC0)>>2;
  setprg8(A,V);
 }
 else
  setprg32(0x8000,(EXPREGS[0]>>4)&3);
}

static void M49CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=(EXPREGS[0]&0xC0)<<1;
 setchr1(A,NV);
}

static DECLFW(M49Write)
{
 //printf("$%04x:$%02x\n",A,V);
 if(A001B&0x80)
 {
  EXPREGS[0]=V;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
}

static void M49Reset(void)
{
 EXPREGS[0]=0;
 MMC3RegReset();
}

static void M49Power(void)
{
 M49Reset();
 GenMMC3Power();
 SetWriteHandler(0x6000,0x7FFF,M49Write);
 SetReadHandler(0x6000,0x7FFF,0);
}

void Mapper49_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 0, 0);
 cwrap=M49CW;
 pwrap=M49PW;
 info->Reset=M49Reset;
 info->Power=M49Power;

 AddExState(EXPREGS, 1, 0, "EXPR");
}

static void M115PW(uint32 A, uint8 V)
{
 setprg8(A,V);
 if(EXPREGS[0]&0x80)
 {
  setprg16(0x8000,EXPREGS[0]&7);
 }
}

static void M115CW(uint32 A, uint8 V)
{
 setchr1(A,(uint32)V|((EXPREGS[1]&1)<<8));
}

static DECLFW(M115Write)
{
 if(A==0x6000)
  EXPREGS[0]=V;
 else if(A==0x6001)
  EXPREGS[1]=V;
 FixMMC3PRG(MMC3_cmd);
}

static void M115Power(void)
{
 GenMMC3Power();
 SetWriteHandler(0x6000,0x7FFF,M115Write);
 SetReadHandler(0x6000,0x7FFF,0);
}

void Mapper115_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 0, 0);
 cwrap=M115CW;
 pwrap=M115PW;
 info->Power=M115Power;
 AddExState(EXPREGS, 2, 0, "EXPR");
}

static void M116PW(uint32 A, uint8 V)
{
 // setprg8(A,(V&0xF)|((EXPREGS[1]&0x2)<<3));
 // setprg8(A,(V&7)|(<<3));
 setprg8(A,V);
 if(!(EXPREGS[1]&2))
 {
  setprg16(0x8000,1); //EXPREGS[2]);
 }
}

static void M116CW(uint32 A, uint8 V)
{
 //if(EXPREGS[1]&2)
  setchr1(A,V|((EXPREGS[0]&0x4)<<6));
 //else
 //{
 // setchr1(A,(V&7)|((EXPREGS[2])<<3));
 //}
}

static DECLFW(M116Write)
{
 if(A==0x4100) {EXPREGS[0]=V;setmirror(V&1);}
 else if(A==0x4141) EXPREGS[1]=V;
 else if(A==0xa000) EXPREGS[2]=V;
 else if(A==0x4106) EXPREGS[3]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
// MIRROR_SET(0);
 //printf("Wr: $%04x:$%02x, $%04x, %d, %d\n",A,V,X.PC,timestamp,scanline);
}
static DECLFW(M116Write2)
{
 //printf("Wr2: %04x:%02x\n",A,V);
}
static void M116Power(void)
{
 
 EXPREGS[0]=0;
 EXPREGS[1]=2;
 EXPREGS[2]=0;
 EXPREGS[3]=0;

 GenMMC3Power();

 SetWriteHandler(0x4020,0x7fff,M116Write);
 SetWriteHandler(0xa000,0xbfff,M116Write2);
}

void Mapper116_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 0, 0);
 cwrap=M116CW;
 pwrap=M116PW;
 info->Power=M116Power;
 AddExState(EXPREGS, 4, 0, "EXPR");
}

static void M165CW(uint32 A, uint8 V)
{
 //printf("%04x:%02x\n",A,V);
 if(V < 0x4)
  setchr1r(0x10, A, V);
 else
  setchr1(A,V);
}

void Mapper165_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 128, 8, info->battery);
 cwrap=M165CW;
 CHRRAM = (uint8*)FCEU_gmalloc(8192);
 CHRRAMSize = 8192;
 AddExState(CHRRAM, 8192, 0, "chrr");
 SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
}

static DECLFW(Mapper250_write)
{
	Mapper4_write((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

static DECLFW(M250_IRQWrite)
{
	MMC3_IRQWrite((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

static void M250_Power(void)
{
	GenMMC3Power();
	SetWriteHandler(0x8000,0xBFFF,Mapper250_write);
	SetWriteHandler(0xC000,0xFFFF,M250_IRQWrite);
}

void Mapper250_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 info->Power=M250_Power;
}

static void M249PW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&0x2)
 {
  if(V<0x20)
   V=(V&1)|((V>>3)&2)|((V>>1)&4)|((V<<2)&8)|((V<<2)&0x10);
  else
  {
   V-=0x20;
   V=(V&3)|((V>>1)&4)|((V>>4)&8)|((V>>2)&0x10)|((V<<3)&0x20)|((V<<2)&0xC0);
  }
 }
 setprg8(A,V);
}

static void M249CW(uint32 A, uint8 V)
{
 if(EXPREGS[0]&0x2)
  V=(V&3)|((V>>1)&4)|((V>>4)&8)|((V>>2)&0x10)|((V<<3)&0x20)|((V<<2)&0xC0);
 setchr1(A,V);
}

static DECLFW(Mapper249_write)
{
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M249Power(void)
{
 EXPREGS[0]=0;
 GenMMC3Power();
 SetWriteHandler(0x5000,0x5000,Mapper249_write);
}

void Mapper249_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=M249CW;
 pwrap=M249PW;
 info->Power=M249Power;
 AddExState(EXPREGS, 1, 0, "EXPR");
}

static void M245CW(uint32 A, uint8 V)
{
 //printf("$%04x:$%02x\n",A,V);
 //setchr1(A,V);
 //	
 EXPREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
}

static void M245PW(uint32 A, uint8 V)
{
 setprg8(A,(V&0x3F)|((EXPREGS[0]&2)<<5));
 //printf("%d\n",(V&0x3F)|((EXPREGS[0]&2)<<5));
}

static void M245Power(void)
{
 EXPREGS[0]=0;
 GenMMC3Power();
}

void Mapper245_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 info->Power=M245Power;
 cwrap=M245CW;
 pwrap=M245PW;
 AddExState(EXPREGS, 1, 0, "EXPR");
}

void m74p(uint32 a, uint8 v)
{
 setprg8(a,v&0x3f);
}

static void m74kie(uint32 a, uint8 v)
{
 if(v==((PRGsize[0]>>16)&0x08) || v==1+((PRGsize[0]>>16)&0x08))
  setchr1r(0x10,a,v);
 else
  setchr1r(0x0,a,v);
}

void Mapper74_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=m74kie;
 pwrap=m74p;
 CHRRAM=(uint8*)FCEU_gmalloc(2048);
 CHRRAMSize=2048;
 SetupCartCHRMapping(0x10, CHRRAM, 2048, 1);
 AddExState(CHRRAM, 2048, 0, "CHRR");
}

static void m148kie(uint32 a, uint8 v)
{
 if(v==128 || v==129)
  setchr1r(0x10,a,v);
 else
  setchr1r(0x0,a,v);
}

void Mapper148_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=m148kie;
 pwrap=m74p;
 CHRRAM=(uint8*)FCEU_gmalloc(2048);
 CHRRAMSize=2048;
 SetupCartCHRMapping(0x10, CHRRAM, 2048, 1);
 AddExState(CHRRAM, 2048, 0, "CHRR");
}

static void FP_FASTAPASS(1) TKSPPU(uint32 A)
{
 //static uint8 z;
 //if(A>=0x2000 || type<0) return;
 //if(type<0) return;
 A&=0x1FFF;
 //if(scanline>=140 && scanline<=200) {setmirror(MI_1);return;}
 //if(scanline>=140 && scanline<=200)
 // if(scanline>=190 && scanline<=200) {setmirror(MI_1);return;}
 // setmirror(MI_1); 
 //printf("$%04x\n",A);

 A>>=10;
 PPUCHRBus=A;
 setmirror(MI_0+TKSMIR[A]);
}

static void TKSWRAP(uint32 A, uint8 V)
{
 TKSMIR[A>>10]=V>>7;
 setchr1(A,V&0x7F);
 if(PPUCHRBus==(A>>10))
  setmirror(MI_0+(V>>7));
}


static void TQWRAP(uint32 A, uint8 V)
{
 setchr1r((V&0x40)>>2,A,V&0x3F);
}

static int wrams;

static DECLFW(MBWRAM)
{
  WRAM[A-0x6000]=V;
}

static DECLFR(MAWRAM)
{
 return(WRAM[A-0x6000]);
}

static DECLFW(MBWRAMMMC6)
{
 WRAM[A&0x3ff]=V;
}

static DECLFR(MAWRAMMMC6)
{
 return(WRAM[A&0x3ff]);
}

static void GenMMC3Power(void)
{
 SetWriteHandler(0x8000,0xBFFF,Mapper4_write);
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0xC000,0xFFFF,MMC3_IRQWrite);
 A001B=A000B=0;
 setmirror((A000B&1)^1);
 if(mmc3opts&1)
 {
  if(wrams==1024)
  {
   FCEU_CheatAddRAM(1,0x7000,WRAM);
   SetReadHandler(0x7000,0x7FFF,MAWRAMMMC6);
   SetWriteHandler(0x7000,0x7FFF,MBWRAMMMC6);
  }
  else
  {
   FCEU_CheatAddRAM(wrams/1024,0x6000,WRAM);
   SetReadHandler(0x6000,0x6000+wrams-1,MAWRAM);
   SetWriteHandler(0x6000,0x6000+wrams-1,MBWRAM);
  }
  if(!(mmc3opts&2))
   FCEU_dwmemset(WRAM,0,wrams);
 }
 MMC3RegReset();
 if(CHRRAM)
  FCEU_dwmemset(CHRRAM,0,CHRRAMSize);
}

static void GenMMC3Close(void)
{
 if(CHRRAM)
  FCEU_gfree(CHRRAM);
 if(WRAM)
  FCEU_gfree(WRAM);
 CHRRAM=WRAM=NULL;
}

void GenMMC3_Init(CartInfo *info, int prg, int chr, int wram, int battery)
{
 pwrap=GENPWRAP;
 cwrap=GENCWRAP;
 mwrap=GENMWRAP;

 wrams=wram*1024;

 PRGmask8[0]&=(prg>>13)-1;
 CHRmask1[0]&=(chr>>10)-1;
 CHRmask2[0]&=(chr>>11)-1;

 if(wram)
 {
  mmc3opts|=1;
  WRAM=(uint8*)FCEU_gmalloc(wram*1024);
  AddExState(WRAM, wram*1024, 0, "WRAM");
 }

 if(battery)
 {
  mmc3opts|=2;

  info->SaveGame[0]=WRAM;
  info->SaveGameLen[0]=wram*1024;
 }

 if(!chr)
 {
  CHRRAM=(uint8*)FCEU_gmalloc(8192);
  CHRRAMSize=8192;
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
  AddExState(CHRRAM, 8192, 0, "CHRR");
 }
 AddExState(MMC3_StateRegs, ~0, 0, 0);

 info->Power=GenMMC3Power;
 info->Reset=MMC3RegReset;
 info->Close=GenMMC3Close;

 //PPU_hook=MMC3_PPUIRQ;

 if(info->CRC32 == 0x5104833e)		// Kick Master
  GameHBIRQHook = MMC3_hb_KickMasterHack;
 else if(info->CRC32 == 0x5a6860f1 || info->CRC32 == 0xae280e20) // Shougi Meikan '92/'93
  GameHBIRQHook = MMC3_hb_KickMasterHack;
 else if(info->CRC32 == 0xfcd772eb)	// PAL Star Wars, similar problem as Kick Master.
  GameHBIRQHook = MMC3_hb_PALStarWarsHack;
 else 
  GameHBIRQHook=MMC3_hb;
 GameStateRestore=genmmc3restore;
}

static int hackm4=0;		/* For Karnov, maybe others.  BLAH.  Stupid iNES format.*/

static void Mapper4Power(void)
{
 GenMMC3Power();
 A000B=(hackm4^1)&1;
 setmirror(hackm4);
}

void Mapper4_Init(CartInfo *info)
{
 int ws=8;

 if((info->CRC32==0x93991433 || info->CRC32==0xaf65aa84))
 {
  FCEU_printf("Low-G-Man can not work normally in the iNES format.\nThis game has been recognized by its CRC32 value, and the appropriate changes will be made so it will run.\nIf you wish to hack this game, you should use the UNIF format for your hack.\n\n");
  ws=0;
 }
 GenMMC3_Init(info,512,256,ws,info->battery);
 info->Power=Mapper4Power;
 hackm4=info->mirror;
}

void Mapper118_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=TKSWRAP;   
 mwrap=GENNOMWRAP;
 PPU_hook=TKSPPU;
 AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void Mapper119_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 64, 0, 0);
 cwrap=TQWRAP;
 CHRRAM=(uint8*)FCEU_gmalloc(8192);
 CHRRAMSize=8192;
 SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
}

void TEROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 32, 32, 0, 0);
}

void TFROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 64, 0, 0);
}

void TGROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 0, 0, 0);
}

void TKROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
}

void TLROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 0, 0);
}

void TSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, 0);
}

void TLSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, 0);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
 PPU_hook=TKSPPU;
 AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void TKSROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 256, 8, info->battery);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
 PPU_hook=TKSPPU;
 AddExState(&PPUCHRBus, 1, 0, "PPUC");
}

void TQROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 64, 0, 0);
 cwrap=TQWRAP;
 CHRRAM=(uint8*)FCEU_gmalloc(8192);
 CHRRAMSize=8192;
 SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
}

/* MMC6 board */
void HKROM_Init(CartInfo *info)
{
 GenMMC3_Init(info, 512, 512, 1, info->battery);
}
