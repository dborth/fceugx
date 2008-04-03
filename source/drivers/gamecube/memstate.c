/****************************************************************************
 * Memory Based Load/Save State Manager
 *
 * These are simply the state routines, brought together as GCxxxxx
 * The original file I/O is replaced with Memory Read/Writes to the
 * statebuffer below
 ****************************************************************************/
#include <gccore.h>
#include <string.h>
#include <sdcard.h>
#include "../../types.h"
#include "../../state.h"
#include "saveicon.h"

#define SAVEDIR "fceu\\saves"

/*** External functions ***/
extern void FCEUPPU_SaveState(void);
extern void FCEUSND_SaveState(void);
extern void WaitPrompt( char *text );
extern void FlipByteOrder(uint8 *src, uint32 count);
extern void ShowAction( char *text );

/*** External save structures ***/
extern SFORMAT SFCPU[];
extern SFORMAT SFCPUC[];
extern SFORMAT FCEUPPU_STATEINFO[];
extern SFORMAT FCEUCTRL_STATEINFO[];
extern SFORMAT FCEUSND_STATEINFO[];
extern SFORMAT SFMDATA[64];
extern u32 iNESGameCRC32;
int CARDSLOT = CARD_SLOTA;

#define RLSB 0x80000000
#define FILESIZEOFFSET 2116

unsigned char statebuffer[64 * 1024] ATTRIBUTE_ALIGN(32);	/*** Never had one this big ! ***/
int sboffset;				/*** Used as a basic fileptr  ***/
int mcversion = 0x981211;

static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);

/****************************************************************************
 * Memory based file functions
 ****************************************************************************/

/*** Open a file ***/
void memopen()
{
	sboffset = 0;
	memset(&statebuffer[0], 0, sizeof(statebuffer));
}

/*** Close a file ***/
void memclose()
{
	sboffset = 0;
}

/*** Write to the file ***/
void memfwrite( void *buffer, int len )
{
	if ( (sboffset + len ) > sizeof(statebuffer))
		WaitPrompt("Buffer Exceeded");

	if ( len > 0 ) {
		memcpy(&statebuffer[sboffset], buffer, len );
		sboffset += len;
	}
}

/*** Read from a file ***/
void memfread( void *buffer, int len )
{

	if ( ( sboffset + len ) > sizeof(statebuffer))
		WaitPrompt("Buffer exceeded");

	if ( len > 0 ) {
		memcpy(buffer, &statebuffer[sboffset], len);
		sboffset += len;
	}
}

/****************************************************************************
 * GCReadChunk
 *
 * Read the array of SFORMAT structures to memory
 ****************************************************************************/

