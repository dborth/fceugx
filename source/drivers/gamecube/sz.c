/****************************************************************************
 * SZ.C
 * svpe June 2007
 *
 * This file manages the 7zip support for this emulator.
 * Currently it only provides functions for loading a 7zip file from a DVD.
 ****************************************************************************/
#ifdef HW_DOL // only do 7zip in Gamecube mode for now...

#include "sz.h"

extern u8 UseSDCARD;
extern u8 UseWiiSDCARD;
extern sd_file *filehandle;

// 7zip error list
char szerrormsg[][30] = {"7z: Data error",
    "7z: Out of memory", 
    "7z: CRC Error", 
    "7z: Not implemented", 
    "7z: Fail", 
    "7z: Archive error"};


SZ_RESULT SzRes;

SzFileInStream SzArchiveStream;
CArchiveDatabaseEx SzDb;
ISzAlloc SzAllocImp;
ISzAlloc SzAllocTempImp;
UInt32 SzBlockIndex = 0xFFFFFFFF;
size_t SzBufferSize;
size_t SzOffset;
size_t SzOutSizeProcessed;
CFileItem *SzF;
char sz_buffer[2048];

// needed because there are no header files -.-
#include <sdcard.h>
#define MAXFILES 1000
#define MAXJOLIET 256

extern FILEENTRIES filelist[MAXFILES];

extern int selection;
extern int maxfiles;
extern int offset;

// the GC's dvd drive only supports offsets and length which are a multiply of 32 bytes
// additionally the max length of a read is 2048 bytes
// this function removes these limitations
// additionally the 7zip SDK does often read data in 1 byte parts from the DVD even when
// it could read 32 bytes. the dvdsf_buffer has been added to avoid having to read the same sector
// over and over again
unsigned char dvdsf_buffer[DVD_SECTOR_SIZE];
u64 dvdsf_last_offset = 0;
u64 dvdsf_last_length = 0;

int dvd_buffered_read(void *dst, u32 len, u64 offset) {
    int ret = 0;

    // only read data if the data inside dvdsf_buffer cannot be used
    if(offset != dvdsf_last_offset || len > dvdsf_last_length) {
        char msg[1024];
        sprintf(msg, "buff_read: len=%d, offset=%llX, UseSD=%d", len, offset, UseSDCARD);
        //WaitPrompt(msg);
        memset(&dvdsf_buffer, '\0', DVD_SECTOR_SIZE);
        if (UseSDCARD) {
            if (filehandle == NULL)
                GetSDInfo();
            SDCARD_SeekFile(filehandle, offset, SDCARD_SEEK_SET);
            SDCARD_ReadFile(filehandle, &dvdsf_buffer, len);
        } else if (!UseWiiSDCARD)
            ret = dvd_read(&dvdsf_buffer, len, offset);
        dvdsf_last_offset = offset;
        dvdsf_last_length = len;
    }

    memcpy(dst, &dvdsf_buffer, len);
    return ret;
}

