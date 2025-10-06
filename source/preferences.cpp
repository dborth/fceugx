/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * preferences.cpp
 *
 * Preferences save/load preferences utilities
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ogcsys.h>
#include <mxml.h>

#include "fceugx.h"
#include "filelist.h"
#include "button_mapping.h"
#include "filebrowser.h"
#include "menu.h"
#include "fileop.h"
#include "gcvideo.h"
#include "pad.h"

struct SGCSettings GCSettings;

/****************************************************************************
 * Prepare Preferences Data
 *
 * This sets up the save buffer for saving.
 ***************************************************************************/
static mxml_node_t *xml = NULL;
static mxml_node_t *data = NULL;
static mxml_node_t *section = NULL;
static mxml_node_t *item = NULL;
static mxml_node_t *elem = NULL;

static char temp[200];

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

/****************************************************************************
 * Settings Configuration Table
 * Single source of truth for all preference settings
 ***************************************************************************/
enum SettingType {
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_STRING
};

struct SettingInfo {
	const char* name;
	const char* description;
	SettingType type;
	void* ptr;
	int maxSize;
	const char* section;
	const char* sectionDesc;
	bool platformSpecific;
};

static const SettingInfo settingsConfig[] = {
	// File Settings
	{"AutoLoad", "Auto Load", TYPE_INT, &GCSettings.AutoLoad, 0, "File", "File Settings", false},
	{"AutoSave", "Auto Save", TYPE_INT, &GCSettings.AutoSave, 0, "File", "File Settings", false},
	{"LoadMethod", "Load Method", TYPE_INT, &GCSettings.LoadMethod, 0, "File", "File Settings", false},
	{"SaveMethod", "Save Method", TYPE_INT, &GCSettings.SaveMethod, 0, "File", "File Settings", false},
	{"LoadFolder", "Load Folder", TYPE_STRING, GCSettings.LoadFolder, sizeof(GCSettings.LoadFolder), "File", "File Settings", false},
	{"LastFileLoaded", "Last File Loaded", TYPE_STRING, GCSettings.LastFileLoaded, sizeof(GCSettings.LastFileLoaded), "File", "File Settings", false},
	{"SaveFolder", "Save Folder", TYPE_STRING, GCSettings.SaveFolder, sizeof(GCSettings.SaveFolder), "File", "File Settings", false},
	{"AppendAuto", "Append Auto to .SAV Files", TYPE_INT, &GCSettings.AppendAuto, 0, "File", "File Settings", false},
	{"CheatFolder", "Cheats Folder", TYPE_STRING, GCSettings.CheatFolder, sizeof(GCSettings.CheatFolder), "File", "File Settings", false},
	{"gamegenie", "Game Genie", TYPE_INT, &GCSettings.gamegenie, 0, "File", "File Settings", false},
	{"ScreenshotsFolder", "Screenshots Folder", TYPE_STRING, GCSettings.ScreenshotsFolder, sizeof(GCSettings.ScreenshotsFolder), "File", "File Settings", false},
	{"CoverFolder", "Covers Folder", TYPE_STRING, GCSettings.CoverFolder, sizeof(GCSettings.CoverFolder), "File", "File Settings", false},
	{"ArtworkFolder", "Artwork Folder", TYPE_STRING, GCSettings.ArtworkFolder, sizeof(GCSettings.ArtworkFolder), "File", "File Settings", false},
	
	// Network Settings
	{"smbip", "Share Computer IP", TYPE_STRING, GCSettings.smbip, sizeof(GCSettings.smbip), "Network", "Network Settings", false},
	{"smbshare", "Share Name", TYPE_STRING, GCSettings.smbshare, sizeof(GCSettings.smbshare), "Network", "Network Settings", false},
	{"smbuser", "Share Username", TYPE_STRING, GCSettings.smbuser, sizeof(GCSettings.smbuser), "Network", "Network Settings", false},
	{"smbpwd", "Share Password", TYPE_STRING, GCSettings.smbpwd, sizeof(GCSettings.smbpwd), "Network", "Network Settings", false},
	
	// Video Settings
	{"videomode", "Video Mode", TYPE_INT, &GCSettings.videomode, 0, "Video", "Video Settings", false},
	{"currpal", "Palette", TYPE_INT, &GCSettings.currpal, 0, "Video", "Video Settings", false},
	{"timing", "Timing", TYPE_INT, &GCSettings.timing, 0, "Video", "Video Settings", false},
	{"spritelimit", "Sprite Limit", TYPE_INT, &GCSettings.spritelimit, 0, "Video", "Video Settings", false},
	{"zoomHor", "Horizontal Zoom Level", TYPE_FLOAT, &GCSettings.zoomHor, 0, "Video", "Video Settings", false},
	{"zoomVert", "Vertical Zoom Level", TYPE_FLOAT, &GCSettings.zoomVert, 0, "Video", "Video Settings", false},
	{"render", "Video Filtering", TYPE_INT, &GCSettings.render, 0, "Video", "Video Settings", false},
	{"widescreen", "Aspect Ratio Correction", TYPE_INT, &GCSettings.widescreen, 0, "Video", "Video Settings", false},
	{"hideoverscan", "Video Cropping", TYPE_INT, &GCSettings.hideoverscan, 0, "Video", "Video Settings", false},
	{"xshift", "Horizontal Video Shift", TYPE_INT, &GCSettings.xshift, 0, "Video", "Video Settings", false},
	{"yshift", "Vertical Video Shift", TYPE_INT, &GCSettings.yshift, 0, "Video", "Video Settings", false},
	{"TurboModeEnabled", "Turbo Mode Enabled", TYPE_INT, &GCSettings.TurboModeEnabled, 0, "Video", "Video Settings", false},
	{"TurboModeButton", "Turbo Mode Button", TYPE_INT, &GCSettings.TurboModeButton, 0, "Video", "Video Settings", false},
	{"GamepadMenuToggle", "Gamepad Menu Toggle", TYPE_INT, &GCSettings.GamepadMenuToggle, 0, "Video", "Video Settings", false},
	
	// Menu Settings
#ifdef HW_RVL
	{"WiimoteOrientation", "Wiimote Orientation", TYPE_INT, &GCSettings.WiimoteOrientation, 0, "Menu", "Menu Settings", true},
#endif
	{"ExitAction", "Exit Action", TYPE_INT, &GCSettings.ExitAction, 0, "Menu", "Menu Settings", false},
	{"MusicVolume", "Music Volume", TYPE_INT, &GCSettings.MusicVolume, 0, "Menu", "Menu Settings", false},
	{"SFXVolume", "Sound Effects Volume", TYPE_INT, &GCSettings.SFXVolume, 0, "Menu", "Menu Settings", false},
	{"Rumble", "Rumble", TYPE_INT, &GCSettings.Rumble, 0, "Menu", "Menu Settings", false},
	{"language", "Language", TYPE_INT, &GCSettings.language, 0, "Menu", "Menu Settings", false},
	{"PreviewImage", "Preview Image", TYPE_INT, &GCSettings.PreviewImage, 0, "Menu", "Menu Settings", false},
	{"HideRAMSaving", "Hide RAM Saving", TYPE_INT, &GCSettings.HideRAMSaving, 0, "Menu", "Menu Settings", false},
	
	// Controller Settings
	{"Controller", "Controller", TYPE_INT, &GCSettings.Controller, 0, "Controller", "Controller Settings", false},
	{"crosshair", "Zapper Crosshair", TYPE_INT, &GCSettings.crosshair, 0, "Controller", "Controller Settings", false}
};

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

