/****************************************************************************
 * FCE Ultra
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

#include <unistd.h>

#include "fceultra/driver.h"

#define APPNAME			"FCE Ultra GX"
#define APPVERSION		"3.4.6"
#define APPFOLDER 		"fceugx"
#define PREF_FILE_NAME	"settings.xml"

#define MAXPATHLEN 1024
#define NOTSILENT 0
#define SILENT 1

const char pathPrefix[9][8] =
{ "", "sd:/", "usb:/", "dvd:/", "smb:/", "carda:/", "cardb:/", "port2:/" };

enum 
{
	DEVICE_AUTO,
	DEVICE_SD,
	DEVICE_USB,
	DEVICE_DVD,
	DEVICE_SMB,
	DEVICE_SD_SLOTA,
	DEVICE_SD_SLOTB,
	DEVICE_SD_PORT2
};

enum 
{
	FILE_RAM,
	FILE_STATE,
	FILE_ROM,
	FILE_CHEAT
};

enum
{
	CTRL_PAD,
	CTRL_ZAPPER,
	CTRL_PAD2,
	CTRL_PAD4,
	CTRL_LENGTH
};

const char ctrlName[6][20] =
{ "NES Controller", "NES Zapper", "NES Controllers (2)", "NES Controllers (4)" };

enum 
{
	LANG_JAPANESE = 0,
	LANG_ENGLISH,
	LANG_GERMAN,
	LANG_FRENCH,
	LANG_SPANISH,
	LANG_ITALIAN,
	LANG_DUTCH,
	LANG_SIMP_CHINESE,
	LANG_TRAD_CHINESE,
	LANG_KOREAN,
	LANG_PORTUGUESE,
	LANG_BRAZILIAN_PORTUGUESE,
	LANG_CATALAN,
	LANG_TURKISH,
	LANG_LENGTH
};

struct SGCSettings
{
	int		AutoLoad;
    int		AutoSave;
    int		LoadMethod; // For ROMS: Auto, SD, DVD, USB, Network (SMB)
	int		SaveMethod; // For SRAM, Freeze, Prefs: Auto, SD, USB, SMB
	int		AppendAuto; // 0 - no, 1 - yes
	char	LoadFolder[MAXPATHLEN]; // Path to game files
	char	LastFileLoaded[MAXPATHLEN]; //Last file loaded filename
	char	SaveFolder[MAXPATHLEN]; // Path to save files
	char	CheatFolder[MAXPATHLEN]; // Path to cheat files
	char	ScreenshotsFolder[MAXPATHLEN]; // Path to screenshot files
	char	CoverFolder[MAXPATHLEN]; 	// Path to cover files
	char	ArtworkFolder[MAXPATHLEN]; 	// Path to artwork files
	int		AutoloadGame;
	
	char	smbip[80];
	char	smbuser[20];
	char	smbpwd[20];
	char	smbshare[20];

	float	zoomHor; // horizontal zoom amount
	float	zoomVert; // vertical zoom amount
	int		render;		// 0 - original, 1 - filtered, 2 - unfiltered
	int		videomode; // 0 - automatic, 1 - NTSC (480i), 2 - Progressive (480p), 3 - PAL (50Hz), 4 - PAL (60Hz)
	int		widescreen;
	int		hideoverscan; // 0 = off, 1 = vertical, 2 = horizontal, 3 = both
	int		gamegenie;
	int		currpal;
	int		timing;
	int		Controller;
	int		crosshair;
	int		spritelimit;
	int		xshift;		// video output shift
	int		yshift;
	int		WiimoteOrientation;
	int		ExitAction;
	int		MusicVolume;
	int		SFXVolume;
	int		Rumble;
	int 	language;
	int		PreviewImage;
};

char* ImageFolder();

void ExitApp();
void ShutdownWii();
bool SupportedIOS(u32 ios);
bool SaneIOS(u32 ios);
extern struct SGCSettings GCSettings;
extern int ScreenshotRequested;
extern int ConfigRequested;
extern int ShutdownRequested;
extern int ExitRequested;
extern char appPath[];
extern int frameskip;
extern int fskip;
extern int fskipc;
extern int turbomode;
extern bool romLoaded;
extern bool isWiiVC;
static inline bool IsWiiU(void)
{
	return ((*(vu16*)0xCD8005A0 == 0xCAFE) || isWiiVC);
}
static inline bool IsWiiUFastCPU(void)
{
	return ((*(vu16*)0xCD8005A0 == 0xCAFE) && ((*(vu32*)0xCD8005B0 & 0x20) == 0));
}
#endif
