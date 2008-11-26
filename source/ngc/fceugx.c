/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceugx.c
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

#include "fceugx.h"
#include "fceuconfig.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "common.h"
#include "menudraw.h"
#include "menu.h"
#include "preferences.h"
#include "fileop.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "pad.h"

#ifdef WII_DVD
#include <di/di.h>
#endif

unsigned char * nesrom = NULL;
int ConfigRequested = 0;
int ShutdownRequested = 0;
int ResetRequested = 0;
char appPath[1024];
bool isWii;
uint8 *xbsave=NULL;
int frameskip = 0;

extern bool romLoaded;

extern int cleanSFMDATA();
extern void PowerNES(void);
extern uint8 FDSBIOS[8192];

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

/****************************************************************************
 * Shutdown / Reboot / Exit
 ***************************************************************************/

#ifdef HW_DOL
	#define PSOSDLOADID 0x7c6000a6
	int *psoid = (int *) 0x80001800;
	void (*PSOReload) () = (void (*)()) 0x80001800;
#endif

void Reboot()
{
	UnmountAllFAT();
#ifdef HW_RVL
	DI_Close();
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#else
	#define SOFTRESET_ADR ((volatile u32*)0xCC003024)
	*SOFTRESET_ADR = 0x00000000;
#endif
}

void ExitToLoader()
{
	UnmountAllFAT();
	// Exit to Loader
	#ifdef HW_RVL
		DI_Close();
		exit(0);
	#else	// gamecube
		if (psoid[0] == PSOSDLOADID)
			PSOReload ();
	#endif
}

#ifdef HW_RVL
void ShutdownCB()
{
	ConfigRequested = 1;
	ShutdownRequested = 1;
}
void ResetCB()
{
	ResetRequested = 1;
}
void ShutdownWii()
{
	UnmountAllFAT();
	DI_Close();
	SYS_ResetSystem(SYS_POWEROFF, 0, 0);
}
#endif

#ifdef HW_DOL
/****************************************************************************
 * ipl_set_config
 * lowlevel Qoob Modchip disable
 ***************************************************************************/

void ipl_set_config(unsigned char c)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	unsigned long val,addr;
	addr=0xc0000000;
	val = c << 24;
	exi[0] = ((((exi[0]) & 0x405) | 256) | 48);	//select IPL
	//write addr of IPL
	exi[0 * 5 + 4] = addr;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);
	//write the ipl we want to send
	exi[0 * 5 + 4] = val;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);

	exi[0] &= 0x405;	//deselect IPL
}
#endif

void CreateAppPath(char origpath[])
{
#ifdef HW_DOL
	sprintf(appPath, GCSettings.SaveFolder);
#else
	char path[1024];
	strcpy(path, origpath); // make a copy so we don't mess up original

	char * loc;
	int pos = -1;

	loc = strrchr(path,'/');
	if (loc != NULL)
		*loc = 0; // strip file name

	loc = strchr(path,'/'); // looking for / from fat:/
	if (loc != NULL)
		pos = loc - path + 1;

	if(pos >= 0 && pos < 1024)
		sprintf(appPath, &(path[pos]));
#endif
}

/****************************************************************************
 * main
 * This is where it all happens!
 ***************************************************************************/

int main(int argc, char *argv[])
{
	#ifdef HW_DOL
	ipl_set_config(6); // disable Qoob modchip
	#endif

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

	// Wii Power/Reset buttons
	#ifdef HW_RVL
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);
	#endif

    InitGCVideo ();
    ResetVideo_Menu (); // change to menu video mode

    /*** Initialise freetype ***/
	if (FT_Init ())
	{
		printf ("Cannot initialise font subsystem!\n");
		while (1);
	}

    InitialiseAudio();
    fatInit (8, false);

    // Initialize DVD subsystem (GameCube only)
	#ifdef HW_DOL
	DVD_Init ();
	#endif

    // allocate memory to store rom
    nesrom = (unsigned char *)malloc(1024*1024*3); // 3 MB should be plenty

    /*** Minimal Emulation Loop ***/
    if ( !FCEUI_Initialize() )
    {
		WaitPrompt((char *)"Unable to initialize FCE Ultra\n");
		ExitToLoader();
    }

	FCEUI_SetGameGenie(0); // 0 - OFF, 1 - ON

    memset(FDSBIOS, 0, sizeof(FDSBIOS)); // clear FDS BIOS memory
    cleanSFMDATA(); // clear state data

    // Set defaults
	DefaultSettings();

	// store path app was loaded from
	sprintf(appPath, "fceugx");
	if(argc > 0 && argv[0] != NULL)
		CreateAppPath(argv[0]);

	// Load preferences
	if(!LoadPrefs())
	{
		WaitPrompt((char*) "Preferences reset - check settings!");
		selectedMenu = 1; // change to preferences menu
	}

	FCEUI_SetSoundQuality(1); // 0 - low, 1 - high, 2 - high (alt.)
	FCEUI_SetVidSystem(GCSettings.timing); // causes a small 'pop' in the audio

    while (1) // main loop
    {
		#ifdef HW_RVL
		if(ShutdownRequested)
			ShutdownWii();
		#endif

    	MainMenu(selectedMenu);
		selectedMenu = 2; // return to game menu from now on

		ResetVideo_Emu();

		setFrameTimer(); // set frametimer method before emulation
		SetPalette();

		static int fskipc=0;

		while(1) // emulation loop
		{
			uint8 *gfx;
			int32 *sound;
			int32 ssize;

			#ifdef FRAMESKIP
			fskipc=(fskipc+1)%(frameskip+1);
			#endif

			FCEUI_Emulate(&gfx, &sound, &ssize, fskipc);

			if(!fskipc)
			{
				xbsave = gfx;
				FCEUD_Update(gfx, sound, ssize);
			}

			if(ResetRequested)
			{
				PowerNES(); // reset game
				ResetRequested = 0;
			}

			if(ConfigRequested)
			{
				ResetVideo_Menu();
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

				// save zoom level
				SavePrefs(GCSettings.SaveMethod, SILENT);

				ConfigRequested = 0;
				break; // leave emulation loop
			}
		}
    }
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
    RenderFrame(XBuf); // output video frame
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
