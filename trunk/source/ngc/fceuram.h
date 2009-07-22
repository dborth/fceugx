/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceuram.h
 *
 * Memory Based Load/Save RAM Manager
 ****************************************************************************/

bool SaveRAM (char * filepath, int method, bool silent);
bool SaveRAMAuto (int method, bool silent);
bool LoadRAM (char * filepath, int method, bool silent);
bool LoadRAMAuto (int method, bool silent);
