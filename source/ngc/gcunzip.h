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

#ifndef _GCUNZIP_H_
#define _GCUNZIP_H_

#include <smb.h>

extern int IsZipFile (char *buffer);
char * GetFirstZipFilename(int method);
int UnZipFATFile (unsigned char *outbuffer, FILE* infile); // Reading from FAT
int UnZipDVDFile (unsigned char *outbuffer, u64 inoffset); // Reading from DVD
int UnZipSMBFile (unsigned char *outbuffer, SMBFILE infile); // Reading from SMB
int SzParse(char * filepath, int method);
int SzExtractFile(int i, unsigned char *buffer);
void SzClose();

/*
 * Zip file header definition
 */
typedef struct
{
  unsigned int zipid __attribute__ ((__packed__));	// 0x04034b50
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
}
PKZIPHEADER;

u32 FLIP32 (u32 b);
u16 FLIP16 (u16 b);

#endif