int dvd_safe_read(void *dst_v, u32 len, u64 offset) {
    unsigned char buffer[DVD_SECTOR_SIZE]; // buffer for one dvd sector

    // if read size and length are a multiply of DVD_(OFFSET,LENGTH)_MULTIPLY and length < DVD_MAX_READ_LENGTH
    // we don't need to fix anything
    if(len % DVD_LENGTH_MULTIPLY == 0 && offset % DVD_OFFSET_MULTIPLY == 0 && len <= DVD_MAX_READ_LENGTH) {
        char msg[1024];
        sprintf(msg, "simple_safe_read: len=%d, offset=%llX, UseSD=%d", len, offset, UseSDCARD);
        //WaitPrompt(msg);
        int ret = dvd_buffered_read(buffer, len, offset);
        memcpy(dst_v, &buffer, len);
        return ret;
    } else {
        char msg[1024];
        sprintf(msg, "complex_safe_read: len=%d, offset=%llX, UseSD=%d", len, offset, UseSDCARD);
        //WaitPrompt(msg);
        // no errors yet -> ret = 0
        // the return value of dvd_read will be OR'd with ret
        // because dvd_read does return 1 on error and 0 on success and
        // because 0 | 1 = 1 ret will also contain 1 if at least one error
        // occured and 0 otherwise ;)
        int ret = 0; // return value of dvd_read

        // we might need to fix all 3 issues
        unsigned char *dst = (unsigned char *)dst_v; // gcc will not allow to use var[num] on void* types
        u64 bytesToRead; // the number of bytes we still need to read & copy to the output buffer
        u64 currentOffset; // the current dvd offset 
        u64 bufferOffset; // the current buffer offset
        u64 i, j, k; // temporary variables which might be used for different stuff
        //	unsigned char buffer[DVD_SECTOR_SIZE]; // buffer for one dvd sector

        currentOffset = offset;
        bytesToRead = len;
        bufferOffset = 0;

        // fix first issue (offset is not a multiply of 32)
        if(offset % DVD_OFFSET_MULTIPLY) {
            // calcualte offset of the prior 32 byte position
            i = currentOffset - (currentOffset % DVD_OFFSET_MULTIPLY);

            // calculate the offset from which the data of the dvd buffer will be copied
            j = currentOffset % DVD_OFFSET_MULTIPLY;

            // calculate the number of bytes needed to reach the next DVD_OFFSET_MULTIPLY byte mark
            k = DVD_OFFSET_MULTIPLY - j;

            // maybe we'll only need to copy a few bytes and we therefore don't even reach the next sector
            if(k > len) {
                k = len;
            }

            // read 32 bytes from the last 32 byte position
            ret |= dvd_buffered_read(buffer, DVD_OFFSET_MULTIPLY, i);

            // copy the bytes to the output buffer and update currentOffset, bufferOffset and bytesToRead
            memcpy(&dst[bufferOffset], &buffer[j], k);
            currentOffset += k;
            bufferOffset += k;
            bytesToRead -= k;
        }

        // fix second issue (more than 2048 bytes are needed)
        if(bytesToRead > DVD_MAX_READ_LENGTH) {
            // calculate the number of 2048 bytes sector needed to get all data
            i = (bytesToRead - (bytesToRead % DVD_MAX_READ_LENGTH)) / DVD_MAX_READ_LENGTH;

            // read data in 2048 byte sector
            for(j = 0; j < i; j++) {
                ret |= dvd_buffered_read(buffer, DVD_MAX_READ_LENGTH, currentOffset); // read sector
                memcpy(&dst[bufferOffset], buffer, DVD_MAX_READ_LENGTH); // copy to output buffer

                // update currentOffset, bufferOffset and bytesToRead
                currentOffset += DVD_MAX_READ_LENGTH;
                bufferOffset += DVD_MAX_READ_LENGTH;
                bytesToRead -= DVD_MAX_READ_LENGTH;
            }
        }

        // fix third issue (length is not a multiply of 32)
        if(bytesToRead) {
            ret |= dvd_buffered_read(buffer, DVD_MAX_READ_LENGTH, currentOffset); // read 32 byte from the dvd
            memcpy(&dst[bufferOffset], buffer, bytesToRead); // copy bytes to output buffer
        }

        //free(tmp);
        return ret;
    }
}

// function used by the 7zip SDK to read data from the DVD (fread)
SZ_RESULT SzDvdFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
    // the void* object is a SzFileInStream
    SzFileInStream *s = (SzFileInStream *)object;

    // calculate dvd sector offset 
    u64 offset = (u64)(s->offset + s->pos);

    if(maxRequiredSize > 2048)
    {
        maxRequiredSize = 2048;
    }

    // read data
    dvd_safe_read(sz_buffer, maxRequiredSize, offset);
    *buffer = sz_buffer;
    *processedSize = maxRequiredSize;
    s->pos += *processedSize;

    return SZ_OK;
}

// function used by the 7zip SDK to change the filepointer (fseek(object, pos, SEEK_SET))
SZ_RESULT SzDvdFileSeekImp(void *object, CFileSize pos)
{
    // the void* object is a SzFileInStream
    SzFileInStream *s = (SzFileInStream *)object;

    // check if the 7z SDK wants to move the pointer to somewhere after the EOF
    if(pos >= s->len)
    {
        WaitPrompt("7z Error: The 7z SDK wants to start reading somewhere behind the EOF...");
        return SZE_FAIL;
    }

    // save new position and return
    s->pos = pos;
    return SZ_OK;
}

