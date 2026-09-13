/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * preferences.cpp
 *
 * Preferences save/load preferences utilities
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mxml.h>
#if defined(HW_RVL) || defined(HW_DOL)
#include <ogc/conf.h>
#include <ogc/system.h>
#endif

#include "fceugx.h"
#include "filelist.h"
#include "button_mapping.h"
#include "filebrowser.h"
#include "menu.h"
#include "fileop.h"
#include "videosupport.h"
#include "pad.h"

#if defined(HW_RVL) || defined(HW_DOL)
#include "drivers/ogc/WiiPlatform.h"
#include "drivers/ogc/GameCubePlatform.h"
#include "drivers/ogc/videofilters.h"
#endif

struct SEmuSettings EmuSettings;

/****************************************************************************
 * Prepare Preferences Data
 *
 * This sets up the save buffer for saving.
 ***************************************************************************/
static mxml_node_t *xml = nullptr;
static mxml_node_t *data = nullptr;
static mxml_node_t *section = nullptr;
static mxml_node_t *item = nullptr;
static mxml_node_t *elem = nullptr;

static char temp[200];

static const char* BtoStr(bool b)
{
    return b ? "1" : "0";
}
static const char * toStr(int i)
{
	sprintf(temp, "%d", i);
	return temp;
}

static const char * FtoStr(float i)
{
	sprintf(temp, "%.2f", i);
	return temp;
}

static void createXMLSection(const char * name, const char * description)
{
	section = mxmlNewElement(data, "section");
	mxmlElementSetAttr(section, "name", name);
	mxmlElementSetAttr(section, "description", description);
}

static void createXMLSetting(const char * name, const char * description, const char * value)
{
	item = mxmlNewElement(section, "setting");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "value", value);
	mxmlElementSetAttr(item, "description", description);
}

static void createXMLController(uint32_t controller[], const char * name, const char * description)
{
	item = mxmlNewElement(section, "controller");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "description", description);

	// create buttons
	for(int i=0; i < MAXJP; i++)
	{
		elem = mxmlNewElement(item, "button");
		mxmlElementSetAttr(elem, "number", toStr(i));
		mxmlElementSetAttr(elem, "assignment", toStr(controller[i]));
	}
}

static const char * XMLSaveCallback(mxml_node_t *node, int where)
{
	const char *name;

	name = mxmlGetElement(node);

	if(where == MXML_WS_BEFORE_CLOSE)
	{
		if(!strcmp(name, "file") || !strcmp(name, "section"))
			return ("\n");
		else if(!strcmp(name, "controller"))
			return ("\n\t");
	}
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if(!strcmp(name, "file"))
			return ("\n");
		else if(!strcmp(name, "section"))
			return ("\n\n");
		else if(!strcmp(name, "setting") || !strcmp(name, "controller"))
			return ("\n\t");
		else if(!strcmp(name, "button"))
			return ("\n\t\t");
	}
	return (nullptr);
}

