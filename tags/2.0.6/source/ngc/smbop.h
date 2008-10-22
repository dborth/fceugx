/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * smbop.h
 *
 * SMB support routines
 ****************************************************************************/

#ifndef _SMBOP_H_
#define _SMBOP_H_

#include <smb.h>

bool InitializeNetwork(bool silent);
bool ConnectShare (bool silent);
char * SMBPath(char * path);
int UpdateSMBdirname();
int ParseSMBdirectory ();
SMBFILE OpenSMBFile(char * filepath);
int LoadSMBFile (char * fbuffer, int length);
int LoadSMBSzFile(char * filepath, unsigned char * rbuffer);
int LoadSaveBufferFromSMB (char *filepath, bool silent);
int LoadBufferFromSMB (char * sbuffer, char *filepath, int length, bool silent);
int SaveBufferToSMB (char *filepath, int datasize, bool silent);

extern SMBFILE smbfile;

#endif
