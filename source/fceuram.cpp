/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
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

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <string.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "menu.h"
#include "filebrowser.h"
#include "fileop.h"
#include "fceuram.h"
#include "pocketnes/goombasav.h"

// Copies the cart's battery-backed RAM to (operation 0) or from (operation 1) buffer.
// With operation 0 and a null buffer nothing is copied - it just returns the size.
static u32 WiiFCEU_GameSave(CartInfo *LocalHWInfo, int operation, unsigned char * buffer)
{
	u32 offset = 0;

	if(LocalHWInfo->battery && !LocalHWInfo->SaveGame.empty())
	{
		for (size_t x = 0; x < LocalHWInfo->SaveGame.size(); x++)
		{
			if(LocalHWInfo->SaveGame[x].bufptr)
			{
				if(operation == 0) // save to file
				{
					if(buffer)
						memcpy(buffer+offset, LocalHWInfo->SaveGame[x].bufptr, LocalHWInfo->SaveGame[x].buflen);
				}
				else // load from file
					memcpy(LocalHWInfo->SaveGame[x].bufptr, buffer+offset, LocalHWInfo->SaveGame[x].buflen);
				offset += LocalHWInfo->SaveGame[x].buflen;
			}
		}
	}
	return offset;
}

// Copies the current game's battery-backed RAM into buffer (null: just measure it)
// \return size in bytes
static int GetGameRAM(unsigned char * buffer)
{
	if(GameInfo->type == GIT_CART)
		return WiiFCEU_GameSave(&iNESCart, 0, buffer);
	else if(GameInfo->type == GIT_VSUNI)
		return WiiFCEU_GameSave(&UNIFCart, 0, buffer);
	return 0;
}

// Writes the RAM that is sitting in the savebuffer (datasize bytes) to filepath,
// merging it into the existing file if that is a PocketNES save.
// The caller holds the savebuffer (AllocSaveBuffer).
static bool WriteRAMFromSaveBuffer (char * filepath, int datasize, bool silent)
{
	bool retval = false;
	int offset = 0;

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
				const stateheader* sh1 = nullptr;
				const stateheader* sh2 = nullptr;

				const stateheader* sh = stateheader_first(gba_data);
				while (sh && stateheader_plausible(sh)) {
					if (little_endian_conv_16(sh->type) != GOOMBA_SRAMSAVE) {}
					else if (sh1 == nullptr) {
						sh1 = sh;
					}
					else {
						sh2 = sh;
						break;
					}
					sh = stateheader_advance(sh);
				}

				if (sh1 == nullptr)
				{
					ErrorPrompt("PocketNES save file has no SRAM.");
					datasize = 0;
				}
				else if (sh2 != nullptr)
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
	return retval;
}

bool SaveRAM (char * filepath, bool silent)
{
	bool retval = false;
	int datasize = 0;
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
	datasize = GetGameRAM(savebuffer);

	retval = WriteRAMFromSaveBuffer(filepath, datasize, silent);

	FreeSaveBuffer ();
	return retval;
}

/****************************************************************************
 * Deferred auto-save
 *
 * SnapshotRAMAuto() copies everything that is needed (the RAM and where it
 * goes) so it can be called from the main thread at the moment the game is
 * left. WriteRAMSnapshot() then does the slow part - the device I/O - and
 * can run whenever, on any thread, even after another game has been loaded.
 ***************************************************************************/
struct RAMSnapshot
{
	char path[MAXPATHLEN];
	unsigned char * data;
	int size;
};

RAMSnapshot * SnapshotRAMAuto ()
{
	if(GameInfo->type == GIT_FDS) // RAM saves don't exist for FDS games
		return nullptr;

	int size = GetGameRAM(nullptr);

	if(size <= 0)
		return nullptr;

	RAMSnapshot * snapshot = (RAMSnapshot *)calloc(1, sizeof(RAMSnapshot));

	if(!snapshot)
		return nullptr;

	snapshot->data = (unsigned char *)malloc(size);

	if(!snapshot->data || !MakeFilePath(snapshot->path, FILE_RAM, romFilename, 0))
	{
		FreeRAMSnapshot(snapshot);
		return nullptr;
	}

	GetGameRAM(snapshot->data);
	snapshot->size = size;
	return snapshot;
}

bool WriteRAMSnapshot (RAMSnapshot * snapshot, bool silent)
{
	int device;

	if(!snapshot || !snapshot->data || !FindDevice(snapshot->path, &device))
		return false;

	AllocSaveBuffer ();
	memcpy(savebuffer, snapshot->data, snapshot->size);
	bool retval = WriteRAMFromSaveBuffer(snapshot->path, snapshot->size, silent);
	FreeSaveBuffer ();
	return retval;
}

void FreeRAMSnapshot (RAMSnapshot * snapshot)
{
	if(!snapshot)
		return;

	free(snapshot->data);
	free(snapshot);
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
		const stateheader* sh1 = nullptr;
		const stateheader* sh2 = nullptr;

		const stateheader* sh = stateheader_first(savebuffer);
		while (sh && stateheader_plausible(sh)) {
			if (little_endian_conv_16(sh->type) != GOOMBA_SRAMSAVE) { }
			else if (sh1 == nullptr) {
				sh1 = sh;
			}
			else {
				sh2 = sh;
				break;
			}
			sh = stateheader_advance(sh);
		}

		if (sh1 == nullptr)
		{
			ErrorPrompt("PocketNES save file has no SRAM.");
			offset = 0;
		}
		else if (sh2 != nullptr)
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
			WiiFCEU_GameSave(&iNESCart, 1, savebuffer);
		else if(GameInfo->type == GIT_VSUNI)
			WiiFCEU_GameSave(&UNIFCart, 1, savebuffer);

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

	if (!EmuSettings.appendAuto)
		return false;

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
