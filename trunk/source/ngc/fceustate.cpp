/****************************************************************************
 * FCE Ultra 0.98.12
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
#include "pngu/pngu.h"

#include "fceugx.h"
#include "fceusupport.h"
#include "menu.h"
#include "filebrowser.h"
#include "memcardop.h"
#include "fileop.h"
#include "gcvideo.h"

#define RLSB 0x80000000

/****************************************************************************
 * Memory based file functions
 ****************************************************************************/
static int sboffset; // Used as a basic fileptr

/*** Open a file ***/
static void memopen()
{
	sboffset = 0;
}

/*** Write to the file ***/
static void memfwrite(void *buffer, int len)
{
	if ((sboffset + len) > SAVEBUFFERSIZE)
		ErrorPrompt("Buffer Exceeded");

	if (len > 0)
	{
		memcpy(&savebuffer[sboffset], buffer, len);
		sboffset += len;
	}
}

/*** Read from a file ***/
static void memfread(void *buffer, int len)
{
	if ((sboffset + len) > SAVEBUFFERSIZE)
		ErrorPrompt("Buffer exceeded");

	if (len > 0)
	{
		memcpy(buffer, &savebuffer[sboffset], len);
		sboffset += len;
	}
}

/****************************************************************************
 * GCReadChunk
 *
 * Read the array of SFORMAT structures to memory
 ****************************************************************************/
static int GCReadChunk(int chunkid, SFORMAT *sf)
{
	uint32 csize;
	static char chunk[6];
	int chunklength;
	int thischunk;
	char info[128];

	memfread(&chunk, 4);
	memfread(&thischunk, 4);
	memfread(&chunklength, 4);

	if (memcmp(&chunk, "CHNK", 4) == 0)
	{
		if (chunkid == thischunk)
		{
			/*** Now decode the array of chunks to this one ***/
			while (sf->v)
			{
				memfread(&chunk, 4);
				if (memcmp(&chunk, "CHKE", 4) == 0)
					return 1;

				if (memcmp(&chunk, sf->desc, 4) == 0)
				{
					memfread(&csize, 4);
					if (csize == (sf->s & (~RLSB)))
					{
						memfread(sf->v, csize);
						sprintf(info, "%s %d", chunk, csize);
					}
					else
					{
						ErrorPrompt("Bad chunk link");
						return 0;
					}
				}
				else
				{
					sprintf(info, "No Sync %s %s", chunk, sf->desc);
					ErrorPrompt(info);
					return 0;
				}
				sf++;
			}
		}
		else
			return 0;
	}
	else
		return 0;

	return 1;
}

/****************************************************************************
 * GCFCEUSS_Load
 *
 * Reads the SFORMAT arrays
 ****************************************************************************/
static int GCFCEUSS_Load(int method)
{
	memopen(); // reset file pointer

	sboffset += 16; // skip FCEU header

	// Now read the chunks back
	if (GCReadChunk(1, SFCPU))
	{
		if (GCReadChunk(2, SFCPUC))
		{
			X.mooPI = X.P; // Quick and dirty hack.
			if (GCReadChunk(3, FCEUPPU_STATEINFO))
			{
				if (GCReadChunk(4, FCEUCTRL_STATEINFO))
				{
					if (GCReadChunk(5, FCEUSND_STATEINFO))
					{
						if (GCReadChunk(0x10, SFMDATA))
						{
							if (GameStateRestore)
								GameStateRestore(FCEU_VERSION_NUMERIC);

							FCEUPPU_LoadState(FCEU_VERSION_NUMERIC);
							FCEUSND_LoadState(FCEU_VERSION_NUMERIC);
							return 1;
						}
					}
				}
			}
		}
	}

	return 0;
}

/****************************************************************************
 * GCSaveChunk
 *
 * Write the array of SFORMAT structures to the file
 ****************************************************************************/
static int GCSaveChunk(int chunkid, SFORMAT *sf)
{
	int chnkstart;
	int csize = 0;
	int chsize = 0;
	char chunk[] = "CHNK";

	/*** Add chunk marker ***/
	memfwrite(&chunk, 4);
	memfwrite(&chunkid, 4);
	chnkstart = sboffset; /*** Save ptr ***/
	sboffset += 4; /*** Space for length ***/
	csize += 12;

	/*** Now run through this structure ***/
	while (sf->v)
	{
		/*** Check that there is a decription ***/
		if (sf->desc == NULL)
			break;

		/*** Write out the description ***/
		memfwrite(sf->desc, 4);

		/*** Write the length of this chunk ***/
		chsize = (sf->s & (~RLSB));
		memfwrite(&chsize, 4);

		if (chsize > 0)
		{
			/*** Write the actual data ***/
			memfwrite(sf->v, chsize);
		}

		csize += 8;
		csize += chsize;

		sf++;
	}

	/*** Update CHNK length ***/
	memcpy(&savebuffer[chnkstart], &csize, 4);

	return csize;
}

