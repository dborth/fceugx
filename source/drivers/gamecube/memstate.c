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
#include <malloc.h>
#include "../../types.h"
#include "../../state.h"
#include "saveicon.h"
#include "intl.h"

#ifdef HW_RVL
#include "wiisd/tff.h"
#include "wiisd/integer.h"
FATFS frontfs;
FILINFO finfo;
#endif
#define FCEUDIR "fceu"
#define SAVEDIR "saves"

/*** External functions ***/
extern void FCEUPPU_SaveState(void);
extern void FCEUSND_SaveState(void);
extern void WaitPrompt( char *text );
extern void FlipByteOrder(uint8 *src, uint32 count);
extern void ShowAction( char *text );
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
int CARDSLOT = CARD_SLOTA;

#define RLSB 0x80000000
#define FILESIZEOFFSET 2116

unsigned char statebuffer[64 * 1024] ATTRIBUTE_ALIGN(32);	/*** Never had one this big ! ***/
int sboffset;				/*** Used as a basic fileptr  ***/
int mcversion = 0x981211;

static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);
extern u8 ChosenSlot;
extern u8 ChosenDevice;
extern u8 screenscaler;
extern u8 currpal;
extern u8 slimit;
extern u8 timing;
extern u8 mpads[6];
extern u8 FSDisable;
extern u8 PADCAL;
extern u8 PADTUR;
extern u8 UseSDCARD;
extern u8 UseWiiSDCARD;

extern struct st_palettes {
    char *name, *desc;
    unsigned int data[64];
} *palettes;
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
    char Comment[2][100] = { { MENU_CREDITS_TITLE }, { "A GAME" } };

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
        //CardError = CARD_Mount(CARDSLOT, SysArea, NULL);        /*** Don't need or want a callback ***/
        CardError = CARD_Mount(CARDSLOT, SysArea, CardRemoved);
        if ( CardError == 0 )
            return 0;
        else {
            EXI_ProbeReset();
        }
        tries++;
    }

    return -1;
}

/****************************************************************************
 * MemCard Save
 *
 * This is based on the code from libogc
 ****************************************************************************/
