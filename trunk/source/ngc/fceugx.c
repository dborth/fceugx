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

#include "fceuconfig.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "common.h"
#include "menudraw.h"
#include "menu.h"
#include "preferences.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "pad.h"

#ifdef WII_DVD
#include <di/di.h>
#endif

unsigned char * nesrom = NULL;
int ConfigRequested = 0;
bool isWii;
uint8 *xbsave=NULL;

long long prev;
long long now;

long long gettime();
u32 diff_usec(long long start,long long end);

extern bool romLoaded;

extern int cleanSFMDATA();
extern void ResetNES(void);
extern uint8 FDSBIOS[8192];

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

/****************************************************************************
 * setFrameTimer()
 * change frame timings depending on whether ROM is NTSC or PAL
 ***************************************************************************/

int normaldiff;

void setFrameTimer()
{
	if (GCSettings.timing) // PAL
	{
		if(vmode_60hz == 1)
			normaldiff = 20000; // 50hz
		else
			normaldiff = 16667; // 60hz
	}
	else
	{
		if(vmode_60hz == 1)
			normaldiff = 16667; // 60hz
		else
			normaldiff = 20000; // 50hz
	}
	FrameTimer = 0;
	prev = gettime();
}

void SyncSpeed()
{
	now = gettime();
	int diff = normaldiff - diff_usec(prev, now);
	if (diff > 0) // ahead - take a nap
		usleep(diff);

	prev = now;
	FrameTimer--;
}

/****************************************************************************
 * main
 * This is where it all happens!
 ***************************************************************************/

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

	PAD_Init();

    initDisplay();

    /*** Initialise freetype ***/
	if (FT_Init ())
	{
		printf ("Cannot initialise font subsystem!\n");
		while (1);
	}

    InitialiseSound();
    fatInit (8, false);
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

    while (1) // main loop
    {
		MainMenu(selectedMenu);
		selectedMenu = 4; // return to game menu from now on

		setFrameTimer(); // set frametimer method before emulation
		FCEUI_SetVidSystem(GCSettings.timing);

		while(1) // emulation loop
		{
			uint8 *gfx;
			int32 *sound;
			int32 ssize;

			FCEUI_Emulate(&gfx, &sound, &ssize, 0);
			xbsave = gfx;
			FCEUD_Update(gfx, sound, ssize);

			if(ConfigRequested)
			{
				if (GCSettings.AutoSave == 1)
				{
					SaveRAM(GCSettings.SaveMethod, SILENT);
				}
				else if (GCSettings.AutoSave == 2)
				{
					SaveState(GCSettings.SaveMethod, SILENT);
				}
				else if(GCSettings.AutoSave == 3)
				{
					SaveRAM(GCSettings.SaveMethod, SILENT);
					SaveState(GCSettings.SaveMethod, SILENT);
				}
				ConfigRequested = 0;
				break; // leave emulation loop
			}

			SyncSpeed();
		}
    }

    return 0;
}

/****************************************************************************
 * FCEU Support Functions
 ****************************************************************************/
// File Control
FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
    return NULL;
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
