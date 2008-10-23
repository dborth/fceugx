/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * preferences.c
 *
 * Preferences save/load preferences utilities
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include <mxml.h>

#include "menudraw.h"
#include "memcardop.h"
#include "fileop.h"
#include "smbop.h"
#include "filesel.h"
#include "fceuconfig.h"
#include "pad.h"

extern const unsigned short saveicon[1024];
extern unsigned char savebuffer[];
extern int currconfig[4];

// button map configurations
extern unsigned int gcpadmap[];
extern unsigned int wmpadmap[];
extern unsigned int ccpadmap[];
extern unsigned int ncpadmap[];

#define PREFS_FILE_NAME "FCEUGX.xml"

char prefscomment[2][32];

/****************************************************************************
 * Prepare Preferences Data
 *
 * This sets up the save buffer for saving.
 ****************************************************************************/
mxml_node_t *xml;
mxml_node_t *data;
mxml_node_t *section;
mxml_node_t *item;
mxml_node_t *elem;

char temp[200];

const char * toStr(int i)
{
	sprintf(temp, "%d", i);
	return temp;
}

void createXMLSection(const char * name, const char * description)
{
	section = mxmlNewElement(data, "section");
	mxmlElementSetAttr(section, "name", name);
	mxmlElementSetAttr(section, "description", description);
}

void createXMLSetting(const char * name, const char * description, const char * value)
{
	item = mxmlNewElement(section, "setting");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "value", value);
	mxmlElementSetAttr(item, "description", description);
}

void createXMLController(unsigned int controller[], const char * name, const char * description)
{
	item = mxmlNewElement(section, "controller");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "description", description);

	// create buttons
	int i;
	for(i=0; i < MAXJP; i++)
	{
		elem = mxmlNewElement(item, "button");
		mxmlElementSetAttr(elem, "number", toStr(i));
		mxmlElementSetAttr(elem, "assignment", toStr(controller[i]));
	}
}

const char * XMLSaveCallback(mxml_node_t *node, int where)
{
	const char *name;

	name = node->value.element.name;

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


int
preparePrefsData (int method)
{
	int offset = 0;
	memset (savebuffer, 0, SAVEBUFFERSIZE);

	// add save icon and comments for Memory Card saves
	if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
	{
		offset = sizeof (saveicon);

		// Copy in save icon
		memcpy (savebuffer, saveicon, offset);

		// And the comments
		sprintf (prefscomment[0], "%s Prefs", VERSIONSTR);
		sprintf (prefscomment[1], "Preferences");
		memcpy (savebuffer + offset, prefscomment, 64);
		offset += 64;
	}

	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "version",VERSIONSTR);

	createXMLSection("File", "File Settings");

	createXMLSetting("AutoLoad", "Auto Load", toStr(GCSettings.AutoLoad));
	createXMLSetting("AutoSave", "Auto Save", toStr(GCSettings.AutoSave));
	createXMLSetting("LoadMethod", "Load Method", toStr(GCSettings.LoadMethod));
	createXMLSetting("SaveMethod", "Save Method", toStr(GCSettings.SaveMethod));
	createXMLSetting("LoadFolder", "Load Folder", GCSettings.LoadFolder);
	createXMLSetting("SaveFolder", "Save Folder", GCSettings.SaveFolder);
	createXMLSetting("CheatFolder", "Cheats Folder", GCSettings.CheatFolder);
	createXMLSetting("VerifySaves", "Verify Memory Card Saves", toStr(GCSettings.VerifySaves));

	createXMLSection("Network", "Network Settings");

	createXMLSetting("smbip", "Share Computer IP", GCSettings.smbip);
	createXMLSetting("smbshare", "Share Name", GCSettings.smbshare);
	createXMLSetting("smbuser", "Share Username", GCSettings.smbuser);
	createXMLSetting("smbpwd", "Share Password", GCSettings.smbpwd);

	createXMLSection("Emulation", "Emulation Settings");

	createXMLSetting("currpal", "Palette", toStr(GCSettings.currpal));
	createXMLSetting("timing", "Timing", toStr(GCSettings.timing));
	createXMLSetting("slimit", "8 Sprite Limit", toStr(GCSettings.slimit));
	createXMLSetting("NGCZoom", "C-Stick Zoom", toStr(GCSettings.NGCZoom));
	createXMLSetting("render", "Video Filtering", toStr(GCSettings.render));
	createXMLSetting("widescreen", "Aspect Ratio Correction", toStr(GCSettings.widescreen));

	createXMLSection("Controller", "Controller Settings");

	createXMLSetting("FSDisable", "Four Score", toStr(GCSettings.FSDisable));
	createXMLSetting("zapper", "Zapper", toStr(GCSettings.zapper));
	createXMLSetting("crosshair", "Zapper Crosshair", toStr(GCSettings.crosshair));
	createXMLController(gcpadmap, "gcpadmap", "GameCube Pad");
	createXMLController(wmpadmap, "wmpadmap", "Wiimote");
	createXMLController(ccpadmap, "ccpadmap", "Classic Controller");
	createXMLController(ncpadmap, "ncpadmap", "Nunchuk");

	int datasize = mxmlSaveString(xml, (char *)savebuffer, SAVEBUFFERSIZE, XMLSaveCallback);

	mxmlDelete(xml);

	return datasize;
}


