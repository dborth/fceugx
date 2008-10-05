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
int ParseDVDdirectory ();
int LoadDVDFile (unsigned char *buffer, int length);
bool TestDVD();
int dvd_read (void *dst, unsigned int len, u64 offset);
bool SwitchDVDFolder(char dir[]);

#endif