static int
preparePrefsData ()
{
	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "app", APPNAME);
	mxmlElementSetAttr(data, "version", APPVERSION);

	createXMLSection("File", "File EmuSettings");

	createXMLSetting("AutoLoad", "Auto Load", toStr(EmuSettings.AutoLoad));
	createXMLSetting("AutoSave", "Auto Save", toStr(EmuSettings.AutoSave));
	createXMLSetting("LoadMethod", "Load Method", toStr(EmuSettings.LoadMethod));
	createXMLSetting("SaveMethod", "Save Method", toStr(EmuSettings.SaveMethod));
	createXMLSetting("LoadFolder", "Load Folder", EmuSettings.LoadFolder);
	createXMLSetting("LastFileLoaded", "Last File Loaded", EmuSettings.LastFileLoaded);
	createXMLSetting("SaveFolder", "Save Folder", EmuSettings.SaveFolder);
	createXMLSetting("AppendAuto", "Append Auto to .SAV Files", BtoStr(EmuSettings.AppendAuto));
	createXMLSetting("CheatFolder", "Cheats Folder", EmuSettings.CheatFolder);
	createXMLSetting("gamegenie", "Game Genie", BtoStr(EmuSettings.gamegenie));
	createXMLSetting("ScreenshotsFolder", "Screenshots Folder", EmuSettings.ScreenshotsFolder);
	createXMLSetting("CoverFolder", "Covers Folder", EmuSettings.CoverFolder);
	createXMLSetting("ArtworkFolder", "Artwork Folder", EmuSettings.ArtworkFolder);

	createXMLSection("Network", "Network EmuSettings");

	createXMLSetting("smbip", "Share Computer IP", EmuSettings.smbip);
	createXMLSetting("smbshare", "Share Name", EmuSettings.smbshare);
	createXMLSetting("smbuser", "Share Username", EmuSettings.smbuser);
	createXMLSetting("smbpwd", "Share Password", EmuSettings.smbpwd);

	createXMLSection("Video", "Video EmuSettings");

	createXMLSetting("videoMode", "Output Mode", toStr(EmuSettings.videoMode));
	createXMLSetting("videoAspectRatioCorrection", "Aspect Ratio Correction", toStr(EmuSettings.videoAspectRatioCorrection));
	createXMLSetting("hideoverscan", "Cropping", toStr(EmuSettings.hideoverscan));
	createXMLSetting("currpal", "Palette", toStr(EmuSettings.currpal));
	createXMLSetting("videoBilinearFilter", "Bilinear Filtering", BtoStr(EmuSettings.videoBilinearFilter));
	createXMLSetting("videoHardwareSoften", "Hardware Soften", toStr(EmuSettings.videoHardwareSoften));
	createXMLSetting("videoScanlines", "Scanlines", BtoStr(EmuSettings.videoScanlines));
	createXMLSetting("videoUpscalingFilter", "Upscaling Filter Method", toStr(EmuSettings.videoUpscalingFilter));
	createXMLSetting("videoZoomHor", "Horizontal Zoom Level", FtoStr(EmuSettings.videoZoomHor));
	createXMLSetting("videoZoomVert", "Vertical Zoom Level", FtoStr(EmuSettings.videoZoomVert));
	createXMLSetting("videoXshift", "Horizontal Video Shift", toStr(EmuSettings.videoXshift));
	createXMLSetting("videoYshift", "Vertical Video Shift", toStr(EmuSettings.videoYshift));

	createXMLSection("Emulation", "Emulation EmuSettings");

	createXMLSetting("timing", "Timing", toStr(EmuSettings.timing));
	createXMLSetting("spritelimit", "Sprite Limit", BtoStr(EmuSettings.spritelimit));
	createXMLSetting("crosshair", "Zapper Crosshair", BtoStr(EmuSettings.crosshair));

	createXMLSection("Menu", "Menu EmuSettings");

#ifdef HW_RVL
	createXMLSetting("wiimoteOrientation", "Wiimote Orientation", toStr(EmuSettings.wiimoteOrientation));
