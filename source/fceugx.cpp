/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceugx.cpp
 *
 * This file controls overall program flow. Most things start and end here!
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "fceugx.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "fceusupport.h"
#include "menu.h"
#include "preferences.h"
#include "fileop.h"
#include "filebrowser.h"
#include "networkop.h"
#include "gcvideo.h"
#include "pad.h"
#include "filelist.h"
#include "font_ttf.h"
#include "fceultra/types.h"
#include "libgui/Gui.h"

#include "drivers/Platform.h"
#include "drivers/ogc/OgcEmulatorAudio.h"
#include "drivers/ogc/videofilters.h"

#ifdef HW_RVL
	#include "mem2.h"
#endif
#ifdef HW_DOL
	#include "drivers/ogc/vm/vmalloc.h"
#endif

#ifdef HW_DOL
#include "drivers/ogc/GameCubePlatform.h"
static GameCubePlatform platformInstance;
#else
#include "drivers/ogc/WiiPlatform.h"
static WiiPlatform platformInstance;
#endif
Platform* platform = &platformInstance;

AppRequest appRequest = AppRequest::NONE;

int fskipc = 0;
int fskip = 0;
static uint8 *gfx=0;
static int32 *sound=0;
static int32 ssize=0;
char appPath[1024] = { 0 };

int frameskip = 0;
int turbomode = 0;
unsigned char * nesrom = nullptr;
int eoptions=0;

static bool autoboot = false;

/****************************************************************************
 * main
 * This is where it all happens!
 ***************************************************************************/

int main(int argc, char *argv[])
{
	platform->init(640, 480);
	InitFileOpThreads();
	MountAllFAT();

	void * decodeScratch = malloc(IMAGE_DECODE_SCRATCH_SIZE);
	GuiImageData::setDecodeScratch(decodeScratch, IMAGE_DECODE_SCRATCH_SIZE);

	fontSystem = new GuiTextRenderer(font_ttf, font_ttf_size, platform->getVideo()->getGlyphRenderer());
	textTranslator = new GuiTextTranslator();
	textTranslator->loadLanguage(en_lang, en_lang_size);

	DefaultSettings();
	ApplySettings();
	platform->getVideo()->startMenuVideo();
	
	#ifdef HW_RVL
	// store path app was loaded from
	if(argc > 0 && argv[0] != nullptr)
		CreateAppPath(argv[0]);

	InitMem2Manager();
	#endif

	savebuffer = (unsigned char *)memalign(32,SAVEBUFFERSIZE);
#ifdef HW_DOL
	browserList = (BROWSERENTRY *)vm_malloc(sizeof(BROWSERENTRY)*MAX_BROWSER_SIZE);
	nesrom = (unsigned char *)vm_malloc(1024*1024*4);
#else
	browserList = (BROWSERENTRY *)memalign(32,sizeof(BROWSERENTRY)*MAX_BROWSER_SIZE);
	nesrom = (unsigned char *)memalign(32,1024*1024*4);
#endif

	InitGUIThreads();

	if (!FCEUI_Initialize())
		ExitApp();

	FCEUI_SetGameGenie(1); // 0 - OFF, 1 - ON
	
	FDSBIOS=(uint8 *)malloc(8192);
	memset(FDSBIOS, 0, sizeof(*FDSBIOS)); // clear FDS BIOS memory

	FCEUI_SetSoundQuality(1); // 0 - low, 1 - high, 2 - high (alt.)
	int currentTiming = 0;

#ifdef HW_RVL
	if(argc > 2 && argv[1] != nullptr && argv[2] != nullptr) {
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
		SavePrefs();

		GCSettings.AutoloadGame = AutoloadGame(argv[1], argv[2]);
		autoboot = GCSettings.AutoloadGame;
	}
#endif

	while (appRequest != AppRequest::EXIT && platform->getSystemEvent() != SystemEvent::ShutdownRequested) // main loop
	{
		if(!autoboot) {
			// go back to checking if devices were inserted/removed
			// since we're entering the menu
			ResumeDeviceCheckingThread();

			platform->getAudio()->startMenuAudio();

			if(!romLoaded)
				MainMenu(MENU_GAMESELECTION);
			else
				MainMenu(MENU_GAME);
		}

		if(appRequest == AppRequest::EXIT || platform->getSystemEvent() == SystemEvent::ShutdownRequested) {
			break;
		}

		if(currentTiming != GCSettings.timing)
		{
			GameInfo->vidsys=(EGIV)GetFCEUTiming();
			UpdateDendy();
			FCEU_ResetVidSys();
		}

		currentTiming = GCSettings.timing;
		SelectFilterMethod(GCSettings.videoUpscalingFilter); // Initialize / Re-evaluate active filter
		autoboot = false;
		appRequest = AppRequest::NONE;
		platform->getAudio()->startEmulatorAudio();

		// stop checking if devices were removed/inserted
		// since we're starting emulation again
		HaltDeviceCheckingThread();

		platform->getVideo()->getEmulatorVideo()->resetVideo();
		SetControllers();
		setFrameTimer(); // set frametimer method before emulation
		SetPalette();
		FCEUI_DisableSpriteLimitation(GCSettings.spritelimit ^ 1);

		fskip=0;
		fskipc=0;
		frameskip=0;

		while(appRequest == AppRequest::NONE) // emulation loop
		{
			SystemEvent event = platform->getSystemEvent(); // poll exactly once per iteration
			if(event == SystemEvent::ShutdownRequested)
				break;

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

			if(event == SystemEvent::ResetRequested)
			{
				PowerNES(); // reset game
			}
			if (appRequest == AppRequest::MENU)
			{
				appRequest = AppRequest::NONE;
				TakeScreenshot();
				platform->getVideo()->startMenuVideo();
				break;
			}
		} // emulation loop
	} // main loop
	ExitApp();
}

void ExitApp()
{
	SavePrefs();

	if (romLoaded && appRequest != AppRequest::MENU && GCSettings.AutoSave == AUTOSAVE_RAM)
		SaveRAMAuto(SILENT);

	HaltDeviceCheckingThread();
	platform->requestExit(GCSettings.ExitAction, autoboot);
}