static void createXMLController(u32 controller[], const char * name, const char * description)
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
	return (NULL);
}

static int
preparePrefsData ()
{
	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "app", APPNAME);
	mxmlElementSetAttr(data, "version", APPVERSION);

	const char* currentSection = NULL;
	const int numSettings = sizeof(settingsConfig) / sizeof(settingsConfig[0]);
	
	for (int i = 0; i < numSettings; i++) {
		const SettingInfo& setting = settingsConfig[i];
		
		// Create new section if needed
		if (!currentSection || strcmp(currentSection, setting.section) != 0) {
			createXMLSection(setting.section, setting.sectionDesc);
			currentSection = setting.section;
		}
		
		// Create setting based on type
		switch(setting.type) {
			case TYPE_INT:
				createXMLSetting(setting.name, setting.description, toStr(*(int*)setting.ptr));
				break;
			case TYPE_FLOAT:
				createXMLSetting(setting.name, setting.description, FtoStr(*(float*)setting.ptr));
				break;
			case TYPE_STRING:
				createXMLSetting(setting.name, setting.description, (char*)setting.ptr);
				break;
		}
	}

	createXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad", "NES Pad - GameCube Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote", "NES Pad - Wiimote");
	createXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic", "NES Pad - Classic Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc", "NES Pad - Wii U Pro Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc", "NES Pad - Wii U Gamepad");
	createXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk", "NES Pad - Nunchuk + Wiimote");
	createXMLController(btnmap[CTRL_ZAPPER][CTRLR_GCPAD], "btnmap_zapper_gcpad", "Zapper - GameCube Controller");
	createXMLController(btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE], "btnmap_zapper_wiimote", "Zapper - Wiimote");

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

