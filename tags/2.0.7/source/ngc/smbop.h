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
int LoadSMBSzFile(char * filepath, unsigned char * rbuffer);
int LoadSMBFile (char * sbuffer, char *filepath, int length, bool silent);
int SaveSMBFile (char * sbuffer, char *filepath, int length, bool silent);

extern SMBFILE smbfile;

#endif
