/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceustate.cpp
 *
 * Memory Based Load/Save State Manager
 *
 * These are simply the state routines, brought together as GCxxxxx
 * The original file I/O is replaced with Memory Read/Writes to the
 * statebuffer below
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <zlib.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "menu.h"
#include "filebrowser.h"
#include "fileop.h"
#include "fceustate.h"
#include "videosupport.h"

bool SaveState (char * filepath, bool silent)
{
	bool retval = false;
	int datasize;
	int offset = 0;
	int device;
			
	if(!FindDevice(filepath, &device))
		return 0;

	if(gameScreenPng.size > 0)
	{
		char screenpath[1024];
		snprintf(screenpath, sizeof(screenpath), "%s", filepath);
		screenpath[strlen(screenpath)-4] = 0;
		strcat(screenpath, ".png");
		SaveFile((char *)gameScreenPng.buffer, screenpath, gameScreenPng.size, silent);
	}

	EMUFILE_MEMFILE save(SAVEBUFFERSIZE);
	FCEUSS_SaveMS(&save, Z_BEST_COMPRESSION);
	datasize = save.size();

	if (datasize)
		offset = SaveFile(save.buf(), filepath, datasize, silent);

	if (offset > 0)
	{
		if (!silent)
			InfoPrompt("Save successful");
		retval = true;
	}
	return retval;
}

/****************************************************************************
 * Deferred auto-save
 *
 * SnapshotStateAuto() serializes and compresses the state and copies the
 * screenshot, so it can be called from the main thread at the moment the game
 * is left. WriteStateSnapshot() then does the slow part - the device I/O -
 * and can run whenever, on any thread, even after another game has been
 * loaded and the screenshot has been cleared.
 ***************************************************************************/
struct StateSnapshot
{
	char path[MAXPATHLEN];
	unsigned char * data; // compressed state
	int size;
	unsigned char * png; // screenshot, or nullptr
	int pngSize;
};

StateSnapshot * SnapshotStateAuto ()
{
	StateSnapshot * snapshot = (StateSnapshot *)calloc(1, sizeof(StateSnapshot));

	if(!snapshot)
		return nullptr;

	if(!MakeFilePath(snapshot->path, FILE_STATE, romFilename, 0))
	{
		FreeStateSnapshot(snapshot);
		return nullptr;
	}

	{
		EMUFILE_MEMFILE save(SAVEBUFFERSIZE);

		if(save.buf())
		{
			FCEUSS_SaveMS(&save, Z_BEST_COMPRESSION);

			int datasize = save.size();

			if(datasize > 0)
			{
				// keep only what was actually used
				snapshot->data = (unsigned char *)malloc(datasize);

				if(snapshot->data)
				{
					memcpy(snapshot->data, save.buf(), datasize);
					snapshot->size = datasize;
				}
			}
		}
	}

	if(!snapshot->data)
	{
		FreeStateSnapshot(snapshot);
		return nullptr;
	}

	if(gameScreenPng.size > 0 && gameScreenPng.buffer)
	{
		snapshot->png = (unsigned char *)malloc(gameScreenPng.size);

		if(snapshot->png)
		{
			memcpy(snapshot->png, gameScreenPng.buffer, gameScreenPng.size);
			snapshot->pngSize = gameScreenPng.size;
		}
	}

	return snapshot;
}

bool WriteStateSnapshot (StateSnapshot * snapshot, bool silent)
{
	int device;

	if(!snapshot || !snapshot->data || !FindDevice(snapshot->path, &device))
		return false;

	if(snapshot->png)
	{
		char screenpath[MAXPATHLEN];
		snprintf(screenpath, sizeof(screenpath), "%s", snapshot->path);
		screenpath[strlen(screenpath)-4] = 0;
		strcat(screenpath, ".png");
		SaveFile((char *)snapshot->png, screenpath, snapshot->pngSize, silent);
	}

	return SaveFile((char *)snapshot->data, snapshot->path, snapshot->size, silent) > 0;
}

void FreeStateSnapshot (StateSnapshot * snapshot)
{
	if(!snapshot)
		return;

	free(snapshot->png);
	free(snapshot->data);
	free(snapshot);
}

bool
SaveStateAuto (bool silent)
{
	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_STATE, romFilename, 0))
		return false;

	return SaveState(filepath, silent);
}

bool LoadState (char * filepath, bool silent)
{
	int offset = 0;
	bool retval = false;
	int device;

	if(!FindDevice(filepath, &device))
		return 0;

	AllocSaveBuffer ();

	offset = LoadFile(filepath, silent);

	if (offset > 0)
	{
		EMUFILE_MEMFILE save(savebuffer, offset);
		FCEUSS_LoadFP(&save, SSLOADPARAM_NOBACKUP);
		retval = true;
	}
	else
	{
		// if we reached here, nothing was done!
		if(!silent)
			ErrorPrompt ("State file not found");
	}
	FreeSaveBuffer ();
	return retval;
}

bool
LoadStateAuto (bool silent)
{
	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_STATE, romFilename, 0))
		return false;

	return LoadState(filepath, silent);
}

bool SavePreviewImg (char * filepath, bool silent)
{
	int device;
	
	if(!FindDevice(filepath, &device))
		return 0;

	if(gameScreenPng.size > 0)
	{
		char screenpath[1024];
		snprintf(screenpath, sizeof(screenpath), "%s", filepath);
		screenpath[strlen(screenpath)] = 0;
		strcat(screenpath, ".png");
		SaveFile((char *)gameScreenPng.buffer, screenpath, gameScreenPng.size, silent);
	}
	
	return 1;
}
