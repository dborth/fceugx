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
#include <unistd.h>

void InitDeviceThread();
void MountAllFAT();
void UnmountAllFAT();
bool ChangeInterface(int method, bool silent);
int ParseDirectory();
u32 LoadFileBuf(char * rbuffer, char *filepath, u32 length, int method, bool silent);
u32 LoadFile(char filepath[], int method, bool silent);
u32 LoadSzFile(char * filepath, unsigned char * rbuffer);
u32 SaveFileBuf(char * buffer, char *filepath, u32 datasize, int method, bool silent);
u32 SaveFile(char filepath[], u32 datasize, int method, bool silent);

extern char currFATdir[MAXPATHLEN];
extern FILE * file;
extern bool unmountRequired[];
extern lwp_t devicethread;

#endif
