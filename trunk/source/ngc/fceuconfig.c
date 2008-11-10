/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceuconfig.c
 *
 * Configuration parameters
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fceuconfig.h"

struct SGCSettings GCSettings;

void
DefaultSettings ()
{
	GCSettings.currpal = 0;
	GCSettings.timing = 0; // 0 - NTSC, 1 - PAL
	GCSettings.FSDisable = 1;
	GCSettings.zapper = 0;
	GCSettings.crosshair = 1;
	GCSettings.slimit = 1;

	GCSettings.Zoom = 0; // zooming default off
	GCSettings.ZoomLevel = 1.0; // zoom amount
	GCSettings.render = 2; // Unfiltered
	GCSettings.widescreen = 0; // no aspect ratio correction
	GCSettings.hideoverscan = 2; // hide both horizontal and vertical

	GCSettings.LoadMethod = METHOD_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	GCSettings.SaveMethod = METHOD_AUTO; // Auto, SD, Memory Card Slot A, Memory Card Slot B, USB, Network (SMB)
	sprintf (GCSettings.LoadFolder,"fceugx/roms"); // Path to game files
	sprintf (GCSettings.SaveFolder,"fceugx/saves"); // Path to save files
	sprintf (GCSettings.CheatFolder,"fceugx/cheats"); // Path to cheat files
	GCSettings.AutoLoad = 1; // Auto Load RAM
	GCSettings.AutoSave = 1; // Auto Save RAM

	// custom SMB settings
	strncpy (GCSettings.smbip, "", 15); // IP Address of share server
	strncpy (GCSettings.smbuser, "", 19); // Your share user
	strncpy (GCSettings.smbpwd, "", 19); // Your share user password
	strncpy (GCSettings.smbshare, "", 19); // Share name on server

	GCSettings.smbip[15] = 0;
	GCSettings.smbuser[19] = 0;
	GCSettings.smbpwd[19] = 0;
	GCSettings.smbshare[19] = 0;

	GCSettings.gcip[0] = 0;
	GCSettings.gwip[0] = 0;
	GCSettings.mask[0] = 0;
	GCSettings.smbsvid[0] = 0;
	GCSettings.smbgcid[0] = 0;
}
