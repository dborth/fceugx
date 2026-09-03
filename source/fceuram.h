/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceuram.h
 *
 * Memory Based Load/Save RAM Manager
 ****************************************************************************/

bool SaveRAM (char * filepath, bool silent);
bool SaveRAMAuto (bool silent);
bool LoadRAM (char * filepath, bool silent);
bool LoadRAMAuto (bool silent);
