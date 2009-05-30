/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * softdev July 2006
 * svpe & crunchy2 June 2007
 * Tantric 2008-2009
 *
 * dvd.h
 *
 * DVD loading routines
 ****************************************************************************/

#ifndef _DVD_H_
#define _DVD_H_

bool MountDVD(bool silent);
int ParseDVDdirectory();
void SetDVDdirectory(u64 dir, int length);
bool SwitchDVDFolder(char dir[]);

int LoadDVDFileOffset(unsigned char *buffer, int length);
int LoadDVDFile(char * buffer, char *filepath, int datasize, bool silent);
int dvd_safe_read (void *dst, unsigned int len, u64 offset);

void SetDVDDriveType();
#ifdef HW_DOL
void dvd_motor_off ();
#endif

#endif
