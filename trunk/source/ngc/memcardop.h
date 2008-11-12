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

int VerifyMCFile (char *buf, int slot, char *filename, int datasize);
int LoadMCFile (char *buf, int slot, char *filename, bool silent);
int SaveMCFile (char *buf, int slot, char *filename, int datasize, bool silent);
int MountCard(int cslot, bool silent);
bool TestCard(int slot, bool silent);

#endif
