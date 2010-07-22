/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceustate.h
 *
 * Memory Based Load/Save State Manager
 ****************************************************************************/

bool SaveState (char * filepath, bool silent);
bool SaveStateAuto (bool silent);
bool LoadState (char * filepath, bool silent);
bool LoadStateAuto (bool silent);
