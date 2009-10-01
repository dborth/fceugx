/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceustate.cpp
 *
 * Memory Based Load/Save RAM Manager
 *
 * These are the battery-backed RAM (save data) routines, brought together
 * as GCxxxxx
 * The original file I/O is replaced with Memory Read/Writes to the
 * savebuffer below
 ****************************************************************************/

#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include <fat.h>
#include <string.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "menu.h"
#include "filebrowser.h"
#include "memcardop.h"
#include "fileop.h"

static u32 NGCFCEU_GameSave(CartInfo *LocalHWInfo, int operation)
{
	u32 offset = 0;

	if(LocalHWInfo->battery && LocalHWInfo->SaveGame[0])
	{
		int x;

		for(x=0;x<4;x++)
		{
			if(LocalHWInfo->SaveGame[x])
			{
				if(operation == 0) // save to file
					memcpy(savebuffer+offset, LocalHWInfo->SaveGame[x], LocalHWInfo->SaveGameLen[x]);
				else // load from file
					memcpy(LocalHWInfo->SaveGame[x], savebuffer+offset, LocalHWInfo->SaveGameLen[x]);
				offset += LocalHWInfo->SaveGameLen[x];
			}
		}
	}
	return offset;
}

bool SaveRAM (char * filepath, bool silent)
{
	bool retval = false;
	int datasize = 0;
	int offset = 0;
	int device;
			
	if(!FindDevice(filepath, &device))
		return 0;

	if(GameInfo->type == GIT_FDS)
	{
		if(!silent)
			InfoPrompt("RAM saving is not available for FDS games!");
		return false;
	}

	AllocSaveBuffer ();

	// save game save to savebuffer
	if(GameInfo->type == GIT_CART)
		datasize = NGCFCEU_GameSave(&iNESCart, 0);
	else if(GameInfo->type == GIT_VSUNI)
		datasize = NGCFCEU_GameSave(&UNIFCart, 0);

	if (datasize)
	{
		if(device == DEVICE_MC_SLOTA || device == DEVICE_MC_SLOTB)
		{
			// Set the comments
			char comments[2][32];
			memset(comments, 0, 64);
			sprintf (comments[0], "%s RAM", APPNAME);
			snprintf (comments[1], 32, romFilename);
			SetMCSaveComments(comments);
		}

		offset = SaveFile(filepath, datasize, silent);

		if (offset > 0)
		{
			if (!silent)
				InfoPrompt("Save successful");
			retval = true;
		}
	}
	else
	{
		if (!silent)
			InfoPrompt("No data to save!");
	}
	FreeSaveBuffer ();
	return retval;
}

bool
SaveRAMAuto (bool silent)
{
	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_RAM, romFilename, 0))
		return false;

	return SaveRAM(filepath, silent);
}

bool LoadRAM (char * filepath, bool silent)
{
	int offset = 0;
	bool retval = false;
	int device;
			
	if(!FindDevice(filepath, &device))
		return 0;

	if(GameInfo->type == GIT_FDS) // RAM saves don't exist for FDS games
		return false;

	AllocSaveBuffer ();

	offset = LoadFile(filepath, silent);

	if (offset > 0)
	{
		if(GameInfo->type == GIT_CART)
			NGCFCEU_GameSave(&iNESCart, 1);
		else if(GameInfo->type == GIT_VSUNI)
			NGCFCEU_GameSave(&UNIFCart, 1);

		ResetNES();
		retval = true;
	}
	else
	{
		// if we reached here, nothing was done!
		if(!silent)
			InfoPrompt ("Save file not found");
	}
	FreeSaveBuffer ();
	return retval;
}

bool
LoadRAMAuto (bool silent)
{
	char filepath[MAXPATHLEN];
	char filepath2[MAXPATHLEN];

	// look for Auto save file
	if(!MakeFilePath(filepath, FILE_RAM, romFilename, 0))
		return false;

	if (LoadRAM(filepath, silent))
		return true;

	// look for file with no number or Auto appended
	if(!MakeFilePath(filepath2, FILE_RAM, romFilename, -1))
		return false;

	if(LoadRAM(filepath2, silent))
	{
		// rename this file - append Auto
		rename(filepath2, filepath); // rename file (to avoid duplicates)
		return true;
	}
	return false;
}
