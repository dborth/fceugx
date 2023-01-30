/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * gcunzip.h
 *
 * Unzip routines
 ****************************************************************************/
#ifndef _GCUNZIP_H_
#define _GCUNZIP_H_

int IsZipFile (char *buffer);
char * GetFirstZipFilename();
size_t UnZipBuffer (unsigned char *outbuffer, size_t buffersize);
int SzParse(char * filepath);
size_t SzExtractFile(int i, unsigned char *buffer);
void SzClose();

#endif
