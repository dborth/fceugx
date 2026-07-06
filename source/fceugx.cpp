/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
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
#include <sys/iosupport.h>

#ifdef HW_RVL
#include <di/di.h>
#endif

#include "fceugx.h"
#include "system.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "fceusupport.h"
#include "menu.h"
#include "preferences.h"
#include "fileop.h"
#include "filebrowser.h"
#include "networkop.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "videofilters.h"
#include "pad.h"
#include "filelist.h"
#include "gui/gui.h"
#include "utils/wiidrc.h"
#include "utils/FreeTypeGX.h"
#ifdef HW_RVL
	#include "mem2.h"
#endif
#ifdef USE_VM
	#include "vmalloc.h"
#endif

#include "fceultra/types.h"

int fskipc = 0;
int fskip = 0;
static uint8 *gfx=0;
static int32 *sound=0;
static int32 ssize=0;
int ScreenshotRequested = 0;
int ConfigRequested = 0;
char appPath[1024] = { 0 };

int frameskip = 0;
int turbomode = 0;
unsigned char * nesrom = NULL;
int eoptions=0;

static bool autoboot = false;

/****************************************************************************
 * main
 * This is where it all happens!
 ***************************************************************************/

int main(int argc, char *argv[])
{
	DefaultSettings (); // Set defaults
	SystemInit();
	ResetVideo_Menu (); // change to menu video mode
	
	#ifdef HW_RVL
	// store path app was loaded from
	if(argc > 0 && argv[0] != NULL)
		CreateAppPath(argv[0]);

	InitMem2Manager();
	#endif

	savebuffer = (unsigned char *)memalign(32,SAVEBUFFERSIZE);
#ifdef USE_VM
	browserList = (BROWSERENTRY *)vm_malloc(sizeof(BROWSERENTRY)*MAX_BROWSER_SIZE);
	nesrom = (unsigned char *)vm_malloc(1024*1024*4);
#else
	browserList = (BROWSERENTRY *)memalign(32,sizeof(BROWSERENTRY)*MAX_BROWSER_SIZE);
	nesrom = (unsigned char *)memalign(32,1024*1024*4);
#endif

	InitGUIThreads();

	/*** Minimal Emulation Loop ***/
	if (!FCEUI_Initialize())
		ExitApp();

	FCEUI_SetGameGenie(1); // 0 - OFF, 1 - ON
	
	FDSBIOS=(uint8 *)malloc(8192);
	memset(FDSBIOS, 0, sizeof(*FDSBIOS)); // clear FDS BIOS memory

	FCEUI_SetSoundQuality(1); // 0 - low, 1 - high, 2 - high (alt.)
	int currentTiming = 0;

#ifdef HW_RVL
	if(argc > 2 && argv[1] != NULL && argv[2] != NULL) {
		LoadPrefs();
		if(strncmp(argv[1], "sd", 2) == 0)
		{
			GCSettings.SaveMethod = DEVICE_SD;
			GCSettings.LoadMethod = DEVICE_SD;
		}
		else if(strncmp(argv[1], "usb", 3) == 0)
		{
			GCSettings.SaveMethod = DEVICE_USB;
			GCSettings.LoadMethod = DEVICE_USB;
		}
		SavePrefs(SILENT);

		GCSettings.AutoloadGame = AutoloadGame(argv[1], argv[2]);
		autoboot = GCSettings.AutoloadGame;
	}
#endif

	while (1) // main loop
	{
		if(!autoboot) {
			// go back to checking if devices were inserted/removed
			// since we're entering the menu
			ResumeDeviceThread();

			SwitchAudioMode(1);

			if(!romLoaded)
				MainMenu(MENU_GAMESELECTION);
			else
				MainMenu(MENU_GAME);
		}

		if(currentTiming != GCSettings.timing)
		{
			GameInfo->vidsys=(EGIV)GetFCEUTiming();
			UpdateDendy();
			FCEU_ResetVidSys();
		}

		currentTiming = GCSettings.timing;
		SelectFilterMethod(GCSettings.FilterMethod); // Initialize / Re-evaluate active filter
		autoboot = false;
		ConfigRequested = 0;
		ScreenshotRequested = 0;
		SwitchAudioMode(0);

		// stop checking if devices were removed/inserted
		// since we're starting emulation again
		HaltDeviceThread();

		ResetVideo_Emu();
		SetControllers();
		setFrameTimer(); // set frametimer method before emulation
		SetPalette();
		FCEUI_DisableSpriteLimitation(GCSettings.spritelimit ^ 1);

		fskip=0;
		fskipc=0;
		frameskip=0;

		while(1) // emulation loop
		{
			fskip = 0;
			
			if(turbomode)
			{
				fskip = 1;
								
				if(fskipc >= 18)
				{
					fskipc = 0;
					fskip = 0;
				}
				else
				{
					fskipc++;
				}
			}
			else if(frameskip > 0)
			{
				fskip = 1;
				
				if(fskipc >= frameskip)
				{
					fskipc = 0;
					fskip = 0;
				}
				else
				{
					fskipc++;
				}
			}

			Check3D();

			FCEUI_Emulate(&gfx, &sound, &ssize, fskip);

			if (!shutter_3d_mode && !anaglyph_3d_mode)
				FCEUD_Update(gfx, sound, ssize);
			else if (eye_3d)
				FCEUD_UpdateRight(gfx, sound, ssize);
			else
				FCEUD_UpdateLeft(gfx, sound, ssize);

			SyncSpeed();

			if(ResetRequested)
			{
				PowerNES(); // reset game
				ResetRequested = 0;
			}
			if(ConfigRequested)
			{
				ConfigRequested = 0;
				ResetVideo_Menu();
				break;
			}
			#ifdef HW_RVL
			if(ShutdownRequested)
				ExitApp();
			#endif

		} // emulation loop
	} // main loop
}

void ExitApp()
{
	SavePrefs(SILENT);

	if (romLoaded && !ConfigRequested && GCSettings.AutoSave == AUTOSAVE_RAM)
		SaveRAMAuto(SILENT);

	SystemExit(GCSettings.ExitAction, autoboot);
}
