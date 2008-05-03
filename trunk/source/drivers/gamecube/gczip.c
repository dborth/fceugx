/****************************************************************************
 * GC Zip Extension
 *
 * GC DVD Zip File Loader.
 *
 * The idea here is not to support every zip file on the planet!
 * The unzip routine will simply unzip the first file in the zip archive.
 *
 * For maximum compression, I'd recommend using 7Zip,
 *	7za a -tzip -mx=9 rom.zip rom.smc
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <sdcard.h>

#ifdef HW_RVL
#include "wiisd/tff.h"

extern VFATFS vfs;
extern FSDIRENTRY vfsfile;
extern int UseWiiSDCARD;
#endif
extern sd_file *filehandle;
extern int UseSDCARD;

extern void ShowAction( char *msg );
extern void WaitPrompt( char *msg );

extern unsigned char readbuffer[2048];
extern unsigned int dvd_read(void *dst, unsigned int len, u64 offset);

#define ZIPCHUNK 2048

/*** PKWare Zip Header ***/
#define PKZIPID 0x504b0304
typedef struct {
    unsigned int zipid __attribute__ ((__packed__));       // 0x04034b50
    unsigned short zipversion __attribute__ ((__packed__));
    unsigned short zipflags __attribute__ ((__packed__));
    unsigned short compressionMethod __attribute__ ((__packed__));
    unsigned short lastmodtime __attribute__ ((__packed__));
    unsigned short lastmoddate __attribute__ ((__packed__));
    unsigned int crc32 __attribute__ ((__packed__));
    unsigned int compressedSize __attribute__ ((__packed__));
    unsigned int uncompressedSize __attribute__ ((__packed__));
    unsigned short filenameLength __attribute__ ((__packed__));
    unsigned short extraDataLength __attribute__ ((__packed__));
} PKZIPHEADER;

static inline u32 FLIP32(u32 b)
{
    unsigned int c;

    c = ( b & 0xff000000 ) >> 24;
    c |= ( b & 0xff0000 ) >> 8;
    c |= ( b & 0xff00 ) << 8;
    c |= ( b & 0xff ) << 24;

    return c;
}

static inline u16 FLIP16(u16 b)
{
    u16 c;

    c = ( b & 0xff00 ) >> 8;
    c |= ( b &0xff ) << 8;

    return c;
}

/****************************************************************************
 * isZipFile
 *
 * This ONLY check the zipid, so any file which starts with the correct
 * 4 bytes will be treated as a zip file.
 *
 * It interrogate the first 4 bytes of the common readbuffer, so make sure
 * it is populated before calling.
 ****************************************************************************/

bool isZipFile()
{
    u32 check;

    memcpy(&check, &readbuffer, 4);
    return ( check == PKZIPID ) ? true : false;
}

/****************************************************************************
 * unzipDVDFile
 *
 * This loads the zip file in small 2k chunks, and decompresses to the 
 * output buffer.
 *
 * Unzip terminates on Z_END_STREAM.
 ***************************************************************************/
int unzipDVDFile( unsigned char *outbuffer, 
        unsigned int discoffset, unsigned int length)
{
    PKZIPHEADER pkzip;
    int zipoffset = 0;
    int zipchunk = 0;
    char out[ZIPCHUNK];
    z_stream zs;
    int res;
    int bufferoffset = 0;
    int have = 0;
    char debug[128];

    /*** Copy PKZip header to local, used as info ***/
    memcpy(&pkzip, &readbuffer, sizeof(PKZIPHEADER));

    sprintf(debug, "Unzipping %d bytes ... Wait", FLIP32(pkzip.uncompressedSize));
    ShowAction(debug);

    /*** Prepare the zip stream ***/
    memset(&zs, 0, sizeof(z_stream));
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = 0;
    zs.next_in = Z_NULL;
    res = inflateInit2(&zs, -MAX_WBITS);

    if ( res != Z_OK )
        return 0;
    /*** Set ZipChunk for first pass ***/
    zipoffset = ( sizeof(PKZIPHEADER) + FLIP16(pkzip.filenameLength) + FLIP16(pkzip.extraDataLength ));
    zipchunk = ZIPCHUNK - zipoffset;

    /*** No do it! ***/
    do
    {
        zs.avail_in = zipchunk;
        zs.next_in = (Bytef *)&readbuffer[zipoffset];

        /*** Now inflate until input buffer is exhausted ***/
        do
        {
            zs.avail_out = ZIPCHUNK;
            zs.next_out = (Bytef *)&out;

            res = inflate(&zs, Z_NO_FLUSH);

            if ( res == Z_MEM_ERROR ) {
                inflateEnd(&zs);
                return 0;
            }

            have = ZIPCHUNK - zs.avail_out;
            if ( have ) {
                /*** Copy to normal block buffer ***/
                memcpy(&outbuffer[bufferoffset], &out, have);
                bufferoffset += have;
            }

        } while ( zs.avail_out == 0 );

        /*** Readup the next 2k block ***/
        zipoffset = 0;
        zipchunk = ZIPCHUNK;
        discoffset += 2048;
        if ( UseSDCARD )
            SDCARD_ReadFile(filehandle, &readbuffer, 2048);
#ifdef HW_RVL
            else if (UseWiiSDCARD) VFAT_fread(&vfsfile, &readbuffer, 2048);
#endif
        else
            dvd_read(&readbuffer, 2048, discoffset);

    } while ( res != Z_STREAM_END );

    inflateEnd(&zs);

    if ( UseSDCARD )
        SDCARD_CloseFile(filehandle);
#ifdef HW_RVL
    else if (UseWiiSDCARD) VFAT_fclose(&vfsfile);
#endif

    if ( res == Z_STREAM_END ) {
        if ( FLIP32(pkzip.uncompressedSize == (u32)bufferoffset ) )
            return bufferoffset;
        else
            return FLIP32(pkzip.uncompressedSize);
    }

    return 0;
}

