/****************************************************************************
 * SZ.C
 * svpe June 2007
 *
 * This file manages the 7zip support for this emulator.
 * Currently it only provides functions for loading a 7zip file from a DVD.
 ****************************************************************************/
 
#include <gccore.h>
#include <ogcsys.h>
#include <gctypes.h>
#include <stdio.h>
#include <string.h>
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

extern SZ_RESULT SzRes;
						 
#define DVD_LENGTH_MULTIPLY 32
#define DVD_OFFSET_MULTIPLY 32
#define DVD_MAX_READ_LENGTH 2048
#define DVD_SECTOR_SIZE 2048

int dvd_buffered_read(void *dst, u32 len, u64 offset);
int dvd_safe_read(void *dst_v, u32 len, u64 offset);
SZ_RESULT SzDvdFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize);
SZ_RESULT SzDvdFileSeekImp(void *object, CFileSize pos);
SZ_RESULT SzDvdIsArchive(u64 dvd_offset);
void SzDisplayError(SZ_RESULT res);
void SzParse(void);
void SzClose(void);
bool SzExtractROM(int i, unsigned char *buffer);

// pseudo-header file part for some functions used in the gamecube port
extern unsigned int dvd_read(void *dst, unsigned int len, u64 offset);
extern void WaitPrompt( char *msg );
extern void ShowAction( char *msg );