/****************************************************************************
 * GCFCEUSS_Save
 *
 * This is a modified version of FCEUSS_Save
 * It uses memory for it's I/O and has an added CHNK block.
 * The file is terminated with CHNK length of 0.
 ****************************************************************************/
extern void (*SPreSave)(void);
extern void (*SPostSave)(void);

static int GCFCEUSS_Save(int method)
{
	int totalsize = 0;
	unsigned char header[16] = "FCS\xff";
	char chunk[] = "CHKE";
	int zero = 0;
	int version = 0x981211;
	memcpy(&header[8], &version, 4); // Add version ID

	memopen(); // Reset Memory File

	// Do internal Saving
	FCEUPPU_SaveState();
	FCEUSND_SaveState();

	// Write header
	memfwrite(&header, 16);
	totalsize += 16;
	totalsize += GCSaveChunk(1, SFCPU);
	totalsize += GCSaveChunk(2, SFCPUC);
	totalsize += GCSaveChunk(3, FCEUPPU_STATEINFO);
	totalsize += GCSaveChunk(4, FCEUCTRL_STATEINFO);
	totalsize += GCSaveChunk(5, FCEUSND_STATEINFO);

	if(nesGameType == 4) // FDS
		SPreSave();

	totalsize += GCSaveChunk(0x10, SFMDATA);

	if(nesGameType == 4) // FDS
		SPostSave();

	// Add terminating CHNK
	memfwrite(&chunk,4);
	memfwrite(&zero,4);
	totalsize += 8;

	return totalsize;
}

bool SaveState (char * filepath, int method, bool silent)
{
	bool retval = false;
	int datasize;
	int offset = 0;
	int imgSize = 0; // image screenshot bytes written

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	// save screenshot - I would prefer to do this from gameScreenTex
	if(gameScreenTex2 != NULL && method != METHOD_MC_SLOTA && method != METHOD_MC_SLOTB)
	{
		AllocSaveBuffer ();

		IMGCTX pngContext = PNGU_SelectImageFromBuffer(savebuffer);

		if (pngContext != NULL)
		{
			imgSize = PNGU_EncodeFromGXTexture(pngContext, screenwidth, screenheight, gameScreenTex2, 0);
			PNGU_ReleaseImageContext(pngContext);
		}

		if(imgSize > 0)
		{
			char screenpath[1024];
			strncpy(screenpath, filepath, 1024);
			screenpath[strlen(screenpath)-4] = 0;
			sprintf(screenpath, "%s.png", screenpath);
			SaveFile(screenpath, imgSize, method, silent);
		}

		FreeSaveBuffer ();
	}

	AllocSaveBuffer ();

	datasize = GCFCEUSS_Save(method);

	if (datasize)
	{
		if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
		{
			// Set the comments
			char comments[2][32];
			memset(comments, 0, 64);
			sprintf (comments[0], "%s State", APPNAME);
			snprintf (comments[1], 32, romFilename);
			SetMCSaveComments(comments);
		}

		offset = SaveFile(filepath, datasize, method, silent);
	}
	FreeSaveBuffer ();

	if (offset > 0)
	{
		if (!silent)
			InfoPrompt("Save successful");
		retval = true;
	}
	return retval;
}

bool
SaveStateAuto (int method, bool silent)
{
	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_STATE, method, romFilename, 0))
		return false;

	return SaveState(filepath, method, silent);
}

bool LoadState (char * filepath, int method, bool silent)
{
	int offset = 0;
	bool retval = false;

	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent); // we use 'Save' because we need R/W

	if(method == METHOD_AUTO)
		return false;

	AllocSaveBuffer ();

	offset = LoadFile(filepath, method, silent);

	if (offset > 0)
	{
		GCFCEUSS_Load(method);
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
LoadStateAuto (int method, bool silent)
{
	if(method == METHOD_AUTO)
		method = autoSaveMethod(silent);

	if(method == METHOD_AUTO)
		return false;

	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_STATE, method, romFilename, 0))
		return false;

	return LoadState(filepath, method, silent);
}