#endif
	createXMLSetting("ExitAction", "Exit Action", toStr(EmuSettings.ExitAction));
	createXMLSetting("MusicVolume", "Music Volume", toStr(EmuSettings.MusicVolume));
	createXMLSetting("SFXVolume", "Sound Effects Volume", toStr(EmuSettings.SFXVolume));
	createXMLSetting("Rumble", "Rumble", BtoStr(EmuSettings.Rumble));
	createXMLSetting("language", "Language", toStr(EmuSettings.language));
	createXMLSetting("PreviewImage", "Preview Image", toStr(EmuSettings.PreviewImage));
	createXMLSetting("HideRAMSaving", "Hide RAM Saving", BtoStr(EmuSettings.HideRAMSaving));

	createXMLSection("Controller", "Controller EmuSettings");

	createXMLSetting("Controller", "Controller", toStr(EmuSettings.Controller));
	createXMLSetting("TurboModeEnabled", "Turbo Mode Enabled", BtoStr(EmuSettings.TurboModeEnabled));
	createXMLSetting("TurboModeButton", "Turbo Mode Button", toStr(EmuSettings.TurboModeButton));
	createXMLSetting("GamepadMenuToggle", "Gamepad Menu Toggle", toStr(EmuSettings.GamepadMenuToggle));

	createXMLController(btnmap[CTRL_PAD][INPUT_HW_GAMECUBE], "btnmapping_pad_gcpad", "NES Pad - GameCube Controller");
	createXMLController(btnmap[CTRL_PAD][INPUT_HW_WIIMOTE], "btnmapping_pad_wiimote", "NES Pad - Wiimote");
	createXMLController(btnmap[CTRL_PAD][INPUT_HW_CLASSIC], "btnmapping_pad_classic", "NES Pad - Classic Controller");
	createXMLController(btnmap[CTRL_PAD][INPUT_HW_WUPC], "btnmapping_pad_wupc", "NES Pad - Wii U Pro Controller");
	createXMLController(btnmap[CTRL_PAD][INPUT_HW_DRC], "btnmapping_pad_wiidrc", "NES Pad - Wii U Gamepad");
	createXMLController(btnmap[CTRL_PAD][INPUT_HW_NUNCHUK], "btnmapping_pad_nunchuk", "NES Pad - Nunchuk + Wiimote");
	createXMLController(btnmap[CTRL_ZAPPER][INPUT_HW_GAMECUBE], "btnmapping_zapper_gcpad", "Zapper - GameCube Controller");
	createXMLController(btnmap[CTRL_ZAPPER][INPUT_HW_WIIMOTE], "btnmapping_zapper_wiimote", "Zapper - Wiimote");

	int datasize = mxmlSaveString(xml, (char *)savebuffer, SAVEBUFFERSIZE, XMLSaveCallback);

	mxmlDelete(xml);

	return datasize;
}

/****************************************************************************
 * loadXMLSetting
 *
 * Load XML elements into variables for an individual variable
 ***************************************************************************/

static void loadXMLSetting(char * var, const char * name, int maxsize)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			snprintf(var, maxsize, "%s", tmp);
	}
}
static void loadXMLSetting(bool * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp) {
			if (strcmp(tmp, "1") == 0 || strcasecmp(tmp, "true") == 0)
				*var = true;
			else
				*var = false;
		}
	}
}
static void loadXMLSetting(int * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atoi(tmp);
	}
}
static void loadXMLSetting(float * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atof(tmp);
	}
}

/****************************************************************************
 * loadXMLController
 *
 * Load XML elements into variables for a controller mapping
 ***************************************************************************/

static void loadXMLController(uint32_t controller[], const char * name)
{
	item = mxmlFindElement(xml, xml, "controller", "name", name, MXML_DESCEND);

	if(item)
	{
		// populate buttons
		for(int i=0; i < MAXJP; i++)
		{
			elem = mxmlFindElement(item, xml, "button", "number", toStr(i), MXML_DESCEND);
			if(elem)
			{
				const char * tmp = mxmlElementGetAttr(elem, "assignment");
				if(tmp)
					controller[i] = atoi(tmp);
			}
		}
	}
}

void ApplyEmuSettings() {
	platform->getInput()->setWiimoteOrientation(EmuSettings.wiimoteOrientation);
	platform->getInput()->setRumbleEnabled(EmuSettings.Rumble);
	GuiSound::setDefaultVolume(SOUND::OGG, EmuSettings.MusicVolume);
	GuiSound::setDefaultVolume(SOUND::PCM, EmuSettings.SFXVolume);
	platform->getVideo()->startMenuVideo();
	ChangeLanguage();
}

/****************************************************************************
 * decodePrefsData
 *
 * Decodes preferences - parses XML and loads preferences into the variables
 ***************************************************************************/

