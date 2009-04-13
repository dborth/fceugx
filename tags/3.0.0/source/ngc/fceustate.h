/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceustate.h
 *
 * Memory Based Load/Save State Manager
 ****************************************************************************/

bool SaveState (char * filepath, int method, bool silent);
bool SaveStateAuto (int method, bool silent);
bool LoadState (char * filepath, int method, bool silent);
bool LoadStateAuto (int method, bool silent);