static void loadXMLController(u32 controller[], const char * name)
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

/****************************************************************************
 * decodePrefsData
 *
 * Decodes preferences - parses XML and loads preferences into the variables
 ***************************************************************************/

static bool
decodePrefsData ()
{
	bool result = false;

	xml = mxmlLoadString(NULL, (char *)savebuffer, MXML_TEXT_CALLBACK);

	if(xml)
	{
		// check settings version
		item = mxmlFindElement(xml, xml, "file", "version", NULL, MXML_DESCEND);
		if(item) // a version entry exists
		{
			const char * version = mxmlElementGetAttr(item, "version");

			if(version && strlen(version) == 5)
			{
				// this code assumes version in format X.X.X
				// XX.X.X, X.XX.X, or X.X.XX will NOT work
				int verMajor = version[0] - '0';
				int verMinor = version[2] - '0';
				int verPoint = version[4] - '0';

				// check that the versioning is valid
				if(!(verMajor >= 3 && verMajor <= 9 &&
					verMinor >= 0 && verMinor <= 9 &&
					verPoint >= 0 && verPoint <= 9))
					result = false;
				else
					result = true;
			}
		}

		if(result)
		{
			const int numSettings = sizeof(settingsConfig) / sizeof(settingsConfig[0]);
			
			for (int i = 0; i < numSettings; i++) {
				const SettingInfo& setting = settingsConfig[i];
				
				switch(setting.type) {
					case TYPE_INT:
						loadXMLSetting((int*)setting.ptr, setting.name);
						break;
					case TYPE_FLOAT:
						loadXMLSetting((float*)setting.ptr, setting.name);
						break;
					case TYPE_STRING:
						loadXMLSetting((char*)setting.ptr, setting.name, setting.maxSize);
						break;
				}
			}

			loadXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk");
			loadXMLController(btnmap[CTRL_ZAPPER][CTRLR_GCPAD], "btnmap_zapper_gcpad");
			loadXMLController(btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE], "btnmap_zapper_wiimote");
		}
		mxmlDelete(xml);
	}
	return result;
}

/****************************************************************************
 * FixInvalidSettings
 *
 * Attempts to correct at least some invalid settings - the ones that
 * might cause crashes
 ***************************************************************************/