static bool
decodePrefsData ()
{
	xml = mxmlLoadString(nullptr, (char *)savebuffer, MXML_TEXT_CALLBACK);

	if(!xml) {
		return false;
	}

	// File EmuSettings

	loadXMLSetting(&EmuSettings.AutoLoad, "AutoLoad");
	loadXMLSetting(&EmuSettings.AutoSave, "AutoSave");
	loadXMLSetting(&EmuSettings.LoadMethod, "LoadMethod");
	loadXMLSetting(&EmuSettings.SaveMethod, "SaveMethod");
	loadXMLSetting(EmuSettings.LoadFolder, "LoadFolder", sizeof(EmuSettings.LoadFolder));
	loadXMLSetting(EmuSettings.LastFileLoaded, "LastFileLoaded", sizeof(EmuSettings.LastFileLoaded));
	loadXMLSetting(EmuSettings.SaveFolder, "SaveFolder", sizeof(EmuSettings.SaveFolder));
	loadXMLSetting(&EmuSettings.AppendAuto, "AppendAuto");
	loadXMLSetting(EmuSettings.CheatFolder, "CheatFolder", sizeof(EmuSettings.CheatFolder));
	loadXMLSetting(&EmuSettings.gamegenie, "gamegenie");
	loadXMLSetting(EmuSettings.ScreenshotsFolder, "ScreenshotsFolder", sizeof(EmuSettings.ScreenshotsFolder));
	loadXMLSetting(EmuSettings.CoverFolder, "CoverFolder", sizeof(EmuSettings.CoverFolder));
	loadXMLSetting(EmuSettings.ArtworkFolder, "ArtworkFolder", sizeof(EmuSettings.ArtworkFolder));

	// Network EmuSettings

	loadXMLSetting(EmuSettings.smbip, "smbip", sizeof(EmuSettings.smbip));
	loadXMLSetting(EmuSettings.smbshare, "smbshare", sizeof(EmuSettings.smbshare));
	loadXMLSetting(EmuSettings.smbuser, "smbuser", sizeof(EmuSettings.smbuser));
	loadXMLSetting(EmuSettings.smbpwd, "smbpwd", sizeof(EmuSettings.smbpwd));

	// Video EmuSettings

	loadXMLSetting(&EmuSettings.videoMode, "videoMode");
	loadXMLSetting(&EmuSettings.videoAspectRatioCorrection, "videoAspectRatioCorrection");
	loadXMLSetting(&EmuSettings.hideoverscan, "hideoverscan");
	loadXMLSetting(&EmuSettings.currpal, "currpal");
	loadXMLSetting(&EmuSettings.videoBilinearFilter, "videoBilinearFilter");
	loadXMLSetting(&EmuSettings.videoHardwareSoften, "videoHardwareSoften");
	loadXMLSetting(&EmuSettings.videoUpscalingFilter, "videoUpscalingFilter");
	loadXMLSetting(&EmuSettings.videoScanlines, "videoScanlines");
	loadXMLSetting(&EmuSettings.videoZoomHor, "videoZoomHor");
	loadXMLSetting(&EmuSettings.videoZoomVert, "videoZoomVert");
	loadXMLSetting(&EmuSettings.videoXshift, "videoXshift");
	loadXMLSetting(&EmuSettings.videoYshift, "videoYshift");

	// Emulation EmuSettings

	loadXMLSetting(&EmuSettings.timing, "timing");
	loadXMLSetting(&EmuSettings.spritelimit, "spritelimit");

	// Menu EmuSettings

	loadXMLSetting(&EmuSettings.wiimoteOrientation, "WiimoteOrientation");
	loadXMLSetting(&EmuSettings.ExitAction, "ExitAction");
	loadXMLSetting(&EmuSettings.MusicVolume, "MusicVolume");
	loadXMLSetting(&EmuSettings.SFXVolume, "SFXVolume");
	loadXMLSetting(&EmuSettings.Rumble, "Rumble");
	loadXMLSetting(&EmuSettings.language, "language");
	loadXMLSetting(&EmuSettings.PreviewImage, "PreviewImage");
	loadXMLSetting(&EmuSettings.HideRAMSaving, "HideRAMSaving");

	// Controller EmuSettings

	loadXMLSetting(&EmuSettings.Controller, "Controller");
	loadXMLSetting(&EmuSettings.crosshair, "crosshair");
	loadXMLSetting(&EmuSettings.TurboModeEnabled, "TurboModeEnabled");
	loadXMLSetting(&EmuSettings.TurboModeButton, "TurboModeButton");
	loadXMLSetting(&EmuSettings.GamepadMenuToggle, "GamepadMenuToggle");

	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_GAMECUBE], "btnmapping_pad_gcpad");
	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_WIIMOTE], "btnmapping_pad_wiimote");
	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_CLASSIC], "btnmapping_pad_classic");
	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_WUPC], "btnmapping_pad_wupc");
	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_DRC], "btnmapping_pad_wiidrc");
	loadXMLController(btnmap[CTRL_PAD][INPUT_HW_NUNCHUK], "btnmapping_pad_nunchuk");
	loadXMLController(btnmap[CTRL_ZAPPER][INPUT_HW_GAMECUBE], "btnmapping_zapper_gcpad");
	loadXMLController(btnmap[CTRL_ZAPPER][INPUT_HW_WIIMOTE], "btnmapping_zapper_wiimote");

	mxmlDelete(xml);
	return true;
}