int GCReadChunk( int chunkid, SFORMAT *sf )
{
	int csize;
	static char chunk[6];
	int chunklength;
	int thischunk;
	char info[128];

	memfread(&chunk, 4);
	memfread(&thischunk, 4);
	memfread(&chunklength, 4);

	if ( strcmp(chunk, "CHNK") == 0 )
	{
		if ( chunkid == thischunk )
		{
			/*** Now decode the array of chunks to this one ***/
			while ( sf->v )
			{
				memfread(&chunk, 4);
				if ( memcmp(&chunk, "CHKE", 4) == 0 )
					return 1;
	
				if ( memcmp(&chunk, sf->desc, 4) == 0 )
				{
					memfread(&csize, 4);
					if ( csize == ( sf->s & ( ~RLSB ) ) )
					{
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
		}
		else
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

int GCFCEUSS_Load()
{
	int totalsize = 0;

	sboffset = 16 + sizeof(saveicon) + 64;	/*** Reset memory file pointer ***/

	memcpy(&totalsize, &statebuffer[FILESIZEOFFSET], 4);
	
	/*** Now read the chunks back ***/
	if ( GCReadChunk( 1, SFCPU ) )
	{
		if ( GCReadChunk( 2, SFCPUC ) )
		{
			if ( GCReadChunk( 3, FCEUPPU_STATEINFO ) )
			{
				if ( GCReadChunk( 4, FCEUCTRL_STATEINFO ) )
				{
					if ( GCReadChunk( 5, FCEUSND_STATEINFO ) )
					{
						
						if ( GCReadChunk( 0x10, SFMDATA ) )
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
int GCSaveChunk( int chunkid, SFORMAT *sf )
{
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
	while (sf->v) 
	{
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
	memcpy(&statebuffer[chnkstart], &csize, 4);

	return csize;
}

/****************************************************************************
 * GCFCEUSS_Save
 *
 * This is a modified version of FCEUSS_Save
 * It uses memory for it's I/O and has an added CHNK block.
 * The file is terminated with CHNK length of 0.
 ****************************************************************************/
int GCFCEUSS_Save()
{
	int totalsize = 0;
	static unsigned char header[16] = "FCS\xff";
	char chunk[] = "CHKE";
	int zero = 0;
	char Comment[2][32] = { { "FCEU GC Version 1.0.9" }, { "A GAME" } };

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
	sprintf(Comment[1], "NES CRC 0x%08x", iNESGameCRC32);
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
	totalsize += GCSaveChunk(0x10, SFMDATA);
	
	/*** Add terminating CHNK ***/
	memfwrite(&chunk,4);
	memfwrite(&zero,4);
	totalsize += 8;

	/*** Update size element ***/
	memcpy(&statebuffer[FILESIZEOFFSET], &totalsize, 4);

	return totalsize;
}

/****************************************************************************
 * Card Removed
 *
 * Straight copy from MemCard demo
 ****************************************************************************/
int CardReady = 0;
void CardRemoved(s32 chn,s32 result) {
	CARD_Unmount(chn);
	CardReady = 0;
}

/****************************************************************************
 * Snes9xGX Memcard
 ****************************************************************************/
void uselessinquiry()
{
        volatile long *udvd = ( volatile long *)0xCC006000;

        udvd[0] = 0;
        udvd[1] = 0;
        udvd[2] = 0x12000000;
        udvd[3] = 0;
        udvd[4] = 0x20;
        udvd[5] = 0x80000000;
        udvd[6] = 0x20;
        udvd[7] = 1;

        while ( udvd[7] & 1 );

}

int MountTheCard()
{
        int tries = 0;
        int CardError;
        while ( tries < 10 )
        {
                 *(unsigned long*)(0xcc006800) |= 1<<13;        /*** Disable Encryption ***/
                uselessinquiry();
                VIDEO_WaitVSync();
                CardError = CARD_Mount(CARDSLOT, SysArea, NULL);        /*** Don't need or want a callback ***/
                if ( CardError == 0 )
                        return 0;
                else {
                        EXI_ProbeReset();
                }
                tries++;
        }

        return 1;
}

/****************************************************************************
 * MemCard Save
 *
 * This is based on the code from libogc
 ****************************************************************************/

void MCManage(int mode, int slot)
{

	char mcFilename[80];
	int CardError;
	card_dir CardDir;
	card_file CardFile;
	int SectorSize;
	int found = 0;
	int FileSize;
	int actualSize;
	int savedBytes=0;
	char debug[128];

	CARDSLOT = slot;
	/*** Build the file name ***/
	sprintf(mcFilename, "FCEU-%08x.fcs", iNESGameCRC32);

	/*** Mount the Card ***/
	CARD_Init("FCEU", "00");
	
	/*** Try for memory card in slot A ***/
	CardError = CARD_Mount(CARDSLOT, SysArea, CardRemoved );

	if ( CardError >= 0 )
	{
		/*** Get card sector size ***/
		CardError = CARD_GetSectorSize(CARDSLOT, &SectorSize);				

		switch ( mode ) {

			case 0 : {	/*** Save Game ***/
				/*** Look for this file ***/
				CardError = CARD_FindFirst(CARDSLOT, &CardDir, true);

				found = 0;

				card_stat CardStatus;
				while ( CardError != CARD_ERROR_NOFILE )
				{
					CardError = CARD_FindNext(&CardDir);
					if ( strcmp(CardDir.filename, mcFilename) == 0 )
					found = 1;
				}

				/*** Determine number of sectors required ***/
				savedBytes = actualSize = GCFCEUSS_Save();
				sprintf(debug, "Saving in MC ...");
				ShowAction(debug);

				FileSize = ( actualSize / SectorSize ) * SectorSize;
				if ( actualSize % SectorSize )
					FileSize += SectorSize;

				/*** Now write the file out ***/
				if ( !found )
					CardError = CARD_Create(CARDSLOT, mcFilename, FileSize, &CardFile);
				else
					CardError = CARD_Open(CARDSLOT, mcFilename, &CardFile);

				CARD_GetStatus( CARDSLOT, CardFile.filenum, &CardStatus);
				CardStatus.icon_addr = 0;
				CardStatus.icon_fmt = 2;
				CardStatus.icon_speed = 1;
				CardStatus.comment_addr = sizeof(saveicon);
				CARD_SetStatus( CARDSLOT, CardFile.filenum, &CardStatus);

				/*** Haha! libogc only write one block at a time! ***/	
				if ( CardError == 0 )
				{
					int sbo = 0;
					while ( actualSize > 0 )
					{
						CardError = CARD_Write(&CardFile, &statebuffer[sbo], SectorSize, sbo );
						actualSize -= SectorSize;
						sbo += SectorSize;
					}

					CardError = CARD_Close(&CardFile);
					sprintf(debug, "Saved %d bytes successfully!", savedBytes);
					WaitPrompt(debug);
				} 
				else WaitPrompt("Save Failed!");

				CARD_Unmount(CARDSLOT);

				} 
				break;	/*** End save ***/

			case 1: {	/*** Load state ***/
                /*** Look for this file ***/
                CardError = CARD_FindFirst(CARDSLOT, &CardDir, true);
					
				memopen();	/*** Clear the buffer ***/

                found = 0;

                while ( CardError != CARD_ERROR_NOFILE )
                {
                    CardError = CARD_FindNext(&CardDir);
                    if ( strcmp(CardDir.filename, mcFilename) == 0 )
                       found = 1;
                }

				if ( found == 0 )
				{
					WaitPrompt("No Save Game Found");
					CARD_Unmount(CARDSLOT);
					return;
				}

				/*** Load the file into memory ***/
				CardError = CARD_Open(CARDSLOT, mcFilename, &CardFile);
				CardError = CARD_Read(&CardFile, &statebuffer, SectorSize, 0);
				
				/*** Get actual size of the file ***/
				memcpy(&actualSize, &statebuffer[FILESIZEOFFSET], 4);
				savedBytes = actualSize;
				
				int sbo = SectorSize;
				actualSize -= SectorSize;	
				while( actualSize > 0 )
				{
					CARD_Read(&CardFile, &statebuffer[sbo], SectorSize, sbo);
					actualSize -= SectorSize;
					sbo += SectorSize;
				}
				CARD_Close(&CardFile);

				/*** Finally, do load ***/
				GCFCEUSS_Load();

				CARD_Unmount(CARDSLOT);
				sprintf(debug, "Loaded %d bytes successfully!", savedBytes);
				WaitPrompt(debug);

				} 
				break;	/*** End load ***/
	
			default: break;	
		}
	} else {
		WaitPrompt("Cannot Mount Memory Card!");
	} 
}

void SD_Manage(int mode, int slot){

	sd_file *handle;
	char path[1024];
	char msg[128];
	int offset = 0;
	int filesize = 0;
	int len = 0;
	
	//sprintf (filepath, "dev%d:\\%s\\%08x.fcs", slot, SAVEDIR, iNESGameCRC32);
	sprintf (path, "dev%d:\\%08x.fcs", slot, iNESGameCRC32);
	
	if (mode == 0) ShowAction ("Saving STATE to SD...");
	else ShowAction ("Loading STATE from SD...");
	
	handle = (mode == 0) ? SDCARD_OpenFile (path, "wb") : SDCARD_OpenFile (path, "rb");
	
	if (handle == NULL){        
        sprintf(msg, "Couldn't open %s", path);
        WaitPrompt(msg);
        return;
    }
	
	if (mode == 0){ //Save
		filesize = GCFCEUSS_Save();

		len = SDCARD_WriteFile (handle, statebuffer, filesize);
		SDCARD_CloseFile (handle);

		if (len != filesize){
			sprintf (msg, "Error writing %s", path);
			WaitPrompt (msg);
			return;			
		}

		sprintf (msg, "Saved %d bytes successfully", filesize);
		WaitPrompt (msg);
	}
	else{ //Load
	
		memopen();
		while ((len = SDCARD_ReadFile (handle, &statebuffer[offset], 1024)) > 0) offset += len;
		SDCARD_CloseFile (handle);
		
		sprintf (msg, "Loaded %d bytes successfully", offset);
		WaitPrompt(msg);

		GCFCEUSS_Load();
		return ;
	}
}

void ManageState(int mode, int slot, int device){

	if (device == 0){
		MCManage(mode, slot);
	}
	else{
		SD_Manage(mode, slot);
	}
}