void FixInvalidSettings()
{
	if(GCSettings.LoadMethod > 8)
		GCSettings.LoadMethod = DEVICE_AUTO;
	if(GCSettings.SaveMethod > 8)
		GCSettings.SaveMethod = DEVICE_AUTO;
	if(!(GCSettings.zoomHor > 0.5 && GCSettings.zoomHor < 1.5))
		GCSettings.zoomHor = 1.0;
	if(!(GCSettings.zoomVert > 0.5 && GCSettings.zoomVert < 1.5))
		GCSettings.zoomVert = 1.0;
	if(!(GCSettings.xshift > -50 && GCSettings.xshift < 50))
		GCSettings.xshift = 0;
	if(!(GCSettings.yshift > -50 && GCSettings.yshift < 50))
		GCSettings.yshift = 0;
	if(!(GCSettings.MusicVolume >= 0 && GCSettings.MusicVolume <= 100))
		GCSettings.MusicVolume = 20;
	if(!(GCSettings.SFXVolume >= 0 && GCSettings.SFXVolume <= 100))
		GCSettings.SFXVolume = 40;
	if(GCSettings.language < 0 || GCSettings.language >= LANG_LENGTH)
		GCSettings.language = LANG_ENGLISH;
	if(GCSettings.Controller > CTRL_PAD4 || GCSettings.Controller < CTRL_ZAPPER)
		GCSettings.Controller = CTRL_PAD2;
	if(!(GCSettings.render >= 0 && GCSettings.render < 5))
		GCSettings.render = 4;
	if(GCSettings.timing < 0 || GCSettings.timing > 3)
		GCSettings.timing = 2;
	if(!(GCSettings.videomode >= 0 && GCSettings.videomode < 5))
		GCSettings.videomode = 0;
}

/****************************************************************************
 * DefaultSettings
 *
 * Sets all the defaults!
 ***************************************************************************/
void
DefaultSettings ()
{
	memset (&GCSettings, 0, sizeof (GCSettings));
	ResetControls(); // controller button mappings

	GCSettings.currpal = 1; // color palette
	GCSettings.timing = 2; // 0 - NTSC, 1 - PAL, 2 - Automatic
	GCSettings.videomode = 0; // automatic video mode detection
	GCSettings.Controller = CTRL_PAD2; // NES pad, Four Score, Zapper
	GCSettings.crosshair = 1; // show zapper crosshair
	GCSettings.spritelimit = 1; // enforce 8 sprite limit
	GCSettings.gamegenie = 0; // Off

	GCSettings.render = 3; // Filtered (sharp)
	GCSettings.hideoverscan = 3; // hide both

	GCSettings.widescreen = 0;

#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		GCSettings.widescreen = 1;
#endif

	GCSettings.zoomHor = 1.0; // horizontal zoom level
	GCSettings.zoomVert = 1.0; // vertical zoom level
	GCSettings.xshift = 0; // horizontal video shift
	GCSettings.yshift = 0; // vertical video shift

	GCSettings.WiimoteOrientation = 0;
	GCSettings.AutoloadGame = 0;
	GCSettings.ExitAction = 0; // Auto
	GCSettings.MusicVolume = 20;
	GCSettings.SFXVolume = 40;
	GCSettings.Rumble = 1; // Enabled
	GCSettings.PreviewImage = 0;
	GCSettings.HideRAMSaving = 0;
	
#ifdef HW_RVL
	GCSettings.language = CONF_GetLanguage();
	
	if(GCSettings.language == LANG_TRAD_CHINESE)
		GCSettings.language = LANG_SIMP_CHINESE;
#else
	GCSettings.language = LANG_ENGLISH;
#endif

	GCSettings.LoadMethod = DEVICE_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	GCSettings.SaveMethod = DEVICE_AUTO; // Auto, SD, USB, Network (SMB)
	sprintf (GCSettings.LoadFolder, "%s/roms", APPFOLDER); // Path to game files
	sprintf (GCSettings.SaveFolder, "%s/saves", APPFOLDER); // Path to save files
	sprintf (GCSettings.CheatFolder, "%s/cheats", APPFOLDER); // Path to cheat files
	sprintf (GCSettings.ScreenshotsFolder, "%s/screenshots", APPFOLDER); // Path to screenshots files
	sprintf (GCSettings.CoverFolder, "%s/covers", APPFOLDER); // Path to cover files
	sprintf (GCSettings.ArtworkFolder, "%s/artwork", APPFOLDER); // Path to artwork files
	GCSettings.AutoLoad = 1; // Auto Load RAM
	GCSettings.AutoSave = 1; // Auto Save RAM
	GCSettings.TurboModeEnabled = 1; // Enabled by default
	GCSettings.TurboModeButton = 0; // Default is Right Analog Stick (0)
	GCSettings.GamepadMenuToggle = 0; // 0 = All options (default), 1 = Home / C-Stick left, 2 = R+L+Start
}

