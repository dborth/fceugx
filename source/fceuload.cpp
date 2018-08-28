/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceload.c
 *
 * NES Memory Load Game
 *
 * This performs the functions of LoadGame and iNESLoad from a single module
 * Helper function for GameCube injected ROMS
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gctypes.h>

#include "fceultra/file.h"

#include "fceugx.h"
#include "gcaudio.h"
#include "fceusupport.h"
#include "pad.h"
#include "menu.h"
#include "fileop.h"
#include "filebrowser.h"
#include "cheatmgr.h"

bool romLoaded = false;

int GCMemROM(int size)
{
	bool biosError = false;

	ResetGameLoaded();

	CloseGame();
	GameInfo = new FCEUGI();
	memset(GameInfo, 0, sizeof(FCEUGI));

	GameInfo->filename = strdup(romFilename);
	GameInfo->archiveCount = 0;

	/*** Set some default values ***/
	GameInfo->name=0;
	GameInfo->type=GIT_CART;
	GameInfo->vidsys=(EGIV)GCSettings.timing;
	GameInfo->input[0]=GameInfo->input[1]=SI_UNSET;
	GameInfo->inputfc=SIFC_UNSET;
	GameInfo->cspecial=SIS_NONE;

	/*** Set internal sound information ***/
	SetSampleRate();
	FCEUI_SetSoundVolume(100); // 0-100
	FCEUI_SetLowPass(0);

	FCEUFILE * fceufp = new FCEUFILE();
	fceufp->size = size;
	fceufp->filename = romFilename;
	fceufp->mode = FCEUFILE::READ; // read only
	EMUFILE_MEMFILE *fceumem = new EMUFILE_MEMFILE(nesrom, size);
	fceufp->stream = fceumem;

	romLoaded = iNESLoad(romFilename, fceufp, 1);

	if(!romLoaded)
	{
		romLoaded = UNIFLoad(romFilename, fceufp);
	}

	if(!romLoaded)
	{
		romLoaded = NSFLoad(romFilename, fceufp);
	}

	if(!romLoaded)
	{
		// read FDS BIOS into FDSBIOS - should be 8192 bytes
		if (FDSBIOS[1] == 0)
		{
			size_t biosSize = 0;
			char filepath[1024];

			AllocSaveBuffer ();

			sprintf (filepath, "%s%s/disksys.rom", pathPrefix[GCSettings.LoadMethod], APPFOLDER);
			biosSize = LoadFile(filepath, SILENT);
			if(biosSize == 0 && strlen(appPath) > 0)
			{
				sprintf (filepath, "%s/disksys.rom", appPath);
				biosSize = LoadFile(filepath, SILENT);
			}

			if (biosSize == 8192)
			{
				memcpy(FDSBIOS, savebuffer, 8192);
			}
			else
			{
				biosError = true;

				if (biosSize > 0)
					ErrorPrompt("FDS BIOS file is invalid!");
				else
					ErrorPrompt("FDS BIOS file not found!");
			}
			FreeSaveBuffer ();
		}
		if (FDSBIOS[1] != 0)
		{
			romLoaded = FDSLoad(romFilename, fceufp);
		}
	}

	delete fceufp;

	if (romLoaded)
	{
		FCEU_ResetVidSys();

		if(GameInfo->type!=GIT_NSF)
			if(FSettings.GameGenie)
				OpenGameGenie();
		PowerNES();

		//if(GameInfo->type!=GIT_NSF)
		//	FCEU_LoadGamePalette();

		FCEU_ResetPalette();
		FCEU_ResetMessages();	// Save state, status messages, etc.
		SetupCheats();
		ResetAudio();
		return 1;
	}
	else
	{
		delete GameInfo;
		GameInfo = 0;

		if(!biosError)
			ErrorPrompt("Invalid game file!");
		romLoaded = false;
		return 0;
	}
}