/****************************************************************************
 * Decode Preferences Data
 ****************************************************************************/
void loadXMLSettingStr(char * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
		sprintf(var, "%s", mxmlElementGetAttr(item, "value"));
}
void loadXMLSettingInt(int * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
		*var = atoi(mxmlElementGetAttr(item, "value"));
}
void loadXMLSettingBool(bool * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
		*var = atoi(mxmlElementGetAttr(item, "value"));
}

void loadXMLController(unsigned int controller[], const char * name)

{
	item = mxmlFindElement(xml, xml, "controller", "name", name, MXML_DESCEND);

	if(item)
	{
		// populate buttons
		int i;
		for(i=0; i < MAXJP; i++)
		{
			elem = mxmlFindElement(item, xml, "button", "number", toStr(i), MXML_DESCEND);
			if(elem)
				controller[i] = atoi(mxmlElementGetAttr(elem, "assignment"));
		}
	}
}

bool
decodePrefsData (int method)
{
	int offset = 0;

	// skip save icon and comments for Memory Card saves
	if(method == METHOD_MC_SLOTA || method == METHOD_MC_SLOTB)
	{
		offset = sizeof (saveicon);
		offset += 64; // sizeof prefscomment
	}

	xml = mxmlLoadString(NULL, (char *)savebuffer+offset, MXML_TEXT_CALLBACK);

	// check settings version
	// we don't do anything with the version #, but we'll store it anyway
	char * version;
	item = mxmlFindElement(xml, xml, "file", "version", NULL, MXML_DESCEND);
	if(item) // a version entry exists
		version = (char *)mxmlElementGetAttr(item, "version");
	else // version # not found, must be invalid
		return false;

	// this code assumes version in format X.X.X
	// XX.X.X, X.XX.X, or X.X.XX will NOT work
	char verMajor = version[13];
	char verMinor = version[15];
	char verPoint = version[17];

	if(verMajor == '2' && verPoint < '3') // less than version 2.0.3
		return false; // reset settings
	else if(verMajor > '2' || verMinor > '0' || verPoint > '6') // some future version
		return false; // reset settings

	// File Settings

	loadXMLSettingInt(&GCSettings.AutoLoad, "AutoLoad");
	loadXMLSettingInt(&GCSettings.AutoSave, "AutoSave");
	loadXMLSettingInt(&GCSettings.LoadMethod, "LoadMethod");
	loadXMLSettingInt(&GCSettings.SaveMethod, "SaveMethod");
	loadXMLSettingStr(GCSettings.LoadFolder, "LoadFolder");
	loadXMLSettingStr(GCSettings.SaveFolder, "SaveFolder");
	loadXMLSettingStr(GCSettings.CheatFolder, "CheatFolder");
	loadXMLSettingInt(&GCSettings.VerifySaves, "VerifySaves");

	// Network Settings

	loadXMLSettingStr(GCSettings.smbip, "smbip");
	loadXMLSettingStr(GCSettings.smbshare, "smbshare");
	loadXMLSettingStr(GCSettings.smbuser, "smbuser");
	loadXMLSettingStr(GCSettings.smbpwd, "smbpwd");

	// Emulation Settings

	loadXMLSettingInt(&GCSettings.currpal, "currpal");
	loadXMLSettingInt(&GCSettings.timing, "timing");
	loadXMLSettingInt(&GCSettings.FSDisable, "FSDisable");
	loadXMLSettingInt(&GCSettings.slimit, "slimit");
	loadXMLSettingInt(&GCSettings.NGCZoom, "NGCZoom");
	loadXMLSettingInt(&GCSettings.render, "render");
	loadXMLSettingInt(&GCSettings.widescreen, "widescreen");
	loadXMLSettingInt(&GCSettings.zapper, "zapper");
	loadXMLSettingInt(&GCSettings.crosshair, "crosshair");
	// Controller Settings

	loadXMLController(gcpadmap, "gcpadmap");
	loadXMLController(wmpadmap, "wmpadmap");
	loadXMLController(ccpadmap, "ccpadmap");
	loadXMLController(ncpadmap, "ncpadmap");

	mxmlDelete(xml);

	return true;
}

