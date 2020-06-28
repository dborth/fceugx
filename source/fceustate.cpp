/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceustate.cpp
 *
 * Memory Based Load/Save State Manager
 *
 * These are simply the state routines, brought together as GCxxxxx
 * The original file I/O is replaced with Memory Read/Writes to the
 * statebuffer below
 ****************************************************************************/

#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include <fat.h>
#include <zlib.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "menu.h"
#include "filebrowser.h"
#include "fileop.h"
#include "gcvideo.h"
#include "utils/pngu.h"

bool SaveState (char * filepath, bool silent)
{
	bool retval = false;
	int datasize;
	int offset = 0;
	int device;
			
	if(!FindDevice(filepath, &device))
		return 0;

	if(gameScreenPngSize > 0)
	{
		char screenpath[1024];
		strncpy(screenpath, filepath, 1024);
		screenpath[strlen(screenpath)-4] = 0;
		strcat(screenpath, ".png");
		SaveFile((char *)gameScreenPng, screenpath, gameScreenPngSize, silent);
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

	if(gameScreenPngSize > 0)
	{
		char screenpath[1024];
		strncpy(screenpath, filepath, 1024);
		screenpath[strlen(screenpath)] = 0;
		strcat(screenpath, ".png");
		SaveFile((char *)gameScreenPng, screenpath, gameScreenPngSize, silent);
	}
	
	return 1;
}
