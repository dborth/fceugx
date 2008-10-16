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

#include "fceuconfig.h"
#include "intl.h"
#include "menudraw.h"
#include "filesel.h"
#include "memcardop.h"
#include "fileop.h"
#include "smbop.h"

extern u32 iNESGameCRC32;
extern CartInfo iNESCart;

extern unsigned char savebuffer[SAVEBUFFERSIZE];
extern char romFilename[];

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
	ShowAction ((char*) "Saving...");

	if(method == METHOD_AUTO)
		method = autoSaveMethod();

	bool retval = false;
	char filepath[1024];
	int datasize;
	int offset = 0;

	datasize = NGCFCEU_GameSave(&iNESCart, 0); // save game save to savebuffer

	if ( datasize )
	{
		if(method == METHOD_SD || method == METHOD_USB)
		{
			if(ChangeFATInterface(method, NOTSILENT))
			{
				sprintf (filepath, "%s/%s/%s.sav", ROOTFATDIR, GCSettings.SaveFolder, romFilename);
				offset = SaveBufferToFAT (filepath, datasize, silent);
			}
		}
		else if(method == METHOD_SMB)
		{
			sprintf (filepath, "%s/%s.sav", GCSettings.SaveFolder, romFilename);
			offset = SaveBufferToSMB (filepath, datasize, silent);
		}
		else if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
		{
			sprintf (filepath, "%08x.sav", iNESGameCRC32);

			if(method == METHOD_MC_SLOTA)
				offset = SaveBufferToMC (savebuffer, CARD_SLOTA, filepath, datasize, silent);
			else
				offset = SaveBufferToMC (savebuffer, CARD_SLOTB, filepath, datasize, silent);
		}

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
	ShowAction ((char*) "Loading...");

	if(method == METHOD_AUTO)
		method = autoSaveMethod(); // we use 'Save' because we need R/W

	char filepath[1024];
	int offset = 0;

	if(method == METHOD_SD || method == METHOD_USB)
	{
		ChangeFATInterface(method, NOTSILENT);
		sprintf (filepath, "%s/%s/%s.sav", ROOTFATDIR, GCSettings.SaveFolder, romFilename);
		offset = LoadBufferFromFAT (filepath, silent);
	}
	else if(method == METHOD_SMB)
	{
		sprintf (filepath, "%s/%s.sav", GCSettings.SaveFolder, romFilename);
		offset = LoadSaveBufferFromSMB (filepath, silent);
	}
	else if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
	{
		sprintf (filepath, "%08x.sav", iNESGameCRC32);

		if(method == METHOD_MC_SLOTA)
			offset = LoadBufferFromMC (savebuffer, CARD_SLOTA, filepath, silent);
		else
			offset = LoadBufferFromMC (savebuffer, CARD_SLOTB, filepath, silent);
	}

	if (offset > 0)
	{
		NGCFCEU_GameSave(&iNESCart, 1); // load game save from savebuffer
		ResetNES();
		return 1;
	}

	// if we reached here, nothing was done!
	if(!silent)
		WaitPrompt ((char*) "Save file not found");

	return 0;
}
