#include "mapinc.h"

static DECLFW(Mapper182_write)
{
  switch(A&0xf003)
  {
   case 0xe003:IRQCount=V;IRQa=1;X6502_IRQEnd(FCEU_IQEXT);break;
   case 0x8001:MIRROR_SET(V&1);break;
   case 0xA000:mapbyte1[0]=V;break;
   case 0xC000:
               switch(mapbyte1[0]&7)
               {
                case 0:VROM_BANK2(0x0000,V>>1);break;
                case 1:VROM_BANK1(0x1400,V);break;
                case 2:VROM_BANK2(0x0800,V>>1);break;
                case 3:VROM_BANK1(0x1c00,V);break;
                case 4:ROM_BANK8(0x8000,V);break;
                case 5:ROM_BANK8(0xA000,V);break;
                case 6:VROM_BANK1(0x1000,V);break;
                case 7:VROM_BANK1(0x1800,V);break;
               }
               break;


  }
}

void blop(void)
{
 if(IRQa)
  {
   if(IRQCount)
   {
    IRQCount--;
    if(!IRQCount)
    {
	IRQa=0;
	X6502_IRQBegin(FCEU_IQEXT);
    }
   }
  }
}
void Mapper182_init(void)
{
 SetWriteHandler(0x8000,0xFFFF,Mapper182_write);
 GameHBIRQHook=blop;
}

