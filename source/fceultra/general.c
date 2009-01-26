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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.h"
#include "fceu.h"

#include "general.h"
#include "state.h"

#include "driver.h"

#include "md5.h"

static char BaseDirectory[2048];
static char FileBase[2048];
static char FileExt[2048];	/* Includes the . character, as in ".nes" */

static char FileBaseDirectory[2048];

void FCEUI_SetBaseDirectory(char *dir)
{
 strncpy(BaseDirectory,dir,2047);
 BaseDirectory[2047]=0;
}

static char *odirs[FCEUIOD__COUNT]={0,0,0,0,0,0};     // odirs, odors. ^_^

void FCEUI_SetDirOverride(int which, char *n)
{
 odirs[which]=n;

 if(FCEUGameInfo)  /* Rebuild cache of present states/movies. */
 {
  if(which==FCEUIOD_STATE)
   FCEUSS_CheckStates();
 }
}

char *FCEU_MakeFName(int type, int id1, char *cd1)
{
	return NULL;
}

void GetFileBase(const char *f)
{
        const char *tp1,*tp3;

 #if PSS_STYLE==4
     tp1=((char *)strrchr(f,':'));
 #elif PSS_STYLE==1
     tp1=((char *)strrchr(f,'/'));
 #else
     tp1=((char *)strrchr(f,'\\'));
  #if PSS_STYLE!=3
     tp3=((char *)strrchr(f,'/'));
     if(tp1<tp3) tp1=tp3;
  #endif
 #endif
     if(!tp1)
     {
      tp1=f;
      strcpy(FileBaseDirectory,".");
     }
     else
     {
      memcpy(FileBaseDirectory,f,tp1-f);
      FileBaseDirectory[tp1-f]=0;
      tp1++;
     }

     if(((tp3=strrchr(f,'.'))!=NULL) && (tp3>tp1))
     {
      memcpy(FileBase,tp1,tp3-tp1);
      FileBase[tp3-tp1]=0;
      strcpy(FileExt,tp3);
     }
     else
     {
      strcpy(FileBase,tp1);
      FileExt[0]=0;
     }
}

uint32 uppow2(uint32 n)
{
 int x;

 for(x=31;x>=0;x--)
  if(n&(1<<x))
  {
   if((1<<x)!=n)
    return(1<<(x+1));
   break;
  }
 return n;
}

