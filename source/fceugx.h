/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
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
#define APPVERSION		"4.0.1"
#define APPFOLDER 		"fceugx"
#define PREF_FILE_NAME	"settings.xml"

#define MAXPATHLEN 1024
#define NOTSILENT 0
#define SILENT 1

const char pathPrefix[10][11] =
{ "", "sd:/", "usb:/", "dvd:/", "smb:/", "carda:/", "cardb:/", "port2:/", "gcloader:/" };

enum 
{
	DEVICE_AUTO = 0,
	DEVICE_SD,
	DEVICE_USB,
	DEVICE_DVD,
	DEVICE_SMB,
	DEVICE_SD_SLOTA,
	DEVICE_SD_SLOTB,
	DEVICE_SD_PORT2,
	DEVICE_SD_GCLOADER,
	DEVICE_LENGTH
};

enum {
    SAVEFOLDER_SAVES = 0,
    SAVEFOLDER_CHEATS,
    SAVEFOLDER_LENGTH
};

enum {
    LOADFOLDER_ROMS = 0,
    LOADFOLDER_SCREENSHOTS,
    LOADFOLDER_COVERS,
    LOADFOLDER_ARTWORK,
    LOADFOLDER_LENGTH
};

typedef struct {
    int id;
    const char *name;
} FolderDef;

const FolderDef saveFolder[] = {
    { SAVEFOLDER_SAVES,  "saves" },
    { SAVEFOLDER_CHEATS, "cheats" }
};

const FolderDef loadFolder[] = {
    { LOADFOLDER_ROMS,        "roms" },
    { LOADFOLDER_SCREENSHOTS, "screenshots" },
    { LOADFOLDER_COVERS,      "covers" },
    { LOADFOLDER_ARTWORK,     "artwork" }
};

enum 
{
	FILE_RAM,
	FILE_STATE,
	FILE_ROM,
	FILE_CHEAT
};

enum {
	AUTOLOAD_OFF = 0,
	AUTOLOAD_RAM,
	AUTOLOAD_STATE
};

enum {
	AUTOSAVE_OFF = 0,
	AUTOSAVE_RAM,
	AUTOSAVE_STATE,
	AUTOSAVE_BOTH
};

enum {
	PREVIEWIMAGE_SCREENSHOT = 0,
	PREVIEWIMAGE_COVER,
	PREVIEWIMAGE_ARTWORK,
	PREVIEWIMAGE_LENGTH
};

enum {
	VIDEO_ASPECT_RATIO_CORRECTION_NONE = 0,
	VIDEO_ASPECT_RATIO_CORRECTION_16_9,
	VIDEO_ASPECT_RATIO_CORRECTION_LENGTH
};

enum {
	VIDEO_HW_SOFTEN_OFF = 0,
	VIDEO_HW_SOFTEN_AUTO,
	VIDEO_HW_SOFTEN_SHARP,
	VIDEO_HW_SOFTEN_SOFT,
	VIDEO_HW_SOFTEN_LENGTH
};

enum {
	VIDEOMODE_AUTO = 0,
	VIDEOMODE_NTSC,
	VIDEOMODE_PROGRESSIVE,
	VIDEOMODE_PAL,
	VIDEOMODE_PAL60,
	VIDEOMODE_ORIGINAL_240P,
	VIDEOMODE_LENGTH
};

enum {
	HIDEOVERSCAN_OFF = 0,
	HIDEOVERSCAN_VERTICAL,
	HIDEOVERSCAN_HORIZONTAL,
	HIDEOVERSCAN_BOTH,
	HIDEOVERSCAN_LENGTH
};

enum {
	GAMEPAD_MENU_TOGGLE_DEFAULT = 0,
	GAMEPAD_MENU_TOGGLE_HOME_RIGHTSTICK,
	GAMEPAD_MENU_TOGGLE_LRSTART_12PLUS,
	GAMEPAD_MENU_TOGGLE_LENGTH
};

enum {
	WIIMOTEORIENTATION_VERTICAL = 0,
	WIIMOTEORIENTATION_HORIZONTAL,
	WIIMOTEORIENTATION_LENGTH
};

enum {
	TIMING_NTSC = 0,
	TIMING_PAL,
	TIMING_AUTOMATIC,
	TIMING_DENDY,
	TIMING_LENGTH
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
	LANG_SWEDISH,
	LANG_LENGTH
};

enum {
	TURBO_BUTTON_RSTICK = 0,
	TURBO_BUTTON_A,
	TURBO_BUTTON_B,
	TURBO_BUTTON_X,
	TURBO_BUTTON_Y,
	TURBO_BUTTON_L,
	TURBO_BUTTON_R,
	TURBO_BUTTON_ZL,
	TURBO_BUTTON_ZR,
	TURBO_BUTTON_Z,
	TURBO_BUTTON_C,
	TURBO_BUTTON_1,
	TURBO_BUTTON_2,
	TURBO_BUTTON_PLUS,
	TURBO_BUTTON_MINUS,
};

struct SGCSettings
{
	int		AutoLoad;
    int		AutoSave;
    int		LoadMethod; // For ROMS: Auto, SD, DVD, USB, Network (SMB)
	int		SaveMethod; // For SRAM, Freeze, Prefs: Auto, SD, USB, SMB
	bool	AppendAuto;
	char	LoadFolder[MAXPATHLEN]; // Path to game files
	char	LastFileLoaded[MAXPATHLEN]; //Last file loaded filename
	char	SaveFolder[MAXPATHLEN]; // Path to save files
	char	CheatFolder[MAXPATHLEN]; // Path to cheat files
	char	ScreenshotsFolder[MAXPATHLEN]; // Path to screenshot files
	char	CoverFolder[MAXPATHLEN]; 	// Path to cover files
	char	ArtworkFolder[MAXPATHLEN]; 	// Path to artwork files
	bool	AutoloadGame;
	
	char	smbip[80];
	char	smbuser[20];
	char	smbpwd[20];
	char	smbshare[20];

	int		videoMode;
	int		videoAspectRatioCorrection;
	bool	videoBilinearFilter;
	int		videoHardwareSoften;
	bool	videoScanlines;
	int		videoUpscalingFilter;
	float	videoZoomHor;
	float	videoZoomVert;
	int		videoXshift;
	int		videoYshift;
	int		currpal;
	int		hideoverscan; // 0 = off, 1 = vertical, 2 = horizontal, 3 = both

	bool	crosshair;
	bool	spritelimit;
	bool	gamegenie;
	int		timing;
	int		Controller;
	int		WiimoteOrientation;
	int		ExitAction;
	int		MusicVolume;
	int		SFXVolume;
	bool	Rumble;
	int 	language;
	int		PreviewImage;
	bool	HideRAMSaving;
	bool	TurboModeEnabled;
	int		TurboModeButton;
	int		GamepadMenuToggle;
};

void ExitApp();
extern struct SGCSettings GCSettings;
extern bool MenuRequested;
extern char appPath[];
extern int frameskip;
extern int fskip;
extern int fskipc;
extern int turbomode;
extern bool romLoaded;

#endif