void MC_ManageState(int mode, int slot) {
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
    CardError = MountTheCard();

    if ( CardError >= 0 ) {
        /*** Get card sector size ***/
        CardError = CARD_GetSectorSize(CARDSLOT, &SectorSize);				

        switch ( mode ) {
            case 0 : {	/*** Save Game ***/
                 /*** Look for this file ***/
                 CardError = CARD_FindFirst(CARDSLOT, &CardDir, true);

                 found = 0;
                 card_stat CardStatus;
                 while ( CardError != CARD_ERROR_NOFILE ) {
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
                 if ( CardError == 0 ) {
                     int sbo = 0;
                     while ( actualSize > 0 ) {
                         CardError = CARD_Write(&CardFile, &statebuffer[sbo], SectorSize, sbo );
                         actualSize -= SectorSize;
                         sbo += SectorSize;
                     }

                     CardError = CARD_Close(&CardFile);
                     sprintf(debug, "Saved %d bytes successfully!", savedBytes);
                     ShowAction(debug);
                 } 
                 else {
                    WaitPrompt("Save Failed");
                 }

                 CARD_Unmount(CARDSLOT);
             } 
             break;	/*** End save ***/

            case 1: {	/*** Load state ***/
                /*** Look for this file ***/
                CardError = CARD_FindFirst(CARDSLOT, &CardDir, true);
                memopen();	/*** Clear the buffer ***/
                found = 0;
                while ( CardError != CARD_ERROR_NOFILE ) {
                    CardError = CARD_FindNext(&CardDir);
                    if ( strcmp(CardDir.filename, mcFilename) == 0 )
                        found = 1;
                }

                if ( found == 0 ) {
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
                while( actualSize > 0 ) {
                    CARD_Read(&CardFile, &statebuffer[sbo], SectorSize, sbo);
                    actualSize -= SectorSize;
                    sbo += SectorSize;
                }
                CARD_Close(&CardFile);

                /*** Finally, do load ***/
                GCFCEUSS_Load();

                CARD_Unmount(CARDSLOT);
                sprintf(debug, "Loaded %d bytes successfully!", savedBytes);
                ShowAction(debug);
            } 
            break;	/*** End load ***/

            default: break;	
        }
    } else {
        WaitPrompt("Cannot Mount Memory Card!");
    } 
}

void SD_ManageState(int mode, int slot) {
    char path[1024];
    char msg[128];
    int offset = 0;
    int filesize = 0;
    int len = 0;

    if (slot < 2) {
        sd_file *handle;
        sprintf (path, "dev%d:\\%s\\%s\\%08x.fcs", ChosenSlot, FCEUDIR, SAVEDIR, iNESGameCRC32);

        if (mode == 0) ShowAction ("Saving STATE to SD...");
        else ShowAction ("Loading STATE from SD...");

        handle = SDCARD_OpenFile(path, (mode == 0) ? "wb" : "rb");

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
            ShowAction(msg);

            GCFCEUSS_Load();
            return ;
        }
    } else { // WiiSD
#ifdef HW_RVL
        if (mode == 0) ShowAction ("Saving State to WiiSD...");
        else ShowAction ("Loading State from WiiSD...");

        sprintf(path, "/%s/%s/%08X.fcs", FCEUDIR, SAVEDIR, iNESGameCRC32);
        FIL fp;
        int res;
        u32 offset = 0;

        /* Mount WiiSD with TinyFatFS*/
        if(f_mount(0, &frontfs) != FR_OK) {
            WaitPrompt((char*)"f_mount failed");
            return 0;
        }
        memset(&finfo, 0, sizeof(finfo));

        if (mode == 0)
            res = f_open(&fp, path, FA_CREATE_ALWAYS | FA_WRITE);
        else {
            if ((res=f_stat(path, &finfo)) != FR_OK) {
                if (res == FR_NO_FILE) {
                    sprintf(msg, "Unable to find %s.", path);
                }
                else {
                    sprintf(msg, "f_stat failed, error %d", res);
                }
                WaitPrompt(msg);
                return;
            }
            res = f_open(&fp, path, FA_READ);
        }

        if (res != FR_OK) {
            sprintf(msg, "Failed to open %s, error %d.", path, res);
            WaitPrompt(msg);
            return;
        }

        if (mode == 0) { // Save
            u32 written = 0, total_written = 0;

            filesize = GCFCEUSS_Save();
            sprintf(msg, "Writing %d bytes..", filesize);
            ShowAction(msg);

            offset = filesize;
            // Can only write 64k at a time
            while (offset > 65000) {
                if ((res = f_write(&fp, &statebuffer[total_written], 65000, &written)) != FR_OK) {
                    sprintf(msg, "f_write failed, error %d", res);
                    WaitPrompt(msg);
                    f_close(&fp);
                    return;
                }
                offset -= written;
                total_written += written;
            }
            // Write last 64k
            if ((res = f_write(&fp, statebuffer+total_written, offset, &written)) != FR_OK) {
                sprintf(msg, "f_write failed, error %d", res);
                WaitPrompt(msg);
                f_close(&fp);
                return;
            }
            offset -= written;
            total_written += written;
            if (total_written == filesize) {
                sprintf(msg, "Wrote %d bytes.", total_written);
                ShowAction(msg);
                f_close(&fp);
                return;
            }

            sprintf(msg, "Write size mismatch, %d of %d bytes", written, filesize);
            WaitPrompt(msg);
            sprintf(msg, "Unable to save %s", path);
            WaitPrompt(msg);
            f_close(&fp);
            return;
        } else { // Load
            u32 bytes_read = 0, bytes_read_total = 0;

            memopen();
            while(bytes_read_total < finfo.fsize) {
                if (f_read(&fp, &statebuffer[bytes_read_total], 0x200, &bytes_read) != FR_OK) {
                    WaitPrompt((char*)"f_read failed");
                    f_close(&fp);
                    return;
                }

                if (bytes_read == 0)
                    break;
                bytes_read_total += bytes_read;
            }

            if (bytes_read_total < finfo.fsize) {
                WaitPrompt((char*)"read failed");
                f_close(&fp);
                return;
            }
            sprintf(msg, "Read %d of %ld bytes.", bytes_read_total, finfo.fsize);
            ShowAction(msg);
            f_close(&fp);
            offset = bytes_read_total;
            GCFCEUSS_Load();
            return;
        }
#endif
    }
}

void ManageState(int mode, int slot, int device) {
    if (device == 0) {
        MC_ManageState(mode, slot);
    }
    else {
        SD_ManageState(mode, slot);
    }
}

/* u8 screenscaler
 * u8 currpal
 * u8 slimit
 * u8 timing
 * u8 mpads[6]
 * int FSDisable (int is u32, 4 bytes)
 * u8 PADCAL
 * u8 PADTUR
 * u8 ChosenSlot
 * u8 ChosenDevice
 * u8 UseSDCARD
 * u8 UseWiiSDCARD
 */
int SaveSettings(u8 *buffer) {
    int filesize = 0;
    buffer[filesize++] = screenscaler;
    buffer[filesize++] = currpal;
    buffer[filesize++] = slimit;
    buffer[filesize++] = timing;
    buffer[filesize++] = mpads[0];
    buffer[filesize++] = mpads[1];
    buffer[filesize++] = mpads[2];
    buffer[filesize++] = mpads[3];
    buffer[filesize++] = mpads[4];
    buffer[filesize++] = mpads[5];
    buffer[filesize++] = FSDisable;
    buffer[filesize++] = PADCAL;
    buffer[filesize++] = PADTUR;
    buffer[filesize++] = ChosenSlot;
    buffer[filesize++] = ChosenDevice;
    buffer[filesize++] = UseSDCARD;
    buffer[filesize++] = UseWiiSDCARD;
    return filesize+1;
}

int LoadSettings(u8 *buffer) {
    int filesize = 0;
    screenscaler = buffer[filesize++];
    currpal = buffer[filesize++];
    if (currpal == 0)
        FCEU_ResetPalette();
    else {
        u8 r, g, b, i;
        /*** Now setup this palette ***/
        for ( i = 0; i < 64; i++ ) {
            r = palettes[currpal-1].data[i] >> 16;
            g = ( palettes[currpal-1].data[i] & 0xff00 ) >> 8;
            b = ( palettes[currpal-1].data[i] & 0xff );
            FCEUD_SetPalette( i, r, g, b);
            FCEUD_SetPalette( i+64, r, g, b);
            FCEUD_SetPalette( i+128, r, g, b);
            FCEUD_SetPalette( i+192, r, g, b);
        }
    }
    slimit = buffer[filesize++];
    FCEUI_DisableSpriteLimitation(slimit);
    timing = buffer[filesize++];
    FCEUI_SetVidSystem(timing);
    mpads[0] = buffer[filesize++];
    mpads[1] = buffer[filesize++];
    mpads[2] = buffer[filesize++];
    mpads[3] = buffer[filesize++];
    mpads[4] = buffer[filesize++];
    mpads[5] = buffer[filesize++];
    FSDisable = buffer[filesize++];
    FCEUI_DisableFourScore(FSDisable);
    PADCAL = buffer[filesize++];
    PADTUR = buffer[filesize++];
    ChosenSlot = buffer[filesize++];
    ChosenDevice = buffer[filesize++];
    UseSDCARD = buffer[filesize++];
    UseWiiSDCARD = buffer[filesize++];
    return filesize+1;
}

/****************************************************************************
 * Save Settings to MemCard
 ****************************************************************************/
void MC_ManageSettings(int mode, int slot, int quiet) {
    char mcFilename[80];
    int CardError;
    card_dir CardDir;
    card_file CardFile;
    int SectorSize;
    int found = 0;
    int FileSize;
    int actualSize;
    int savedBytes=0;
    char msg[128];

    /*** Build the file name ***/
    strcpy(mcFilename, "FCEU-Settings.fcs");

    /*** Mount the Card ***/
    CARD_Init("FCEU", "00");

    /*** Try for memory card in slot A ***/
    CardError = MountTheCard();

    if ( CardError >= 0 ) {
        /*** Get card sector size ***/
        CardError = CARD_GetSectorSize(slot, &SectorSize);				

        switch ( mode ) {
            case 0 : {	/*** Save Game ***/
                /*** Look for this file ***/
                CardError = CARD_FindFirst(slot, &CardDir, true);

                found = 0;
                card_stat CardStatus;
                while ( CardError != CARD_ERROR_NOFILE ) {
                    CardError = CARD_FindNext(&CardDir);
                    if ( strcmp(CardDir.filename, mcFilename) == 0 )
                        found = 1;
                }

                /*** Determine number of bytes required ***/
                u8 buffer[SectorSize];
                memset(buffer, 0, SectorSize);
                actualSize = SaveSettings(buffer);
                savedBytes = actualSize;

                sprintf(msg, "Saving Settings to MC ...");
                ShowAction(msg);

                FileSize = ( actualSize / SectorSize ) * SectorSize;
                if ( (actualSize % SectorSize) || (FileSize == 0) )
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
                if ( CardError == 0 ) {
                    int sbo = 0;
                    while ( actualSize > 0 ) {
                        CardError = CARD_Write(&CardFile, &buffer[sbo], SectorSize, sbo );
                        actualSize -= SectorSize;
                        sbo += SectorSize;
                    }

                    CardError = CARD_Close(&CardFile);
                    strcpy(msg, "Saved settings successfully");
                    if (quiet) ShowAction(msg);
                    else WaitPrompt(msg);
                } 
                else {
                    strcpy(msg, "Save Settings Failed!");
                    if (quiet) ShowAction(msg);
                    else WaitPrompt(msg);
                }

                CARD_Unmount(CARDSLOT);
             } 
             break;	/*** End save ***/

            case 1: {	/*** Load state ***/
                /*** Look for this file ***/
                CardError = CARD_FindFirst(CARDSLOT, &CardDir, true);
                found = 0;

                while ( CardError != CARD_ERROR_NOFILE ) {
                    CardError = CARD_FindNext(&CardDir);
                    if ( strcmp(CardDir.filename, mcFilename) == 0 )
                        found = 1;
                }

                if ( found == 0 ) {
                    strcpy(msg, "No Settings Found");
                    if (quiet) ShowAction(msg);
                    else WaitPrompt(msg);
                    CARD_Unmount(CARDSLOT);
                    return;
                }

                u8 buffer[SectorSize];
                /*** Load the file into memory ***/
                CardError = CARD_Open(CARDSLOT, mcFilename, &CardFile);
                CardError = CARD_Read(&CardFile, &buffer, SectorSize, 0);

                CARD_Close(&CardFile);

                /*** Finally, do load ***/
                savedBytes = LoadSettings(buffer);
                sprintf(msg, "Loaded settings successfully!");
                ShowAction(msg);
                CARD_Unmount(CARDSLOT);
            } 
            break;	/*** End load ***/

            default: break;	
        }
    } else {
        strcpy(msg, "Cannot mount Memory Card!");
        if (quiet) ShowAction(msg);
        else WaitPrompt(msg);
    } 
}

void SD_ManageSettings(int mode, int slot, int quiet) {
    char path[1024];
    char msg[128];
    int filesize = 0;
    int len = 0;
    u8 buffer[128];

    if (slot < 2) {
        sd_file *handle;
        sprintf (path, "dev%d:\\%s\\%s\\Settings.fcs", ChosenSlot, FCEUDIR, SAVEDIR);

        if (mode == 0) ShowAction ("Saving Settings to SD...");
        else ShowAction ("Loading Settings from SD...");

        handle = SDCARD_OpenFile(path, (mode == 0) ? "wb" : "rb");

        if (handle == NULL) {        
            sprintf(msg, "Couldn't open %s", path);
            if (quiet) ShowAction(msg);
            else WaitPrompt(msg);
            return;
        }

        if (mode == 0) { // Save
            filesize = SaveSettings(buffer);

            len = SDCARD_WriteFile(handle, buffer, filesize);
            SDCARD_CloseFile(handle);

            if (len != filesize) {
                sprintf (msg, "Error writing %s", path);
                if (quiet) ShowAction(msg);
                else WaitPrompt(msg);
                return;			
            }

            sprintf(msg, "Saved settings successfully");
            if (quiet) ShowAction(msg);
            else WaitPrompt(msg);
        } else { // Load
            len = SDCARD_ReadFile(handle, buffer, 128);
            SDCARD_CloseFile (handle);

            /*** Finally, do load ***/
            filesize = LoadSettings(buffer);

            sprintf (msg, "Loaded settings successfully");
            ShowAction(msg);
            return;
        }
    } else { // WiiSD
#ifdef HW_RVL
        if (mode == 0) ShowAction ("Saving Settings to WiiSD...");
        else ShowAction ("Loading Settings from WiiSD...");

        sprintf(path, "/%s/%s/Settings.fcs", FCEUDIR, SAVEDIR);
        FIL fp;
        int res;

        /* Mount WiiSD with TinyFatFS*/
        if(f_mount(0, &frontfs) != FR_OK) {
            strcpy(msg, "f_mount failed");
            if (quiet) ShowAction(msg);
            else WaitPrompt(msg);
            return 0;
        }
        if (mode == 0)
            res = f_open(&fp, path, FA_CREATE_ALWAYS | FA_WRITE);
        else {
            if ((res=f_stat(path, &finfo)) != FR_OK) {
                if (res == FR_NO_FILE) {
                    sprintf(msg, "Unable to find %s.", path);
                }
                else {
                    sprintf(msg, "f_stat failed, error %d", res);
                }
                if (quiet) ShowAction(msg);
                else WaitPrompt(msg);
                return;
            }
            res = f_open(&fp, path, FA_READ);
        }

        if (res != FR_OK) {
            sprintf(msg, "Failed to open %s, error %d.", path, res);
            if (quiet) ShowAction(msg);
            else WaitPrompt(msg);
            return;
        }

        if (mode == 0) { // Save
            u32 written = 0;
            filesize = SaveSettings(buffer);

            if ((res = f_write(&fp, buffer, filesize, &written)) != FR_OK) {
                sprintf(msg, "f_write failed, error %d", res);
                if (quiet) ShowAction(msg);
                else WaitPrompt(msg);
                f_close(&fp);
                return;
            }
            sprintf(msg, "Wrote %d bytes.", filesize);
            ShowAction(msg);
            f_close(&fp);
            return;
        } else { // Load
            u32 bytes_read = 0;
            filesize = 0;

            if (f_read(&fp, &buffer, 128, &bytes_read) != FR_OK) {
                strcpy(msg, "f_read failed");
                if (quiet) ShowAction(msg);
                else WaitPrompt(msg);
                f_close(&fp);
                return;
            }

            /*** Finally, do load ***/
            filesize = LoadSettings(buffer);

            sprintf(msg, "Read %d bytes.", bytes_read);
            ShowAction(msg);
            f_close(&fp);
            return;
        }
#endif
    }
}

void ManageSettings(int mode, int slot, int device, int quiet) {
    if (device == 0) {
        MC_ManageSettings(mode, slot, quiet);
    }
    else {
        SD_ManageSettings(mode, slot, quiet);
    }
}
