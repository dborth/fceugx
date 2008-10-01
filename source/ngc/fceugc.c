/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceugc.c
 *
 * This file controls overall program flow. Most things start and end here!
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gctypes.h>
#include <ogc/system.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#include "types.h"

#include "gcvideo.h"
#include "pad.h"
#include "fceuload.h"
#include "common.h"
#include "menudraw.h"
#include "menu.h"
#include "fceuconfig.h"
#include "preferences.h"
#include "gcaudio.h"

#ifdef WII_DVD
#include <di/di.h>
#endif

unsigned char * nesrom = NULL;
extern bool romLoaded;
bool isWii;

uint8 *xbsave=NULL;

extern int cleanSFMDATA();
extern void ResetNES(void);
extern uint8 FDSBIOS[8192];

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

int main(int argc, char *argv[])
{

#ifdef WII_DVD
	DI_Init();	// first
#endif

	int selectedMenu = -1;

#ifdef HW_RVL
	WPAD_Init();
	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL,640,480);
#endif

    initDisplay();

    /*** Initialise freetype ***/
	if (FT_Init ())
	{
		printf ("Cannot initialise font subsystem!\n");
		while (1);
	}

    InitialiseSound();
    fatInitDefault();
#ifndef HW_RVL
    DVD_Init();
#endif

    // allocate memory to store rom
    nesrom = (unsigned char *)malloc(1024*1024*3); // 3 MB should be plenty

    /*** Minimal Emulation Loop ***/
    if ( !FCEUI_Initialize() ) {
        printf("Unable to initialize system\n");
        return 1;
    }

    FCEUI_SetVidSystem(0); // 0 - NTSC, 1 - PAL
    FCEUI_SetGameGenie(0); // 0 - OFF, 1 - ON

    memset(FDSBIOS, 0, sizeof(FDSBIOS)); // clear FDS BIOS memory
    cleanSFMDATA(); // clear state data

    // Set Defaults
	DefaultSettings();

	// Load preferences

	if(!LoadPrefs())
	{
		WaitPrompt((char*) "Preferences reset - check settings!");
		selectedMenu = 3; // change to preferences menu
	}

	// Go to main menu
    MainMenu (selectedMenu);

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
// File Control
FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
    return(fopen(n,m));
}

// General Logging
void FCEUD_PrintError(char *s)
{
}

void FCEUD_Message(char *text)
{
}

// main interface to FCE Ultra
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int32 Count)
{
    PlaySound(Buffer, Count); // play sound
    RenderFrame( (char *)XBuf, GCSettings.screenscaler); // output video frame
    GetJoy(); // check controller input
}

// Netplay
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