SZ_RESULT SzDvdIsArchive(u64 dvd_offset) {
    // 7z signautre
    static Byte Signature[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
    Byte Candidate[6];

    //  read the data from the DVD
    int res = dvd_safe_read (&Candidate, 6, dvd_offset);
    char msg[1024];
    sprintf(msg, "7zSig: %02X %02X %02X %02X %02X %02X",
            Candidate[0],
            Candidate[1],
            Candidate[2],
            Candidate[3],
            Candidate[4],
            Candidate[5]);
    WaitPrompt(msg);

    size_t i;
    for(i = 0; i < 6; i++) {
        if(Candidate[i] != Signature[i]) {
            return SZE_FAIL;
        }
    }

    return SZ_OK;
}

// display an error message
void SzDisplayError(SZ_RESULT res)
{
    WaitPrompt(szerrormsg[(res - 1)]);
}

static u64 rootdir;
static int rootdirlength;

void SzParse(void) {
    // save the offset and the length of this file inside the archive stream structure
    SzArchiveStream.offset = filelist[selection].offset;
    SzArchiveStream.len = filelist[selection].length;
    SzArchiveStream.pos = 0;

    // set handler functions for reading data from DVD and setting the position
    SzArchiveStream.InStream.Read = SzDvdFileReadImp;
    SzArchiveStream.InStream.Seek = SzDvdFileSeekImp;

    // set default 7Zip SDK handlers for allocation and freeing memory
    SzAllocImp.Alloc = SzAlloc;
    SzAllocImp.Free = SzFree;
    SzAllocTempImp.Alloc = SzAllocTemp;
    SzAllocTempImp.Free = SzFreeTemp;

    // prepare CRC and 7Zip database structures
    InitCrcTable();
    SzArDbExInit(&SzDb);

    // open the archive
    SzRes = SzArchiveOpen(&SzArchiveStream.InStream, &SzDb, &SzAllocImp, &SzAllocTempImp);

    if(SzRes != SZ_OK)
    {
        // free memory used by the 7z SDK
        SzArDbExFree(&SzDb, SzAllocImp.Free);
        return;
    }
    else
    {
        // archive opened successfully

        // erase all previous entries
        memset(&filelist, 0, sizeof(FILEENTRIES) * MAXFILES);

        // add '../' folder
        strncpy(filelist[0].filename, "../", 3);
        filelist[0].length = rootdirlength;  //  store rootdir in case the user wants to go one folder up
        filelist[0].offset = rootdir;        //   -''- rootdir length -''-     
        filelist[0].flags = 0;

        // get contents and parse them into the dvd file list structure
        unsigned int SzI, SzJ;
        SzJ = 1;
        for(SzI = 0; SzI < SzDb.Database.NumFiles; SzI++)
        {
            SzF = SzDb.Database.Files + SzI;

            // skip directories
            if(SzF->IsDirectory)
            {
                continue;
            }

            // do not exceed MAXFILES to avoid possible buffer overflows
            if(SzJ == (MAXFILES - 1))
            {
                break;
            }

            // parse information about this file to the dvd file list structure
            strncpy(filelist[SzJ].filename, SzF->Name, MAXJOLIET); // copy joliet name (useless...)
            filelist[SzJ].filename[MAXJOLIET] = 0; // terminate string
            filelist[SzJ].length = SzF->Size; // filesize
            filelist[SzJ].offset = SzI; // the extraction function identifies the file with this number
            filelist[SzJ].flags = 0; // only files will be displayed (-> no flags)
            SzJ++;
        }

        // update maxfiles and select the first entry
        maxfiles = SzJ;
        offset = selection = 0;
        return;
    }
}

void SzClose(void)
{
    SzArDbExFree(&SzDb, SzAllocImp.Free);
}

bool SzExtractROM(int i, unsigned char *buffer)
{

    // prepare some variables
    SzBlockIndex = 0xFFFFFFFF;
    SzOffset = 0;

    // Unzip the file
    ShowAction("Un7zipping file. Please wait...");
    WaitPrompt("Un7zipping file. Please wait...");
    SzRes = SzExtract2(
            &SzArchiveStream.InStream, 
            &SzDb,
            i,                                  /* index of file */
            &SzBlockIndex,                      /* index of solid block */
            &buffer,
            &SzBufferSize,
            &SzOffset,                          /* offset of stream for required file in *outBuffer */
            &SzOutSizeProcessed,                /* size of file in *outBuffer */
            &SzAllocImp,
            &SzAllocTempImp);

    // check for errors
    if(SzRes != SZ_OK)
    {				
        // display error message
        WaitPrompt(szerrormsg[(SzRes - 1)]);
        return false;
    }
    else
    {			
        // close 7Zip archive and free memory
        SzArDbExFree(&SzDb, SzAllocImp.Free);
        return true;
    }
}
#endif