/****************************************************************************
 * FixInvalidEmuSettings
 *
 * Attempts to correct at least some invalid settings - the ones that
 * might cause crashes
 ***************************************************************************/
void FixInvalidEmuSettings()
{
	if(!isValidLoadDevice(EmuSettings.LoadMethod))
		EmuSettings.LoadMethod = DEVICE_AUTO;
	if(!isValidSaveDevice(EmuSettings.SaveMethod))
		EmuSettings.SaveMethod = DEVICE_AUTO;

	if(strlen(EmuSettings.smbshare) == 0 || strlen(EmuSettings.smbip) == 0) {
		if(EmuSettings.LoadMethod == DEVICE_SMB) {
			EmuSettings.LoadMethod = DEVICE_AUTO;
		}
		if(EmuSettings.SaveMethod == DEVICE_SMB) {
			EmuSettings.SaveMethod = DEVICE_AUTO;
		}
	}

	if(!(EmuSettings.videoZoomHor > 0.5 && EmuSettings.videoZoomHor < 1.5))
		EmuSettings.videoZoomHor = 1.0;
	if(!(EmuSettings.videoZoomVert > 0.5 && EmuSettings.videoZoomVert < 1.5))
		EmuSettings.videoZoomVert = 1.0;
	if(!(EmuSettings.videoXshift > -50 && EmuSettings.videoXshift < 50))
		EmuSettings.videoXshift = 0;
	if(!(EmuSettings.videoYshift > -50 && EmuSettings.videoYshift < 50))
		EmuSettings.videoYshift = 0;
	if(!(EmuSettings.MusicVolume >= 0 && EmuSettings.MusicVolume <= 100))
		EmuSettings.MusicVolume = 20;
	if(!(EmuSettings.SFXVolume >= 0 && EmuSettings.SFXVolume <= 100))
		EmuSettings.SFXVolume = 40;
	if(EmuSettings.language < 0 || EmuSettings.language >= LANG_LENGTH)
		EmuSettings.language = LANG_ENGLISH;
	if(EmuSettings.Controller > CTRL_PAD4 || EmuSettings.Controller < CTRL_ZAPPER)
		EmuSettings.Controller = CTRL_PAD2;
	if(!(EmuSettings.videoHardwareSoften >= VIDEO_HW_SOFTEN_OFF && EmuSettings.videoHardwareSoften < VIDEO_HW_SOFTEN_LENGTH))
		EmuSettings.videoHardwareSoften = VIDEO_HW_SOFTEN_AUTO;
	if(!(EmuSettings.videoAspectRatioCorrection >= VIDEO_ASPECT_RATIO_CORRECTION_NONE && EmuSettings.videoAspectRatioCorrection < VIDEO_ASPECT_RATIO_CORRECTION_LENGTH))
		EmuSettings.videoAspectRatioCorrection = VIDEO_ASPECT_RATIO_CORRECTION_NONE;
	if(!(EmuSettings.videoMode >= VIDEOMODE_AUTO && EmuSettings.videoMode < VIDEOMODE_LENGTH))
		EmuSettings.videoMode = VIDEOMODE_AUTO;
#if defined(HW_RVL) || defined(HW_DOL)
	if(!(EmuSettings.videoUpscalingFilter >= FILTER_NONE && EmuSettings.videoUpscalingFilter <= NUM_FILTERS))
		EmuSettings.videoUpscalingFilter = FILTER_NONE;
#endif
	if(EmuSettings.timing < TIMING_NTSC || EmuSettings.timing >= TIMING_LENGTH)
		EmuSettings.timing = TIMING_AUTOMATIC;
	if(!(EmuSettings.hideoverscan >= HIDEOVERSCAN_OFF && EmuSettings.hideoverscan < HIDEOVERSCAN_LENGTH))
		EmuSettings.hideoverscan = HIDEOVERSCAN_BOTH;
	if(!(EmuSettings.wiimoteOrientation >= WIIMOTE_ORIENTATION_AUTO && EmuSettings.wiimoteOrientation < WIIMOTE_ORIENTATION_LENGTH))
		EmuSettings.wiimoteOrientation = WIIMOTE_ORIENTATION_AUTO;
}

