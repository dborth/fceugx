/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceugx.cpp
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
#include <malloc.h>

#include "fceugx.h"
#include "fceuconfig.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "common.h"
#include "menu.h"
#include "preferences.h"
#include "fileop.h"
#include "filebrowser.h"
#include "networkop.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "pad.h"
#include "filelist.h"
#include "gui/gui.h"

#ifdef HW_RVL
#include <di/di.h>
#endif

#include "FreeTypeGX.h"

extern "C" {
#include "types.h"
extern int cleanSFMDATA();
extern void PowerNES(void);
extern uint8 FDSBIOS[8192];

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);
}

static uint8 *xbsave=NULL;
static int fskipc = 0;
static int videoReset = 0;
static int currentMode = 0;
static int ResetRequested = 0;
int ConfigRequested = 0;
int ShutdownRequested = 0;
int ExitRequested = 0;
char appPath[1024];
FreeTypeGX *fontSystem;
int frameskip = 0;
unsigned char * nesrom = NULL;

/****************************************************************************
 * Shutdown / Reboot / Exit
 ***************************************************************************/

static void ExitCleanup()
{
#ifdef HW_RVL
	ShutoffRumble();
#endif
	ShutdownAudio();
	StopGX();

	HaltDeviceThread();
	UnmountAllFAT();

#ifdef HW_RVL
	DI_Close();
#endif
}

#ifdef HW_DOL
	#define PSOSDLOADID 0x7c6000a6
	int *psoid = (int *) 0x80001800;
	void (*PSOReload) () = (void (*)()) 0x80001800;
#endif

void ExitApp()
{
	ExitCleanup();

	#ifdef HW_RVL
	if(GCSettings.ExitAction == 0) // Auto
	{
		char * sig = (char *)0x80001804;
		if(
			sig[0] == 'S' &&
			sig[1] == 'T' &&
			sig[2] == 'U' &&
			sig[3] == 'B' &&
			sig[4] == 'H' &&
			sig[5] == 'A' &&
			sig[6] == 'X' &&
			sig[7] == 'X')
			GCSettings.ExitAction = 3; // Exit to HBC
		else
			GCSettings.ExitAction = 1; // HBC not found
	}
	#endif

	if(GCSettings.ExitAction == 1) // Exit to Menu
	{
		#ifdef HW_RVL
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		#else
			#define SOFTRESET_ADR ((volatile u32*)0xCC003024)
			*SOFTRESET_ADR = 0x00000000;
		#endif
	}
	else if(GCSettings.ExitAction == 2) // Shutdown Wii
	{
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	}
	else // Exit to Loader
	{
		#ifdef HW_RVL
			exit(0);
		#else
			if (psoid[0] == PSOSDLOADID)
				PSOReload();
		#endif
	}
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
	ExitCleanup();
	SYS_ResetSystem(SYS_POWEROFF, 0, 0);
}
#endif

#ifdef HW_DOL
/****************************************************************************
 * ipl_set_config
 * lowlevel Qoob Modchip disable
 ***************************************************************************/

static void ipl_set_config(unsigned char c)
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

static void CreateAppPath(char origpath[])
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

	#ifdef HW_RVL
	DI_Init();	// first
	#endif

	InitDeviceThread();
	VIDEO_Init();
	PAD_Init();

	#ifdef HW_RVL
	WPAD_Init();
	#endif

	InitGCVideo (); // Initialise video
	ResetVideo_Menu (); // change to menu video mode

	#ifdef HW_RVL
	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);

	// Wii Power/Reset buttons
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);
	#endif

	// Initialize DVD subsystem (GameCube only)
	#ifdef HW_DOL
	DVD_Init ();
	#endif

	// store path app was loaded from
	sprintf(appPath, "fceugx");
	if(argc > 0 && argv[0] != NULL)
		CreateAppPath(argv[0]);

	MountAllFAT(); // Initialize libFAT for SD and USB

	DefaultSettings(); // Set defaults

	// Audio
	InitialiseAudio();

	// Initialize font system
	fontSystem = new FreeTypeGX();
	fontSystem->loadFont(font_ttf, font_ttf_size, 0);
	fontSystem->setCompatibilityMode(FTGX_COMPATIBILITY_DEFAULT_TEVOP_GX_PASSCLR | FTGX_COMPATIBILITY_DEFAULT_VTXDESC_GX_NONE);

	InitGUIThreads();

	// allocate memory to store rom
	nesrom = (unsigned char *)memalign(32,1024*1024*3); // 3 MB should be plenty

	/*** Minimal Emulation Loop ***/
	if (!FCEUI_Initialize())
		ExitApp();

	FCEUI_SetGameGenie(0); // 0 - OFF, 1 - ON

	memset(FDSBIOS, 0, sizeof(FDSBIOS)); // clear FDS BIOS memory
	cleanSFMDATA(); // clear state data

	FCEUI_SetSoundQuality(1); // 0 - low, 1 - high, 2 - high (alt.)
	int currentTiming = 0;

    while (1) // main loop
    {
    	// go back to checking if devices were inserted/removed
		// since we're entering the menu
    	ResumeDeviceThread();

		ConfigRequested = 1;
		SwitchAudioMode(1);

		if(!romLoaded)
			MainMenu(MENU_GAMESELECTION);
		else
			MainMenu(MENU_GAME);

		if(currentTiming != GCSettings.timing)
			FCEUI_SetVidSystem(GCSettings.timing); // causes a small 'pop' in the audio

		videoReset = -1;
		currentMode = GCSettings.render;
		ConfigRequested = 0;
		SwitchAudioMode(0);

		// stop checking if devices were removed/inserted
		// since we're starting emulation again
		HaltDeviceThread();

		ResetVideo_Emu();
		SetControllers();
		setFrameTimer(); // set frametimer method before emulation
		SetPalette();
		FCEUI_DisableSpriteLimitation(GCSettings.spritelimit ^ 1);

		fskipc=0;

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
				if((GCSettings.render != 0 && videoReset == -1) || videoReset == 0)
				{
					TakeScreenshot();
					ResetVideo_Menu();
					ConfigRequested = 0;
					GCSettings.render = currentMode;
					break; // leave emulation loop
				}
				else if(videoReset == -1)
				{
					GCSettings.render = 2;
					videoReset = 2;
					ResetVideo_Emu();
				}
				videoReset--;
			}
		} // emulation loop
    } // main loop
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
