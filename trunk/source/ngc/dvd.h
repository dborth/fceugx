/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * dvd.h
 *
 * DVD loading routines
 ****************************************************************************/

#ifndef _DVD_H_
#define _DVD_H_

int getpvd ();
bool MountDVD(bool silent);
int ParseDVDdirectory ();
int LoadDVDFileOffset(unsigned char *buffer, int length);
int LoadDVDFile(char * buffer, char *filepath, int datasize, bool silent);
int dvd_read (void *dst, unsigned int len, u64 offset);
int dvd_safe_read (void *dst, unsigned int len, u64 offset);
bool SwitchDVDFolder(char dir[]);
void SetDVDDriveType();
#ifdef HW_DOL
void dvd_motor_off ();
#endif

extern u64 dvddir;
extern int dvddirlength;

#endif