/****************************************************************************
 * Save Preferences
 ***************************************************************************/
static char prefpath[MAXPATHLEN] = { 0 };

bool
SavePrefs (bool silent)
{
	char filepath[MAXPATHLEN];
	int datasize;
	int offset = 0;
	int device = 0;

	if(prefpath[0] != 0)
	{
		sprintf(filepath, "%s/%s", prefpath, PREF_FILE_NAME);
		FindDevice(filepath, &device);
	}
	else if(appPath[0] != 0)
	{
		sprintf(filepath, "%s/%s", appPath, PREF_FILE_NAME);
		strcpy(prefpath, appPath);
		FindDevice(filepath, &device);
	}
	else
	{
		device = autoSaveMethod(silent);

		if(device == 0)
			return false;
		
		sprintf(filepath, "%s%s", pathPrefix[device], APPFOLDER);
		
		DIR *dir = opendir(filepath);
		if (!dir)
		{
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/roms", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/saves", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
		}
		else
		{
			closedir(dir);
		}
		sprintf(filepath, "%s%s/%s", pathPrefix[device], APPFOLDER, PREF_FILE_NAME);
		sprintf(prefpath, "%s%s", pathPrefix[device], APPFOLDER);
	}

	if(device == 0)
		return false;

	if (!silent)
		ShowAction ("Saving preferences...");

	FixInvalidSettings();

	AllocSaveBuffer ();
	datasize = preparePrefsData ();
	offset = SaveFile(filepath, datasize, silent);

	FreeSaveBuffer ();

	CancelAction();

	if (offset > 0)
	{
		if (!silent)
			InfoPrompt("Preferences saved");
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
static bool prefLoaded = false;

bool LoadPrefs()
{
	if(prefLoaded) // already attempted loading
		return true;

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

	for(int i=0; i<numDevices; i++)
	{
		prefFound = LoadPrefsFromMethod(filepath[i]);
		
		if(prefFound)
			break;
	}
#else
	if(ChangeInterface(DEVICE_SD_SLOTA, SILENT)) {
		sprintf(filepath[0], "carda:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
	else if(ChangeInterface(DEVICE_SD_SLOTB, SILENT)) {
		sprintf(filepath[0], "cardb:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
	else if(ChangeInterface(DEVICE_SD_PORT2, SILENT)) {
		sprintf(filepath[0], "port2:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
	else if(ChangeInterface(DEVICE_SD_GCLOADER, SILENT)) {
		sprintf(filepath[0], "gcloader:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
#endif

	prefLoaded = true; // attempted to load preferences

	if(prefFound)
		FixInvalidSettings();

	// attempt to create directories if they don't exist
	if((GCSettings.LoadMethod == DEVICE_SD && ChangeInterface(DEVICE_SD, SILENT))
		|| (GCSettings.LoadMethod == DEVICE_USB && ChangeInterface(DEVICE_USB, SILENT)))  
	{
		char dirPath[MAXPATHLEN];
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ScreenshotsFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CoverFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ArtworkFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CheatFolder);
		CreateDirectory(dirPath);
	}

	if(GCSettings.videomode > 0) {
		ResetVideo_Menu();
	}

#ifdef HW_RVL
	bg_music = (u8 * )bg_music_ogg;
	bg_music_size = bg_music_ogg_size;
	LoadBgMusic();
#endif

	ChangeLanguage();
	return prefFound;
}
