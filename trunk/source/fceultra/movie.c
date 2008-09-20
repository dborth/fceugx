#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "input.h"
#include "fceu.h"
#include "driver.h"
#include "state.h"
#include "general.h"
#include "video.h"

static int current = 0;		// > 0 for recording, < 0 for playback
static FILE *slots[10]={0};
static uint8 joop[4];
static uint32 framets;

/* Cache variables used for playback. */
static uint32 nextts;
static int nextd;


static int CurrentMovie = 0;
static int MovieShow = 0;  

static int MovieStatus[10];

static void DoEncode(int joy, int button, int);

int FCEUMOV_IsPlaying(void)
{
 if(current < 0) return(1);
 else return(0);
}

void FCEUI_SaveMovie(char *fname)
{
 FILE *fp;
 char *fn;

 if(current < 0)	/* Can't interrupt playback.*/
  return;

 if(current > 0)	/* Stop saving. */
 {
  DoEncode(0,0,1);	/* Write a dummy timestamp value so that the movie will keep
			   "playing" after user input has stopped.
			*/
  fclose(slots[current-1]);
  MovieStatus[current - 1] = 1;
  current=0;
  FCEU_DispMessage("Movie recording stopped.");
  return;  
 }

 current=CurrentMovie;

 if(fname)
  fp = FCEUD_UTF8fopen(fname, "wb");
 else
 {
  fp=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_MOVIE,CurrentMovie,0),"wb");
  free(fn);
 }

 if(!fp) return;

 FCEUSS_SaveFP(fp);
 fseek(fp, 0, SEEK_END);
 slots[current] = fp;
 memset(joop,0,sizeof(joop));
 current++;
 framets=0;
 FCEUI_SelectMovie(CurrentMovie);       /* Quick hack to display status. */
}

static void StopPlayback(void)
{
  fclose(slots[-1 - current]);
  current=0;
  FCEU_DispMessage("Movie playback stopped.");
}

void FCEUMOV_Stop(void)
{
 if(current < 0) StopPlayback();
}

void FCEUI_LoadMovie(char *fname)
{
 FILE *fp;
 char *fn;

 if(current > 0)        /* Can't interrupt recording.*/
  return;

 if(current < 0)        /* Stop playback. */
 {
  StopPlayback();
  return;
 }

 if(fname)
  fp = FCEUD_UTF8fopen(fname, "rb");
 else
 {
  fp=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_MOVIE,CurrentMovie,0),"rb");
  free(fn);
 }
 if(!fp) return;

 if(!FCEUSS_LoadFP(fp)) return;

 current = CurrentMovie;
 slots[current] = fp;

 memset(joop,0,sizeof(joop));
 current = -1 - current;
 framets=0;
 nextts=0;
 nextd = -1;
 MovieStatus[CurrentMovie] = 1;
 FCEUI_SelectMovie(CurrentMovie);       /* Quick hack to display status. */
}


static void DoEncode(int joy, int button, int dummy)
{
 uint8 d;

 d = 0;

 if(framets >= 65536)
  d = 3 << 5;
 else if(framets >= 256)
  d = 2 << 5;
 else if(framets > 0)
  d = 1 << 5;

 if(dummy) d|=0x80;

 d |= joy << 3;
 d |= button;

 fputc(d, slots[current - 1]);
 //printf("Wr: %02x, %d\n",d,slots[current-1]);
 while(framets)
 {
  fputc(framets & 0xff, slots[current - 1]);
  //printf("Wrts: %02x\n",framets & 0xff);
  framets >>= 8;
 }
}

void FCEUMOV_AddJoy(uint8 *js)
{
 int x,y;

 if(!current) return;	/* Not playback nor recording. */

 if(current < 0)	/* Playback */
 {
  while(nextts == framets)
  {
   int tmp,ti;
   uint8 d;

   if(nextd != -1)
   {
    if(nextd&0x80)
    {
	//puts("Egads");
     FCEU_DoSimpleCommand(nextd&0x1F);
    }
    else
     joop[(nextd >> 3)&0x3] ^= 1 << (nextd&0x7);
   }


   tmp = fgetc(slots[-1 - current]);
   d = tmp;

   if(tmp < 0)
   {
    StopPlayback();
    return;
   }

   nextts = 0;
   tmp >>= 5;
   tmp &= 0x3;
   ti=0;

   int tmpfix = tmp;
   while(tmp--) { nextts |= fgetc(slots[-1 - current]) << (ti * 8); ti++; }

   // This fixes a bug in movies recorded before version 0.98.11
   // It's probably not necessary, but it may keep away CRAZY PEOPLE who recorded
   // movies on <= 0.98.10 and don't work on playback.
   if(tmpfix == 1 && !nextts) 
   {nextts |= fgetc(slots[-1 - current])<<8; }
   else if(tmpfix == 2 && !nextts) {nextts |= fgetc(slots[-1 - current])<<16; }

   framets = 0;
   nextd = d;
  }
  memcpy(js,joop,4);
 }
 else			/* Recording */
 {
  for(x=0;x<4;x++)
  {
   if(js[x] != joop[x])
   {
    for(y=0;y<8;y++)
     if((js[x] ^ joop[x]) & (1 << y))
      DoEncode(x, y, 0);
    joop[x] = js[x];
   }
   else if(framets == ((1<<24)-1)) DoEncode(0,0,1);	/* Overflow will happen, so do dummy update. */
  }
 }
 framets++;
}

void FCEUMOV_AddCommand(int cmd)
{
 if(current <= 0) return;	/* Return if not recording a movie */
 //printf("%d\n",cmd);
 DoEncode((cmd>>3)&0x3,cmd&0x7,1);
}

void FCEUMOV_CheckMovies(void)
{
        FILE *st=NULL;
        char *fn;
        int ssel;
        
        for(ssel=0;ssel<10;ssel++)
        {
         st=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_MOVIE,ssel,0),"rb");
         free(fn);
         if(st)
         {
          MovieStatus[ssel]=1;
          fclose(st);
         }   
         else
          MovieStatus[ssel]=0;
        }

}

void FCEUI_SelectMovie(int w)
{
 if(w == -1) { MovieShow = 0; return; }
 FCEUI_SelectState(-1);

 CurrentMovie=w;
 MovieShow=180;

 if(current > 0)
  FCEU_DispMessage("-recording movie %d-",current-1);
 else if (current < 0)
  FCEU_DispMessage("-playing movie %d-",-1 - current);
 else
  FCEU_DispMessage("-select movie-");
}

void FCEU_DrawMovies(uint8 *XBuf)
{
 if(!MovieShow) return;
 
 FCEU_DrawNumberRow(XBuf,MovieStatus, CurrentMovie);
 MovieShow--;
}

