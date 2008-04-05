#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../../types.h"
#include "common.h"

/* Some timing-related variables. */
static int fullscreen=0;
static int genie=0;
static int palyo=0;

static volatile int nofocus=0;
static volatile int userpause=0;

#define SO_FORCE8BIT  1
#define SO_SECONDARY  2
#define SO_GFOCUS     4
#define SO_D16VOL     8

#define GOO_DISABLESS   1       /* Disable screen saver when game is loaded. */
#define GOO_CONFIRMEXIT 2       /* Confirmation before exiting. */
#define GOO_POWERRESET  4       /* Confirm on power/reset. */

static int soundvolume=100;
static int soundquality=0;
static int soundo;

int screenscaler = 2;

uint8 *xbsave=NULL;
int eoptions=EO_BGRUN | EO_FORCEISCALE;

extern int RenderFrame( char *XBuf , int style);

extern int ConfigScreen();

extern void InitialiseSound();
extern void initDisplay();
extern void InitialisePads();
extern int GetJoy();
extern void GCMemROM();
extern void PlaySound( void *Buf, int samples );
long long basetime;

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

void (*PSOReload) () = (void (*)()) 0x80001800;

static void reset_cb() {
    PSOReload();
}

extern int WaitPromptChoice (char *msg, char *bmsg, char *amsg);
int choosenSDSlot = 0;

int main(int argc, char *argv[])
{
    initDisplay();
    SYS_SetResetCallback (reset_cb);
    InitialiseSound();
    SDCARD_Init ();

    /*** Minimal Emulation Loop ***/
    if ( !FCEUI_Initialize() ) {
        printf("Ooops - unable to initialize system\n");
        return 1;
    }

    palyo=0;
    FCEUI_SetVidSystem(palyo);
    genie&=1;
    FCEUI_SetGameGenie(genie);
    fullscreen&=1;
    soundo&=1;
    FCEUI_SetSoundVolume(soundvolume);
    FCEUI_SetSoundQuality(soundquality);  

    cleanSFMDATA();
    GCMemROM();

    choosenSDSlot = !WaitPromptChoice("Choose a SLOT to load Roms from SDCARD", "SLOT B", "SLOT A");
    ConfigScreen(); 

    while (1)
    {
        uint8 *gfx;
        int32 *sound;
        int32 ssize;

        FCEUI_Emulate(&gfx, &sound, &ssize, 0);
        xbsave = gfx;
        FCEUD_Update(gfx, sound, ssize);              
    }

    return 0;
}

/****************************************************************************
 * FCEU Support Functions to be written
 ****************************************************************************/
/*** File Control ***/
FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
    return(fopen(n,m));
}

/*** General Logging ***/
void FCEUD_PrintError(char *s)
{
}

void FCEUD_Message(char *text)
{
}

/*** VIDEO ***/
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{

    PlaySound(Buffer, Count);
    RenderFrame( XBuf, screenscaler );
    GetJoy(); /* Fix by Garglub. Thanks! */

}

/*** Netplay ***/
int FCEUD_SendData(void *data, uint32 len)
{
    return 1;
}

int FCEUD_RecvData(void *data, uint32 len)
{
    return 0;
}

void FCEUD_NetworkClose(void)
{
}

void FCEUD_NetplayText(uint8 *text)
{
}
