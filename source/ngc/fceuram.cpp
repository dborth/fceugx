/****************************************************************************
 * FCE Ultra 0.98.12
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

static u32 NGCFCEU_GameSave(CartInfo *LocalHWInfo, int operation, int method)
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

bool SaveRAM (char * filepath, int method, bool silent)
{
	bool retval = false;
	int datasize = 0;
	int offset = 0;

	if(nesGameType == 4)
	{
		if(!silent)
			InfoPrompt("RAM saving is not available for FDS games!");
		return false;
	}

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	AllocSaveBuffer ();

	// save game save to savebuffer
	if(nesGameType == 1)
		datasize = NGCFCEU_GameSave(&iNESCart, 0, method);
	else if(nesGameType == 2)
		datasize = NGCFCEU_GameSave(&UNIFCart, 0, method);

	if (datasize)
	{
		if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
		{
			// Set the comments
			char comments[2][32];
			memset(comments, 0, 64);
			sprintf (comments[0], "%s RAM", APPNAME);
			snprintf (comments[1], 32, romFilename);
			SetMCSaveComments(comments);
		}

		offset = SaveFile(filepath, datasize, method, silent);

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
SaveRAMAuto (int method, bool silent)
{
	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_RAM, method, romFilename, 0))
		return false;

	return SaveRAM(filepath, method, silent);
}

bool LoadRAM (char * filepath, int method, bool silent)
{
	int offset = 0;
	bool retval = false;

	if(nesGameType == 4) // RAM saves don't exist for FDS games
		return false;

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent); // we use 'Save' because we need R/W

	if(method == METHOD_AUTO)
		return false;

	AllocSaveBuffer ();

	offset = LoadFile(filepath, method, silent);

	if (offset > 0)
	{
		if(nesGameType == 1)
			NGCFCEU_GameSave(&iNESCart, 1, method);
		else if(nesGameType == 2)
			NGCFCEU_GameSave(&UNIFCart, 1, method);

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
LoadRAMAuto (int method, bool silent)
{
	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	char filepath[MAXPATHLEN];
	char fullpath[MAXPATHLEN];
	char filepath2[MAXPATHLEN];
	char fullpath2[MAXPATHLEN];

	// look for Auto save file
	if(!MakeFilePath(filepath, FILE_RAM, method, romFilename, 0))
		return false;

	if (LoadRAM(filepath, method, silent))
		return true;

	// look for file with no number or Auto appended
	if(!MakeFilePath(filepath2, FILE_RAM, method, romFilename, -1))
		return false;

	if(LoadRAM(filepath2, method, silent))
	{
		// rename this file - append Auto
		sprintf(fullpath, "%s%s", rootdir, filepath); // add device to path
		sprintf(fullpath2, "%s%s", rootdir, filepath2); // add device to path
		rename(fullpath2, fullpath); // rename file (to avoid duplicates)
		return true;
	}
	return false;
}