/****************************************************************************
 * DefaultEmuSettings
 *
 * Sets all the defaults!
 ***************************************************************************/
void DefaultEmuSettings()
{
	memset (&EmuSettings, 0, sizeof (EmuSettings));
	ResetControls(); // controller button mappings

	EmuSettings.videoMode = VIDEOMODE_AUTO;
	EmuSettings.hideoverscan = HIDEOVERSCAN_BOTH;
	EmuSettings.currpal = 1;
	EmuSettings.videoBilinearFilter = true;
	EmuSettings.videoHardwareSoften = VIDEO_HW_SOFTEN_SHARP;
	EmuSettings.videoScanlines = false;
#if defined(HW_RVL) || defined(HW_DOL)
	EmuSettings.videoUpscalingFilter = FILTER_NONE;
#else
	EmuSettings.videoUpscalingFilter = 0;
#endif

#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		EmuSettings.videoAspectRatioCorrection = VIDEO_ASPECT_RATIO_CORRECTION_16_9;
	else
		EmuSettings.videoAspectRatioCorrection = VIDEO_ASPECT_RATIO_CORRECTION_NONE;
#elif HW_DOL
	EmuSettings.videoAspectRatioCorrection = VIDEO_ASPECT_RATIO_CORRECTION_NONE;
#endif

	EmuSettings.timing = TIMING_AUTOMATIC;
	EmuSettings.Controller = CTRL_PAD2; // NES pad, Four Score, Zapper
	EmuSettings.crosshair = true; // show zapper crosshair
	EmuSettings.spritelimit = true; // enforce 8 sprite limit
	EmuSettings.gamegenie = false;

	EmuSettings.wiimoteOrientation = WIIMOTE_ORIENTATION_AUTO;
	EmuSettings.AutoloadGame = false;
#ifdef HW_RVL
	EmuSettings.ExitAction = EXITACTION_WII_AUTO;
#elif HW_DOL
	EmuSettings.ExitAction = EXITACTION_GC_RETURN_TO_LOADER;
#endif
	EmuSettings.MusicVolume = 20;
	EmuSettings.SFXVolume = 40;
	EmuSettings.Rumble = true;
	EmuSettings.PreviewImage = PREVIEWIMAGE_COVER;
	EmuSettings.HideRAMSaving = false;
	
#ifdef HW_RVL
	EmuSettings.language = CONF_GetLanguage();
	
	if(EmuSettings.language == LANG_TRAD_CHINESE)
		EmuSettings.language = LANG_SIMP_CHINESE;
#elif HW_DOL
	EmuSettings.language = SYS_GetLanguage() + LANG_ENGLISH;
#endif

	EmuSettings.LoadMethod = DEVICE_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	EmuSettings.SaveMethod = DEVICE_AUTO; // Auto, SD, USB, Network (SMB)
	sprintf (EmuSettings.LoadFolder, "%s/%s", APPFOLDER, loadFolder[LOADFOLDER_ROMS].name); // Path to game files
	sprintf (EmuSettings.SaveFolder, "%s/%s", APPFOLDER, saveFolder[SAVEFOLDER_SAVES].name); // Path to save files
	sprintf (EmuSettings.CheatFolder, "%s/%s", APPFOLDER, saveFolder[SAVEFOLDER_CHEATS].name); // Path to cheat files
	sprintf (EmuSettings.ScreenshotsFolder, "%s/%s", APPFOLDER, loadFolder[LOADFOLDER_SCREENSHOTS].name); // Path to screenshots files
	sprintf (EmuSettings.CoverFolder, "%s/%s", APPFOLDER, loadFolder[LOADFOLDER_COVERS].name); // Path to cover files
	sprintf (EmuSettings.ArtworkFolder, "%s/%s", APPFOLDER, loadFolder[LOADFOLDER_ARTWORK].name); // Path to artwork files
	EmuSettings.AutoLoad = AUTOLOAD_RAM;
	EmuSettings.AutoSave = AUTOSAVE_RAM;
	EmuSettings.TurboModeEnabled = true;
	EmuSettings.TurboModeButton = 0; // Default is Right Analog Stick (0)
	EmuSettings.GamepadMenuToggle = GAMEPAD_MENU_TOGGLE_DEFAULT;
}