/****************************************************************************
 * Save Preferences
 ****************************************************************************/
bool
SavePrefs (int method, bool silent)
{
	// there's no point in saving SMB settings TO SMB, because then we'll have no way to load them the next time!
	// so instead we'll save using whatever other method is available (eg: SD)
	if(method == METHOD_AUTO || method == METHOD_SMB)
		method = autoSaveMethod();

	char filepath[1024];
	int datasize;
	int offset = 0;

	datasize = preparePrefsData (method);

	if (!silent)
		ShowAction ((char*) "Saving preferences...");

	if(method == METHOD_SD || method == METHOD_USB)
	{
		if(ChangeFATInterface(method, NOTSILENT))
		{
			sprintf (filepath, "%s/%s/%s", ROOTFATDIR, GCSettings.SaveFolder, PREFS_FILE_NAME);
			offset = SaveBufferToFAT (filepath, datasize, silent);
		}
	}
	else if(method == METHOD_SMB)
	{
		sprintf (filepath, "%s/%s", GCSettings.SaveFolder, PREFS_FILE_NAME);
		offset = SaveBufferToSMB (filepath, datasize, silent);
	}
	else if(method == METHOD_MC_SLOTA)
	{
		offset = SaveBufferToMC (savebuffer, CARD_SLOTA, (char *)PREFS_FILE_NAME, datasize, silent);
	}
	else if(method == METHOD_MC_SLOTB)
	{
		offset = SaveBufferToMC (savebuffer, CARD_SLOTB, (char *)PREFS_FILE_NAME, datasize, silent);
	}

	if (offset > 0)
	{
		if (!silent)
			WaitPrompt ((char *)"Preferences saved");
		return true;
	}
	return false;
}

/****************************************************************************
 * Load Preferences from specified method
 ****************************************************************************/

bool
LoadPrefsFromMethod (int method)
{
	bool retval = false;
	char filepath[1024];
	int offset = 0;

	if(method == METHOD_SD || method == METHOD_USB)
	{
		if(ChangeFATInterface(method, NOTSILENT))
		{
			sprintf (filepath, "%s/%s/%s", ROOTFATDIR, GCSettings.SaveFolder, PREFS_FILE_NAME);
			offset = LoadSaveBufferFromFAT (filepath, SILENT);
		}
	}
	else if(method == METHOD_SMB)
	{
		sprintf (filepath, "%s/%s", GCSettings.SaveFolder, PREFS_FILE_NAME);
		offset = LoadSaveBufferFromSMB (filepath, SILENT);
	}
	else if(method == METHOD_MC_SLOTA)
	{
		offset = LoadBufferFromMC (savebuffer, CARD_SLOTA, (char *)PREFS_FILE_NAME, SILENT);
	}
	else if(method == METHOD_MC_SLOTB)
	{
		offset = LoadBufferFromMC (savebuffer, CARD_SLOTB, (char *)PREFS_FILE_NAME, SILENT);
	}

	if (offset > 0)
		retval = decodePrefsData (method);

	return retval;
}

/****************************************************************************
 * Load Preferences
 * Checks sources consecutively until we find a preference file
 ****************************************************************************/

bool LoadPrefs()
{
	bool prefFound = false;
	if(ChangeFATInterface(METHOD_SD, SILENT))
		prefFound = LoadPrefsFromMethod(METHOD_SD);
	if(!prefFound && ChangeFATInterface(METHOD_USB, SILENT))
		prefFound = LoadPrefsFromMethod(METHOD_USB);
	if(!prefFound && TestCard(CARD_SLOTA, SILENT))
		prefFound = LoadPrefsFromMethod(METHOD_MC_SLOTA);
	if(!prefFound && TestCard(CARD_SLOTB, SILENT))
		prefFound = LoadPrefsFromMethod(METHOD_MC_SLOTB);
	if(!prefFound && ConnectShare (SILENT))
		prefFound = LoadPrefsFromMethod(METHOD_SMB);

	return prefFound;
}
