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
	GCSettings.timing = 0;
	GCSettings.FSDisable = 1;
	GCSettings.zapper = 0;
	GCSettings.slimit = 1;
	GCSettings.screenscaler = 2;

	GCSettings.LoadMethod = METHOD_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	GCSettings.SaveMethod = METHOD_AUTO; // Auto, SD, Memory Card Slot A, Memory Card Slot B, USB, Network (SMB)
	sprintf (GCSettings.LoadFolder,"fceugx/roms"); // Path to game files
	sprintf (GCSettings.SaveFolder,"fceugx/saves"); // Path to save files
	sprintf (GCSettings.CheatFolder,"fceugx/cheats"); // Path to cheat files
	GCSettings.AutoLoad = 0;
	GCSettings.AutoSave = 0;

	// default SMB settings
	strncpy (GCSettings.smbip, "192.168.0.1", 15); // IP Address of share server
	strncpy (GCSettings.smbuser, "Wiiuser", 19); // Your share user
	strncpy (GCSettings.smbpwd, "password", 19); // Your share user password
	strncpy (GCSettings.smbshare, "NES", 19); // Share name on server

	GCSettings.gcip[0] = 0;
	GCSettings.gwip[0] = 0;
	GCSettings.mask[0] = 0;
	GCSettings.smbsvid[0] = 0;
	GCSettings.smbgcid[0] = 0;
}
