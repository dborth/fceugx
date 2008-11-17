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

#include "types.h"

#include "fceu.h"
#include "ppu.h"

#include "cart.h"
#include "memory.h"
#include "x6502.h"

#include "general.h"

#include "fceugx.h"
#include "intl.h"
#include "menudraw.h"
#include "filesel.h"
#include "memcardop.h"
#include "fileop.h"
#include "smbop.h"

extern u32 iNESGameCRC32;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;

int NGCFCEU_GameSave(CartInfo *LocalHWInfo, int operation)
{
	int size = 0;
	if(LocalHWInfo->battery && LocalHWInfo->SaveGame[0])
	{
		int x;

		for(x=0;x<4;x++)
		{
			if(LocalHWInfo->SaveGame[x])
			{
				if(operation == 0) // save to file
				{
					memcpy(savebuffer, LocalHWInfo->SaveGame[x], LocalHWInfo->SaveGameLen[x]);
				}
				else // load from file
				{
					memcpy(LocalHWInfo->SaveGame[x], savebuffer, LocalHWInfo->SaveGameLen[x]);
				}
				size += LocalHWInfo->SaveGameLen[x];
			}
		}
	}
	return size;
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
			WaitPrompt((char *)"Saving is not available for FDS games!");
		return false;
	}

	if(method == METHOD_AUTO)
		method = autoSaveMethod();

	if (!MakeFilePath(filepath, FILE_RAM, method))
		return false;

	ShowAction ((char*) "Saving...");

	// save game save to savebuffer
	if(nesGameType == 1)
		datasize = NGCFCEU_GameSave(&iNESCart, 0);
	else if(nesGameType == 2)
		datasize = NGCFCEU_GameSave(&UNIFCart, 0);

	if (datasize)
	{
		offset = SaveFile(filepath, datasize, method, silent);

		if (offset > 0)
		{
			if ( !silent )
				WaitPrompt((char *)"Save successful");
			retval = true;
		}
	}
	else
	{
		if ( !silent )
			WaitPrompt((char *)"No data to save!");
	}
	return retval;
}

bool LoadRAM (int method, bool silent)
{
	char filepath[1024];
	int offset = 0;

	if(nesGameType == 4)
	{
		if(!silent)
			WaitPrompt((char *)"Saving is not available for FDS games!");
		return false;
	}

	if(method == METHOD_AUTO)
		method = autoSaveMethod(); // we use 'Save' because we need R/W

	if (!MakeFilePath(filepath, FILE_RAM, method))
		return false;

	ShowAction ((char*) "Loading...");

	offset = LoadFile(filepath, method, silent);

	if (offset > 0)
	{
		if(nesGameType == 1)
			NGCFCEU_GameSave(&iNESCart, 1);
		else if(nesGameType == 2)
			NGCFCEU_GameSave(&UNIFCart, 1);

		ResetNES();
		return true;
	}

	// if we reached here, nothing was done!
	if(!silent)
		WaitPrompt ((char*) "Save file not found");

	return false;
}
