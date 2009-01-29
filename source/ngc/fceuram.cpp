/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceustate.c
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

#include "images/saveicon.h"
#include "fceugx.h"
#include "menudraw.h"
#include "filesel.h"
#include "memcardop.h"
#include "fileop.h"

extern "C" {
#include "types.h"
#include "fceu.h"
#include "ppu.h"
#include "cart.h"
#include "x6502.h"
#include "general.h"
extern u32 iNESGameCRC32;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;
}

static u32 NGCFCEU_GameSave(CartInfo *LocalHWInfo, int operation, int method)
{
	u32 offset = 0;
	char comment[2][32];
	memset(comment, 0, 64);

	if(LocalHWInfo->battery && LocalHWInfo->SaveGame[0])
	{
		// add save icon and comments for Memory Card saves
		if(operation == 0 &&
			(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB))
		{
			// Copy in save icon
			memcpy(savebuffer, saveicon, sizeof(saveicon));
			offset += sizeof(saveicon);

			// And the comments
			sprintf (comment[0], "%s RAM", APPNAME);
			strncpy (comment[1],romFilename,31); // we only have 32 chars to work with!
			comment[1][31] = 0;
			memcpy(savebuffer+offset, comment, 64);
			offset += 64;
		}
		// skip save icon and comments for Memory Card saves
		else if(operation == 1 &&
			(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB))
		{
			offset += sizeof(saveicon);
			offset += 64; // sizeof prefscomment
		}

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

bool SaveRAM (int method, bool silent)
{
	bool retval = false;
	char filepath[1024];
	int datasize = 0;
	int offset = 0;

	if(nesGameType == 4)
	{
		if(!silent)
			WaitPrompt("Saving is not available for FDS games!");
		return false;
	}

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if (!MakeFilePath(filepath, FILE_RAM, method))
		return false;

	ShowAction ("Saving...");

	AllocSaveBuffer ();

	// save game save to savebuffer
	if(nesGameType == 1)
		datasize = NGCFCEU_GameSave(&iNESCart, 0, method);
	else if(nesGameType == 2)
		datasize = NGCFCEU_GameSave(&UNIFCart, 0, method);

	if (datasize)
	{
		offset = SaveFile(filepath, datasize, method, silent);

		if (offset > 0)
		{
			if ( !silent )
				WaitPrompt("Save successful");
			retval = true;
		}
	}
	else
	{
		if ( !silent )
			WaitPrompt("No data to save!");
	}
	FreeSaveBuffer ();
	return retval;
}

bool LoadRAM (int method, bool silent)
{
	char filepath[1024];
	int offset = 0;
	bool retval = false;

	if(nesGameType == 4)
	{
		if(!silent)
			WaitPrompt("Saving is not available for FDS games!");
		return false;
	}

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent); // we use 'Save' because we need R/W

	if (!MakeFilePath(filepath, FILE_RAM, method))
		return false;

	ShowAction ("Loading...");

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
			WaitPrompt ("Save file not found");
	}
	FreeSaveBuffer ();
	return retval;
}
