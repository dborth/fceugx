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

bool ChangeFATInterface(int method, bool silent);
int ParseFATdirectory(int method);
int LoadFATFile ();
int SaveBufferToFAT (char *filepath, int datasize, bool silent);
int LoadSaveBufferFromFAT (char *filepath, bool silent);
int LoadBufferFromFAT (char * buffer, char *filepath, bool silent);

extern char currFATdir[MAXPATHLEN];

#endif
