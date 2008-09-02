/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * memcardop.h
 *
 * Memory Card routines
 ****************************************************************************/

#ifndef _MEMCARDOP_H_
#define _MEMCARDOP_H_

int VerifyMCFile (unsigned char *buf, int slot, char *filename, int datasize);

int LoadBufferFromMC (unsigned char *buf, int slot, char *filename, bool silent);
int SaveBufferToMC (unsigned char *buf, int slot, char *filename, int datasize, bool silent);
int MountCard(int cslot, bool silent);
bool TestCard(int slot, bool silent);

#endif
