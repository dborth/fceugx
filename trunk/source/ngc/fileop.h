/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fatop.h
 *
 * File operations
 ****************************************************************************/

#ifndef _FILEOP_H_
#define _FILEOP_H_
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include <fat.h>
#include <unistd.h>

#define SAVEBUFFERSIZE (1024 * 512)

void InitDeviceThread();
void MountAllFAT();
void UnmountAllFAT();
bool ChangeInterface(int method, bool silent);
int ParseDirectory();
void AllocSaveBuffer();
void FreeSaveBuffer();
u32 LoadFileBuf(char * rbuffer, char *filepath, u32 length, int method, bool silent);
u32 LoadFile(char filepath[], int method, bool silent);
u32 LoadSzFile(char * filepath, unsigned char * rbuffer);
u32 SaveFileBuf(char * buffer, char *filepath, u32 datasize, int method, bool silent);
u32 SaveFile(char filepath[], u32 datasize, int method, bool silent);

extern unsigned char * savebuffer;
extern FILE * file;
extern bool unmountRequired[];
extern bool isMounted[];
extern lwp_t devicethread;

#endif
