/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceustate.c
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
#include "types.h"
#include "state.h"

#include "images/saveicon.h"
#include "fceugx.h"
#include "intl.h"
#include "menudraw.h"
#include "filesel.h"
#include "memcardop.h"
#include "fileop.h"
#include "smbop.h"

/*** External functions ***/
extern void FCEUPPU_SaveState(void);
extern void FCEUSND_SaveState(void);
extern void FlipByteOrder(uint8 *src, uint32 count);
extern void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
extern void FCEU_ResetPalette(void);
extern void FCEUI_DisableSpriteLimitation( int a );

/*** External save structures ***/
extern SFORMAT SFCPU[];
extern SFORMAT SFCPUC[];
extern SFORMAT FCEUPPU_STATEINFO[];
extern SFORMAT FCEUCTRL_STATEINFO[];
extern SFORMAT FCEUSND_STATEINFO[];
extern SFORMAT SFMDATA[64];
extern u32 iNESGameCRC32;

#define RLSB 0x80000000
#define FILESIZEOFFSET 2116

int sboffset;				/*** Used as a basic fileptr  ***/
int mcversion = 0x981211;

/****************************************************************************
 * Memory based file functions
 ****************************************************************************/

/*** Open a file ***/
void memopen() {
    sboffset = 0;
    memset(savebuffer, 0, SAVEBUFFERSIZE);
}

/*** Close a file ***/
void memclose() {
    sboffset = 0;
}

/*** Write to the file ***/
void memfwrite( void *buffer, int len ) {
    if ( (sboffset + len ) > SAVEBUFFERSIZE)
        WaitPrompt("Buffer Exceeded");

    if ( len > 0 ) {
        memcpy(&savebuffer[sboffset], buffer, len );
        sboffset += len;
    }
}

/*** Read from a file ***/
void memfread( void *buffer, int len ) {

    if ( ( sboffset + len ) > SAVEBUFFERSIZE)
        WaitPrompt("Buffer exceeded");

    if ( len > 0 ) {
        memcpy(buffer, &savebuffer[sboffset], len);
        sboffset += len;
    }
}

/****************************************************************************
 * GCReadChunk
 *
 * Read the array of SFORMAT structures to memory
 ****************************************************************************/
int GCReadChunk( int chunkid, SFORMAT *sf ) {
    int csize;
    static char chunk[6];
    int chunklength;
    int thischunk;
    char info[128];

    memfread(&chunk, 4);
    memfread(&thischunk, 4);
    memfread(&chunklength, 4);

    if (strcmp(chunk, "CHNK") == 0) {
        if (chunkid == thischunk) {
            /*** Now decode the array of chunks to this one ***/
            while (sf->v) {
                memfread(&chunk, 4);
                if ( memcmp(&chunk, "CHKE", 4) == 0 )
                    return 1;

                if (memcmp(&chunk, sf->desc, 4) == 0) {
                    memfread(&csize, 4);
                    if (csize == (sf->s & (~RLSB ))) {
                        memfread( sf->v, csize );
                        sprintf(info,"%s %d", chunk, csize);
                    } else {
                        WaitPrompt("Bad chunk link");
                        return 0;
                    }
                } else {
                    sprintf(info, "No Sync %s %s", chunk, sf->desc);
                    WaitPrompt(info);
                    return 0;
                }
                sf++;
            }
        } else
            return 0;
    } else
        return 0;

    return 1;
}

/****************************************************************************
 * GCFCEUSS_Load
 *
 * Reads the SFORMAT arrays
 ****************************************************************************/
int GCFCEUSS_Load() {
    int totalsize = 0;

    sboffset = 16 + sizeof(saveicon) + 64;	/*** Reset memory file pointer ***/

    memcpy(&totalsize, &savebuffer[FILESIZEOFFSET], 4);

    /*** Now read the chunks back ***/
    if (GCReadChunk(1, SFCPU)) {
        if (GCReadChunk(2, SFCPUC)) {
            if (GCReadChunk(3, FCEUPPU_STATEINFO)) {
                if (GCReadChunk(4, FCEUCTRL_STATEINFO)) {
                    if (GCReadChunk(5, FCEUSND_STATEINFO)) {
                        if (GCReadChunk(0x10, SFMDATA))
                            return 1;
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
int GCSaveChunk(int chunkid, SFORMAT *sf) {
    int chnkstart;
    int csize = 0;
    int chsize = 0;
    char chunk[] = "CHNK";

    /*** Add chunk marker ***/
    memfwrite(&chunk, 4);
    memfwrite(&chunkid, 4);
    chnkstart = sboffset;		/*** Save ptr ***/
    sboffset += 4;			/*** Space for length ***/
    csize += 12;

    /*** Now run through this structure ***/
    while (sf->v) {
        /*** Check that there is a decription ***/
        if ( sf->desc == NULL)
            break;

        /*** Write out the description ***/
        memfwrite( sf->desc, 4);

        /*** Write the length of this chunk ***/
        chsize = ( sf->s & (~RLSB) );
        memfwrite( &chsize, 4);

        if ( chsize > 0 )
            /*** Write the actual data ***/
            memfwrite( sf->v, chsize );

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

int GCFCEUSS_Save()
{
    int totalsize = 0;
    static unsigned char header[16] = "FCS\xff";
    char chunk[] = "CHKE";
    int zero = 0;
    char Comment[2][100] = { { MENU_CREDITS_TITLE }, { "GAME" } };

    memopen();	/*** Reset Memory File ***/

    /*** Add version ID ***/
    memcpy(&header[8], &mcversion, 4);

    /*** Do internal Saving ***/
    FCEUPPU_SaveState();
    FCEUSND_SaveState();

    /*** Write Icon ***/
    memfwrite(&saveicon, sizeof(saveicon));
    totalsize += sizeof(saveicon);

    /*** And Comments ***/
    strncpy (Comment[1],romFilename,31); // we only have 32 chars to work with!
    Comment[1][31] = 0;
    memfwrite(&Comment[0], 64);
    totalsize += 64;

    /*** Write header ***/
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

    /*** Add terminating CHNK ***/
    memfwrite(&chunk,4);
    memfwrite(&zero,4);
    totalsize += 8;

    /*** Update size element ***/
    memcpy(&savebuffer[FILESIZEOFFSET], &totalsize, 4);

    return totalsize;
}

bool SaveState (int method, bool silent)
{
	bool retval = false;
	char filepath[1024];
	int datasize;
	int offset = 0;

	if(method == METHOD_AUTO)
		method = autoSaveMethod();

	if (!MakeFilePath(filepath, FILE_STATE, method))
		return false;

	ShowAction ((char*) "Saving...");

	datasize = GCFCEUSS_Save();

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
	return retval;
}

bool LoadState (int method, bool silent)
{
	char filepath[1024];
	int offset = 0;

	if(method == METHOD_AUTO)
		method = autoSaveMethod(); // we use 'Save' because we need R/W

	if (!MakeFilePath(filepath, FILE_STATE, method))
		return false;

	ShowAction ((char*) "Loading...");

	offset = LoadFile(filepath, method, silent);

	if (offset > 0)
	{
		GCFCEUSS_Load();
		return 1;
	}

	// if we reached here, nothing was done!
	if(!silent)
		WaitPrompt ((char*) "State file not found");

	return 0;
}
