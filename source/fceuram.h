/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2022
 *
 * fceuram.h
 *
 * Memory Based Load/Save RAM Manager
 ****************************************************************************/

bool SaveRAM (char * filepath, bool silent);
bool SaveRAMAuto (bool silent);
bool LoadRAM (char * filepath, bool silent);
bool LoadRAMAuto (bool silent);
