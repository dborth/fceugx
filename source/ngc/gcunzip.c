/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * gcunzip.h
 *
 * Unzip routines
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "dvd.h"
#include "menudraw.h"
#include "gcunzip.h"

/*
  * PKWare Zip Header - adopted into zip standard
  */
#define MAXROM 0x500000
#define ZIPCHUNK 2048

/*
 * Zip files are stored little endian
 * Support functions for short and int types
 */
u32
FLIP32 (u32 b)
{
	unsigned int c;

	c = (b & 0xff000000) >> 24;
	c |= (b & 0xff0000) >> 8;
	c |= (b & 0xff00) << 8;
	c |= (b & 0xff) << 24;

	return c;
}

u16
FLIP16 (u16 b)
{
	u16 c;

	c = (b & 0xff00) >> 8;
	c |= (b & 0xff) << 8;

	return c;
}

/****************************************************************************
 * IsZipFile
 *
 * Returns 1 when Zip signature is found
 * Returns 2 when 7z signature is found
 ****************************************************************************/
int
IsZipFile (char *buffer)
{
	unsigned int *check;
	check = (unsigned int *) buffer;

	if (check[0] == 0x504b0304) // ZIP file
		return 1;

	// 7z signature
	static Byte Signature[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};

	int i;
	for(i = 0; i < 6; i++)
		if(buffer[i] != Signature[i])
			return 0;

	return 2; // 7z archive found
}

 /*****************************************************************************
 * unzip
 *
 * It should be noted that there is a limit of 5MB total size for any ROM
 ******************************************************************************/
FILE* fatfile; // FAT
u64 discoffset; // DVD
SMBFILE smbfile; // SMB

int
UnZipBuffer (unsigned char *outbuffer, short where)
{
	PKZIPHEADER pkzip;
	int zipoffset = 0;
	int zipchunk = 0;
	char out[ZIPCHUNK];
	z_stream zs;
	int res;
	int bufferoffset = 0;
	int readoffset = 0;
	int have = 0;
	char readbuffer[ZIPCHUNK];
	char msg[128];

	/*** Read Zip Header ***/
	switch (where)
	{
		case 0:	// SD Card
		fseek(fatfile, 0, SEEK_SET);
		fread (readbuffer, 1, ZIPCHUNK, fatfile);
		break;

		case 1: // DVD
		dvd_read (readbuffer, ZIPCHUNK, discoffset);
		break;

		case 2: // From SMB
		SMB_ReadFile(readbuffer, ZIPCHUNK, 0, smbfile);
		break;
	}

	/*** Copy PKZip header to local, used as info ***/
	memcpy (&pkzip, readbuffer, sizeof (PKZIPHEADER));

	pkzip.uncompressedSize = FLIP32 (pkzip.uncompressedSize);

	sprintf (msg, "Unzipping %d bytes ... Wait",
	pkzip.uncompressedSize);
	ShowAction (msg);

	/*** Prepare the zip stream ***/
	memset (&zs, 0, sizeof (z_stream));
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
	zs.avail_in = 0;
	zs.next_in = Z_NULL;
	res = inflateInit2 (&zs, -MAX_WBITS);

	if (res != Z_OK)
		return 0;

	/*** Set ZipChunk for first pass ***/
	zipoffset =
	(sizeof (PKZIPHEADER) + FLIP16 (pkzip.filenameLength) +
	FLIP16 (pkzip.extraDataLength));
	zipchunk = ZIPCHUNK - zipoffset;

	/*** Now do it! ***/
	do
	{
		zs.avail_in = zipchunk;
		zs.next_in = (Bytef *) & readbuffer[zipoffset];

		/*** Now inflate until input buffer is exhausted ***/
		do
		{
			zs.avail_out = ZIPCHUNK;
			zs.next_out = (Bytef *) & out;

			res = inflate (&zs, Z_NO_FLUSH);

			if (res == Z_MEM_ERROR)
			{
				inflateEnd (&zs);
				return 0;
			}

			have = ZIPCHUNK - zs.avail_out;
			if (have)
			{
				/*** Copy to normal block buffer ***/
				memcpy (&outbuffer[bufferoffset], &out, have);
				bufferoffset += have;
			}
		}
		while (zs.avail_out == 0);

		/*** Readup the next 2k block ***/
		zipoffset = 0;
		zipchunk = ZIPCHUNK;

		switch (where)
		{
			case 0:	// SD Card
			fread (readbuffer, 1, ZIPCHUNK, fatfile);
			break;

			case 1:	// DVD
			readoffset += ZIPCHUNK;
			dvd_read (readbuffer, ZIPCHUNK, discoffset+readoffset);
			break;

			case 2: // From SMB
			readoffset += ZIPCHUNK;
			SMB_ReadFile(readbuffer, ZIPCHUNK, readoffset, smbfile);
			break;
		}
	}
	while (res != Z_STREAM_END);

	inflateEnd (&zs);

	if (res == Z_STREAM_END)
	{
		if (pkzip.uncompressedSize == (u32) bufferoffset)
			return bufferoffset;
		else
			return pkzip.uncompressedSize;
	}

	return 0;
}
// Reading from FAT
int
UnZipFATFile (unsigned char *outbuffer, FILE* infile)
{
	fatfile = infile;
	return UnZipBuffer(outbuffer, 0);
}
// Reading from DVD
int
UnZipDVDFile (unsigned char *outbuffer, u64 inoffset)
{
	discoffset = inoffset;
	return UnZipBuffer(outbuffer, 1);
}
// Reading from SMB
int
UnZipSMBFile (unsigned char *outbuffer, SMBFILE infile)
{
	smbfile = infile;
	return UnZipBuffer(outbuffer, 2);
}

