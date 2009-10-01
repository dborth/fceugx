/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceuconfig.cpp
 *
 * Configuration parameters
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fceusupport.h"
#include "fceugx.h"
#include "videofilter.h"
#include "pad.h"

struct SGCSettings GCSettings;

/****************************************************************************
 * FixInvalidSettings
 *
 * Attempts to correct at least some invalid settings - the ones that
 * might cause crashes
 ***************************************************************************/
void FixInvalidSettings()
{
	if(!(GCSettings.ZoomLevel > 0.5 && GCSettings.ZoomLevel < 1.5))
		GCSettings.ZoomLevel = 1.0;
	if(!(GCSettings.xshift > -50 && GCSettings.xshift < 50))
		GCSettings.xshift = 0;
	if(!(GCSettings.yshift > -50 && GCSettings.yshift < 50))
		GCSettings.yshift = 0;
	if(!(GCSettings.MusicVolume >= 0 && GCSettings.MusicVolume <= 100))
		GCSettings.MusicVolume = 40;
	if(!(GCSettings.SFXVolume >= 0 && GCSettings.SFXVolume <= 100))
		GCSettings.SFXVolume = 40;
	if(GCSettings.Controller > CTRL_PAD4 || GCSettings.Controller < CTRL_ZAPPER)
		GCSettings.Controller = CTRL_PAD2;
	if(!(GCSettings.render >= 0 && GCSettings.render < 3))
		GCSettings.render = 2;
	if(GCSettings.timing != 0 && GCSettings.timing != 1)
		GCSettings.timing = 0;
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
	ResetControls(); // controller button mappings

	GCSettings.currpal = 0; // color palette
	GCSettings.timing = 0; // 0 - NTSC, 1 - PAL
	GCSettings.videomode = 0; // automatic video mode detection
	GCSettings.Controller = CTRL_PAD2; // NES pad, Four Score, Zapper
	GCSettings.crosshair = 1; // show zapper crosshair
	GCSettings.spritelimit = 1; // enforce 8 sprite limit
	GCSettings.currpal = 1;

	GCSettings.ZoomLevel = 1.0; // zoom amount
	GCSettings.render = 2; // Unfiltered
	GCSettings.widescreen = 0; // no aspect ratio correction
	GCSettings.hideoverscan = 2; // hide both horizontal and vertical
	GCSettings.FilterMethod = FILTER_NONE;	// no hq2x

	GCSettings.xshift = 0; // horizontal video shift
	GCSettings.yshift = 0; // vertical video shift

	GCSettings.WiimoteOrientation = 0;
	GCSettings.ExitAction = 0;
	GCSettings.MusicVolume = 40;
	GCSettings.SFXVolume = 40;
	GCSettings.Rumble = 1;

	GCSettings.LoadMethod = DEVICE_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	GCSettings.SaveMethod = DEVICE_AUTO; // Auto, SD, Memory Card Slot A, Memory Card Slot B, USB, Network (SMB)
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
}
