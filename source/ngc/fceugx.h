/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceugx.h
 *
 * This file controls overall program flow. Most things start and end here!
 ****************************************************************************/

#ifndef _FCEUGX_H_
#define _FCEUGX_H_

#define VERSIONNUM "2.0.6"
#define VERSIONSTR "FCE Ultra GX 2.0.6"
#define PREF_FILE_NAME "FCEUGX.xml"

#define NOTSILENT 0
#define SILENT 1

enum {
	METHOD_AUTO,
	METHOD_SD,
	METHOD_USB,
	METHOD_DVD,
	METHOD_SMB,
	METHOD_MC_SLOTA,
	METHOD_MC_SLOTB
};

enum {
	FILE_ROM,
	FILE_RAM,
	FILE_STATE,
	FILE_FDSBIOS,
	FILE_CHEAT,
	FILE_PREF
};

struct SGCSettings{
	int		AutoLoad;
	int		AutoSave;
	int		LoadMethod; // For ROMS: Auto, SD, DVD, USB, Network (SMB)
	int		SaveMethod; // For SRAM, Freeze, Prefs: Auto, SD, Memory Card Slot A, Memory Card Slot B, USB, SMB
	char	LoadFolder[200]; // Path to game files
	char	SaveFolder[200]; // Path to save files
	char	CheatFolder[200]; // Path to cheat files
	char	gcip[16];
	char	gwip[16];
	char	mask[16];
	char	smbip[16];
	char	smbuser[20];
	char	smbpwd[20];
	char	smbgcid[20];
	char	smbsvid[20];
	char	smbshare[20];
	int		Zoom; // 0 - off, 1 - on
	float	ZoomLevel; // zoom amount
	int		VerifySaves;
	int		render;		// 0 - original, 1 - filtered, 2 - unfiltered
	int		widescreen;
	int		hideoverscan;
	int		currpal;
	int		timing;
	int		FSDisable;
	int		zapper;
	int		crosshair;
	int		slimit;
};

void ExitToLoader();
void Reboot();
void ShutdownWii();
extern struct SGCSettings GCSettings;
extern int ConfigRequested;
extern int ShutdownRequested;
extern int frameskip;

#endif