/*
 * 7-zip functions are below. Have to be written to work with above.
 * 
else if (selection == 0 && inSz == true) {
	rootdir = filelist[1].offset;
	rootdirlength = filelist[1].length;
	offset = 0;
	maxfiles = parsedir();
	inSz = false;
	SzClose();
}
else if (inSz == false && SzDvdIsArchive(filelist[selection].offset) == SZ_OK) {
	// parse the 7zip file
	ShowAction("Found 7z");
	SzParse();
	if(SzRes == SZ_OK) {
		inSz = true;
		offset = selection = 0;
	} else {
		SzDisplayError(SzRes);
	}
}
else if (inSz == true) {
	// extract the selected ROM from the 7zip file to the buffer
	if(SzExtractROM(filelist[selection].offset, nesrom) == true) {
		haverom = 1;
		inSz = false;

		// go one directory up
		rootdir = filelist[1].offset;
		rootdirlength = filelist[1].length;
		offset = selection = 0;
		maxfiles = parsedir();
	}
}
*/




/*
 * 7-zip functions are below. Have to be written to work with above.


#include "7zCrc.h"
#include "7zIn.h"
#include "7zExtract.h"

typedef struct _SzFileInStream
{
    ISzInStream InStream;
    u64 offset; // offset of the file
    unsigned int len; // length of the file
    u64 pos;  // current position of the file pointer
} SzFileInStream;




 // 7zip error list
char szerrormsg[][30] = {
    "7z: Data error",
    "7z: Out of memory",
    "7z: CRC Error",
    "7z: Not implemented",
    "7z: Fail",
    "7z: Archive error"
};

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

            fseek(filehandle, offset, SEEK_SET);
            fread(&dvdsf_buffer, len, 1, filehandle);
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
    //ShowAction("Un7zipping file. Please wait...");
    WaitPrompt("Un7zipping file. Please wait...");
    SzRes = SzExtract2(
            &SzArchiveStream.InStream,
            &SzDb,
            i,                      // index of file
            &SzBlockIndex,          // index of solid block
            &buffer,
            &SzBufferSize,
            &SzOffset,              // offset of stream for required file in *outBuffer
            &SzOutSizeProcessed,    // size of file in *outBuffer
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
*/
