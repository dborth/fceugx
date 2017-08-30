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
#include "fileop.h"
#include "pocketnes/goombasav.h"

static u32 WiiFCEU_GameSave(CartInfo *LocalHWInfo, int operation)
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
		datasize = WiiFCEU_GameSave(&iNESCart, 0);
	else if(GameInfo->type == GIT_VSUNI)
		datasize = WiiFCEU_GameSave(&UNIFCart, 0);

	if (datasize)
	{
		// Check to see if this is a PocketNES save file
		FILE* file = fopen(filepath, "rb");
		if (file)
		{
			uint32 tag;
			fread(&tag, sizeof(uint32), 1, file);
			fclose(file);
			
			if (goomba_is_sram(&tag))
			{
				void* gba_data = malloc(GOOMBA_COLOR_SRAM_SIZE);
				
				file = fopen(filepath, "rb");
				fread(gba_data, 1, GOOMBA_COLOR_SRAM_SIZE, file);
				fclose(file);
				
				void* cleaned = goomba_cleanup(gba_data);
				if (!cleaned) {
					ErrorPrompt(goomba_last_error());
				} else if (cleaned != gba_data) {
					memcpy(gba_data, cleaned, GOOMBA_COLOR_SRAM_SIZE);
					free(cleaned);
				}

				// Look for just one save file. If there aren't any, or there is more than one, don't read any data.
				const stateheader* sh1 = NULL;
				const stateheader* sh2 = NULL;

				const stateheader* sh = stateheader_first(gba_data);
				while (sh && stateheader_plausible(sh)) {
					if (little_endian_conv_16(sh->type) != GOOMBA_SRAMSAVE) {}
					else if (sh1 == NULL) {
						sh1 = sh;
					}
					else {
						sh2 = sh;
						break;
					}
					sh = stateheader_advance(sh);
				}

				if (sh1 == NULL)
				{
					ErrorPrompt("PocketNES save file has no SRAM.");
					datasize = 0;
				}
				else if (sh2 != NULL)
				{
					ErrorPrompt("PocketNES save file has more than one SRAM.");
					datasize = 0;
				}
				else
				{
					char* newdata = goomba_new_sav(gba_data, sh1, savebuffer, datasize);
					if (!newdata) {
						ErrorPrompt(goomba_last_error());
						datasize = 0;
					} else {
						memcpy(savebuffer, newdata, GOOMBA_COLOR_SRAM_SIZE);
						datasize = GOOMBA_COLOR_SRAM_SIZE;
						free(newdata);
					}
				}
			}
		}
	}

	if (datasize)
	{
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

	// Check to see if this is a PocketNES save file
	if (goomba_is_sram(savebuffer))
	{
		void* cleaned = goomba_cleanup(savebuffer);
		if (!cleaned) {
			ErrorPrompt(goomba_last_error());
		} else if (cleaned != savebuffer) {
			memcpy(savebuffer, cleaned, GOOMBA_COLOR_SRAM_SIZE);
			free(cleaned);
		}
		
		// Look for just one save file. If there aren't any, or there is more than one, don't read any data.
		const stateheader* sh1 = NULL;
		const stateheader* sh2 = NULL;

		const stateheader* sh = stateheader_first(savebuffer);
		while (sh && stateheader_plausible(sh)) {
			if (little_endian_conv_16(sh->type) != GOOMBA_SRAMSAVE) { }
			else if (sh1 == NULL) {
				sh1 = sh;
			}
			else {
				sh2 = sh;
				break;
			}
			sh = stateheader_advance(sh);
		}

		if (sh1 == NULL)
		{
			ErrorPrompt("PocketNES save file has no SRAM.");
			offset = 0;
		}
		else if (sh2 != NULL)
		{
			ErrorPrompt("PocketNES save file has more than one SRAM.");
			offset = 0;
		}
		else
		{
			goomba_size_t len;
			void* extracted = goomba_extract(savebuffer, sh1, &len);
			if (!extracted)
				ErrorPrompt(goomba_last_error());
			else
			{
				memcpy(savebuffer, extracted, len);
				offset = len;
				free(extracted);
			}
		}
	}

	if (offset > 0)
	{
		if(GameInfo->type == GIT_CART)
			WiiFCEU_GameSave(&iNESCart, 1);
		else if(GameInfo->type == GIT_VSUNI)
			WiiFCEU_GameSave(&UNIFCart, 1);

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
