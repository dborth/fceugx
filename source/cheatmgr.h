/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2009-2023
 *
 * cheatmgr.h
 *
 * Cheat handling
 ***************************************************************************/

void SetupCheats();
bool FindGameGenie();
void OpenGameGenie();
void ResetGameGenie();
const char *GameGenieError();

extern int numcheats;
