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

#ifndef _NGCSMB_
#define _NGCSMB_

void InitializeNetwork(bool silent);
bool ConnectShare (bool silent);
void CloseShare();

#endif
