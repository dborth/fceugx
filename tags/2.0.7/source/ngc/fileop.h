/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fatop.h
 *
 * FAT File operations
 ****************************************************************************/

#ifndef _FATOP_H_
#define _FATOP_H_
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include <fat.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>

#define ROOTFATDIR "fat:/"

void UnmountAllFAT();
bool ChangeFATInterface(int method, bool silent);
int ParseFATdirectory(int method);
int LoadFATSzFile(char * filepath, unsigned char * rbuffer);
int SaveFATFile (char * sbuffer, char *filepath, int length, bool silent);
int LoadFATFile (char * sbuffer, char *filepath, int length, bool silent);

extern char currFATdir[MAXPATHLEN];
extern FILE * fatfile;

#endif
