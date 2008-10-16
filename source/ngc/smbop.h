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

bool InitializeNetwork(bool silent);
bool ConnectShare (bool silent);
char * SMBPath(char * path);
int UpdateSMBdirname();
int ParseSMBdirectory ();
int LoadSMBFile (char *filename, int length);
int LoadSaveBufferFromSMB (char *filepath, bool silent);
int LoadBufferFromSMB (char * sbuffer, char *filepath, bool silent);
int SaveBufferToSMB (char *filepath, int datasize, bool silent);

#endif