/****************************************************************************
 * Save Preferences
 ***************************************************************************/
static char prefpath[MAXPATHLEN] = { 0 };

bool SavePrefs()
{
	char filepath[MAXPATHLEN];
	int datasize;
	int offset = 0;
	int device = DEVICE_AUTO;

	if(prefpath[0] != 0)
	{
		snprintf(filepath, sizeof(filepath), "%s/%s", prefpath, PREF_FILE_NAME);
		FindDevice(filepath, &device);
	}
	else if(appPath[0] != 0)
	{
		snprintf(filepath, sizeof(filepath), "%s/%s", appPath, PREF_FILE_NAME);
		strcpy(prefpath, appPath);
		FindDevice(filepath, &device);
	}
	else
	{
		autoSaveMethod();
		device = EmuSettings.SaveMethod;

		if(!ChangeInterface(device, true)) {
			return false;
		}

		platform->getFileSystem()->getPath(filepath, device, APPFOLDER);
		if(!CreateDirectory(filepath)) {
			return false;
		}

		platform->getFileSystem()->getPath(filepath, device, APPFOLDER, PREF_FILE_NAME);
		platform->getFileSystem()->getPath(prefpath, device, APPFOLDER);
	}

	if(device == DEVICE_AUTO)
		return false;

	FixInvalidEmuSettings();

	AllocSaveBuffer ();
	datasize = preparePrefsData ();
	offset = SaveFile(filepath, datasize, true);

	FreeSaveBuffer ();

	CancelAction();

	if (offset > 0)
	{
		if(appPath[0] == 0)
			strcpy(appPath, prefpath);
		return true;
	}
	return false;
}

/****************************************************************************
 * Load Preferences from specified filepath
 ***************************************************************************/
bool
LoadPrefsFromMethod (char * path)
{
	bool retval = false;
	int offset = 0;
	char filepath[MAXPATHLEN];
	sprintf(filepath, "%s/%s", path, PREF_FILE_NAME);

	AllocSaveBuffer ();

	offset = LoadFile(filepath, SILENT);

	if (offset > 0)
		retval = decodePrefsData ();

	FreeSaveBuffer ();

	if(retval)
	{
		strcpy(prefpath, path);

		if(appPath[0] == 0)
			strcpy(appPath, prefpath);
	}

	return retval;
}

/****************************************************************************
 * Load Preferences
 * Checks sources consecutively until we find a preference file
 ***************************************************************************/
static bool prefLoadAttempted = false;

bool LoadPrefs()
{
	if(prefLoadAttempted) // already attempted loading
		return true;

	prefLoadAttempted = true;

	bool prefFound = false;
	char filepath[5][MAXPATHLEN];
	int numDevices;

#ifdef HW_RVL
	numDevices = 5;
	sprintf(filepath[0], "%s", appPath);
	sprintf(filepath[1], "sd:/apps/%s", APPFOLDER);
	sprintf(filepath[2], "usb:/apps/%s", APPFOLDER);
	sprintf(filepath[3], "sd:/%s", APPFOLDER);
	sprintf(filepath[4], "usb:/%s", APPFOLDER);
#elif HW_DOL
	numDevices = 4;
	sprintf(filepath[0], "carda:/%s", APPFOLDER);
	sprintf(filepath[1], "cardb:/%s", APPFOLDER);
	sprintf(filepath[2], "port2:/%s", APPFOLDER);
	sprintf(filepath[3], "gcloader:/%s", APPFOLDER);
#endif

	for(int i=0; i<numDevices; i++) {
		prefFound = LoadPrefsFromMethod(filepath[i]);

		if(prefFound)
			break;
	}

	if(!prefFound) {
		return false;
	}

	FixInvalidEmuSettings();
	ApplyEmuSettings();

#ifdef HW_RVL
	bg_music = (uint8_t * )bg_music_ogg;
	bg_music_size = bg_music_ogg_size;
	LoadBgMusic();
#endif
	return true;
}

void CreatePathWithPrefix(int device, const char* folder) {
    char fullPath[MAXPATHLEN];
    MakeFilePathForFolderPath(fullPath, device, folder);
    CreateDirectory(fullPath);
}

void CreateMissingDirectories() {
    char defaultFolder[MAXPATHLEN];

    if (EmuSettings.SaveMethod > DEVICE_AUTO && ChangeInterface(EmuSettings.SaveMethod, NOTSILENT)) {
        const char* savePointers[] = { EmuSettings.SaveFolder, EmuSettings.CheatFolder };

        for (int i = 0; i < SAVEFOLDER_LENGTH; i++) {
            const char* currentPath = savePointers[i];

            if (strncmp(currentPath, APPFOLDER, strlen(APPFOLDER)) == 0) {
                CreatePathWithPrefix(EmuSettings.SaveMethod, APPFOLDER);
            }

            GetDefaultFolderPath(defaultFolder, saveFolder[i].name);
            if (strcmp(currentPath, defaultFolder) == 0) {
                CreatePathWithPrefix(EmuSettings.SaveMethod, currentPath);
            }
        }
    }

    if (EmuSettings.LoadMethod > DEVICE_AUTO && EmuSettings.LoadMethod != DEVICE_DVD && ChangeInterface(EmuSettings.LoadMethod, NOTSILENT)) {
        const char* loadPointers[] = {
            EmuSettings.LoadFolder,
            EmuSettings.ScreenshotsFolder,
            EmuSettings.CoverFolder,
            EmuSettings.ArtworkFolder
        };

        for (int i = 0; i < LOADFOLDER_LENGTH; i++) {
            const char* currentPath = loadPointers[i];

            if (strncmp(currentPath, APPFOLDER, strlen(APPFOLDER)) == 0) {
                CreatePathWithPrefix(EmuSettings.LoadMethod, APPFOLDER);
            }

            GetDefaultFolderPath(defaultFolder, loadFolder[i].name);
            if (strcmp(currentPath, defaultFolder) == 0) {
                CreatePathWithPrefix(EmuSettings.LoadMethod, currentPath);
            }
        }
    }
}
