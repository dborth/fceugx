/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * menu.c
 *
 * Main menu flow control
 ****************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#ifdef WII_DVD
#include <di/di.h>
#endif

#include "common.h"
#include "fceuconfig.h"
#include "pad.h"
#include "button_mapping.h"
#include "filesel.h"
#include "gcunzip.h"
#include "smbop.h"
#include "memcardop.h"
#include "fileop.h"
#include "dvd.h"
#include "menudraw.h"
#include "fceustate.h"
#include "gcvideo.h"
#include "preferences.h"
#include "fceuram.h"
#include "fceuload.h"

extern void ResetNES(void);
extern void PowerNES(void);

extern int menu;
extern bool romLoaded;

#define SOFTRESET_ADR ((volatile u32*)0xCC003024)

/****************************************************************************
 * Reboot / Exit
 ****************************************************************************/

#ifndef HW_RVL
#define PSOSDLOADID 0x7c6000a6
int *psoid = (int *) 0x80001800;
void (*PSOReload) () = (void (*)()) 0x80001800;
#endif

void Reboot()
{
#ifdef HW_RVL
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#else
#define SOFTRESET_ADR ((volatile u32*)0xCC003024)
    *SOFTRESET_ADR = 0x00000000;
#endif
}

void ExitToLoader()
{
	// Exit to Loader
	#ifdef HW_RVL
		#ifdef WII_DVD
		DI_Close();
		#endif
		exit(0);
	#else	// gamecube
		if (psoid[0] == PSOSDLOADID)
			PSOReload ();
	#endif
}

/****************************************************************************
 * Load Manager
 ****************************************************************************/

int
LoadManager ()
{
	int loadROM = OpenROM(GCSettings.LoadMethod);

	if ( loadROM == 1 ) // if ROM was loaded
	{
		// load the RAM
		if (GCSettings.AutoLoad == 1)
			LoadRAM(GCSettings.SaveMethod, SILENT);
		else if (GCSettings.AutoLoad == 2)
			LoadState(GCSettings.SaveMethod, SILENT);

		ResetNES();
	}
	return loadROM;
}

/****************************************************************************
 * Video Options Menu
 ****************************************************************************/
static int videomenuCount = 7;
static char videomenu[][50] = {

	"Video Rendering",
	"Video Scaling",
	"Palette",
	"Enable Zooming",
	"Timing",
	"8 Sprite Limit",

	"Back to Preferences Menu"
};

void
VideoOptions ()
{
	int ret = 0;
	int quit = 0;
	int oldmenu = menu;
	menu = 0;
	while (quit == 0)
	{
		// don't allow original render mode if progressive video mode detected
		if (GCSettings.render==0 && progressive)
			GCSettings.render++;

		if ( GCSettings.render == 0 )
			sprintf (videomenu[0], "Video Rendering Original");
		if ( GCSettings.render == 1 )
			sprintf (videomenu[0], "Video Rendering Filtered");
		if ( GCSettings.render == 2 )
			sprintf (videomenu[0], "Video Rendering Unfiltered");

		sprintf (videomenu[1], "Video Scaling %s",
			GCSettings.widescreen == true ? "16:9 Correction" : "Default");

		sprintf (videomenu[2], "Palette - %s",
			GCSettings.currpal ? palettes[GCSettings.currpal-1].name : "Default");

		sprintf (videomenu[3], "Enable Zooming %s",
			GCSettings.Zoom == true ? " ON" : "OFF");

		sprintf (videomenu[4], "Timing - %s",
			GCSettings.timing == true ? " PAL" : "NTSC");

		sprintf (videomenu[5], "8 Sprite Limit - %s",
			GCSettings.slimit == true ? " ON" : "OFF");

		ret = RunMenu (videomenu, videomenuCount, (char*)"Video Options", 20, -1);

		switch (ret)
		{
			case 0:
				GCSettings.render++;
				if (GCSettings.render > 2 )
					GCSettings.render = 0;
				// reset zoom
				zoom_reset ();
				break;

			case 1:
				GCSettings.widescreen ^= 1;
				break;

			case 2: // palette
				if ( ++GCSettings.currpal > MAXPAL )
					GCSettings.currpal = 0;
				break;

			case 3:
				GCSettings.Zoom ^= 1;
				break;

			case 4: // timing
				GCSettings.timing ^= 1;
				break;

			case 5: // 8 sprite limit
				GCSettings.slimit ^=1;
				FCEUI_DisableSpriteLimitation(GCSettings.slimit);
				break;

			case -1: // Button B
			case 6:
				quit = 1;
				break;

		}
	}
	menu = oldmenu;
}


/****************************************************************************
 * File Options Menu
 ****************************************************************************/
static int filemenuCount = 8;
static char filemenu[][50] = {

	"Load Method",
	"Load Folder",
	"Save Method",
	"Save Folder",

	"Auto Load",
	"Auto Save",
	"Verify MC Saves",

	"Back to Preferences Menu"
};

void
FileOptions()
{
	int ret = 0;
	int quit = 0;
	int oldmenu = menu;
	menu = 0;
	while (quit == 0)
	{
		// some load/save methods are not implemented - here's where we skip them
		// they need to be skipped in the order they were enumerated in snes9xGX.h

		// no USB ports on GameCube
		#ifndef HW_RVL
		if(GCSettings.LoadMethod == METHOD_USB)
			GCSettings.LoadMethod++;
		if(GCSettings.SaveMethod == METHOD_USB)
			GCSettings.SaveMethod++;
		#endif

		// saving to DVD is impossible
		if(GCSettings.SaveMethod == METHOD_DVD)
			GCSettings.SaveMethod++;

		// disable SMB in GC mode (stalls out)
		#ifndef HW_RVL
		if(GCSettings.LoadMethod == METHOD_SMB)
			GCSettings.LoadMethod++;
		if(GCSettings.SaveMethod == METHOD_SMB)
			GCSettings.SaveMethod++;
		#endif

		// disable MC saving in Wii mode - does not work for some reason!
		#ifdef HW_RVL
		if(GCSettings.SaveMethod == METHOD_MC_SLOTA)
			GCSettings.SaveMethod++;
		if(GCSettings.SaveMethod == METHOD_MC_SLOTB)
			GCSettings.SaveMethod++;
		filemenu[6][0] = '\0';
		#else
		sprintf (filemenu[6], "Verify MC Saves %s",
			GCSettings.VerifySaves == true ? " ON" : "OFF");
		#endif

		// correct load/save methods out of bounds
		if(GCSettings.LoadMethod > 4)
			GCSettings.LoadMethod = 0;
		if(GCSettings.SaveMethod > 6)
			GCSettings.SaveMethod = 0;

		if (GCSettings.LoadMethod == METHOD_AUTO) sprintf (filemenu[0],"Load Method AUTO");
		else if (GCSettings.LoadMethod == METHOD_SD) sprintf (filemenu[0],"Load Method SD");
		else if (GCSettings.LoadMethod == METHOD_USB) sprintf (filemenu[0],"Load Method USB");
		else if (GCSettings.LoadMethod == METHOD_DVD) sprintf (filemenu[0],"Load Method DVD");
		else if (GCSettings.LoadMethod == METHOD_SMB) sprintf (filemenu[0],"Load Method Network");

		sprintf (filemenu[1], "Load Folder %s",	GCSettings.LoadFolder);

		if (GCSettings.SaveMethod == METHOD_AUTO) sprintf (filemenu[2],"Save Method AUTO");
		else if (GCSettings.SaveMethod == METHOD_SD) sprintf (filemenu[2],"Save Method SD");
		else if (GCSettings.SaveMethod == METHOD_USB) sprintf (filemenu[2],"Save Method USB");
		else if (GCSettings.SaveMethod == METHOD_SMB) sprintf (filemenu[2],"Save Method Network");
		else if (GCSettings.SaveMethod == METHOD_MC_SLOTA) sprintf (filemenu[2],"Save Method MC Slot A");
		else if (GCSettings.SaveMethod == METHOD_MC_SLOTB) sprintf (filemenu[2],"Save Method MC Slot B");

		sprintf (filemenu[3], "Save Folder %s",	GCSettings.SaveFolder);

		// disable changing load/save directories for now
		filemenu[1][0] = '\0';
		filemenu[3][0] = '\0';

		if (GCSettings.AutoLoad == 0) sprintf (filemenu[4],"Auto Load OFF");
		else if (GCSettings.AutoLoad == 1) sprintf (filemenu[4],"Auto Load RAM");
		else if (GCSettings.AutoLoad == 2) sprintf (filemenu[4],"Auto Load STATE");

		if (GCSettings.AutoSave == 0) sprintf (filemenu[5],"Auto Save OFF");
		else if (GCSettings.AutoSave == 1) sprintf (filemenu[5],"Auto Save RAM");
		else if (GCSettings.AutoSave == 2) sprintf (filemenu[5],"Auto Save STATE");
		else if (GCSettings.AutoSave == 3) sprintf (filemenu[5],"Auto Save BOTH");

		ret = RunMenu (filemenu, filemenuCount, (char*)"Save/Load Options", 20, -1);

		switch (ret)
		{
			case 0:
				GCSettings.LoadMethod ++;
				break;

			case 1:
				break;

			case 2:
				GCSettings.SaveMethod ++;
				break;

			case 3:
				break;

			case 4:
				GCSettings.AutoLoad ++;
				if (GCSettings.AutoLoad > 2)
					GCSettings.AutoLoad = 0;
				break;

			case 5:
				GCSettings.AutoSave ++;
				if (GCSettings.AutoSave > 3)
					GCSettings.AutoSave = 0;
				break;

			case 6:
				GCSettings.VerifySaves ^= 1;
				break;

			case -1: // Button B
			case 7:
				quit = 1;
				break;

		}
	}
	menu = oldmenu;
}

/****************************************************************************
 * Game Options Menu
 ****************************************************************************/

int
GameMenu ()
{
	int gamemenuCount = 9;
	char gamemenu[][50] = {
	  "Return to Game",
	  "Reset Game",
	  "ROM Information",
	  "Load RAM", "Save RAM",
	  "Load State", "Save State",
	  "Reset Zoom",
	  "Back to Main Menu"
	};

	int ret, retval = 0;
	int quit = 0;
	int oldmenu = menu;
	menu = 0;

	while (quit == 0)
	{
		if(nesGameType == 4) // FDS game
		{
			// disable RAM saving/loading
			gamemenu[3][0] = '\0';
			gamemenu[4][0] = '\0';

			// disable ROM Information
			gamemenu[2][0] = '\0';
		}

		// disable RAM/STATE saving/loading if AUTO is on
		if (GCSettings.AutoLoad == 1) // Auto Load RAM
			gamemenu[3][0] = '\0';
		else if (GCSettings.AutoLoad == 2) // Auto Load STATE
			gamemenu[5][0] = '\0';

		if (GCSettings.AutoSave == 1) // Auto Save RAM
			gamemenu[4][0] = '\0';
		else if (GCSettings.AutoSave == 2) // Auto Save STATE
			gamemenu[6][0] = '\0';
		else if (GCSettings.AutoSave == 3) // Auto Save BOTH
		{
			gamemenu[4][0] = '\0';
			gamemenu[6][0] = '\0';
		}

		// disable Reset Zoom if Zooming is off
		if(!GCSettings.Zoom)
			gamemenu[7][0] = '\0';

		ret = RunMenu (gamemenu, gamemenuCount, (char*)"Game Menu", 20, -1);

		switch (ret)
		{
			case 0: // Return to Game
				quit = retval = 1;
				break;

			case 1: // Reset Game
				PowerNES();
				quit = retval = 1;
				break;

			case 2: // ROM Information
				RomInfo();
				WaitButtonA ();
				break;

			case 3: // Load RAM
				quit = retval = LoadRAM(GCSettings.SaveMethod, NOTSILENT);
				break;

			case 4: // Save RAM
				SaveRAM(GCSettings.SaveMethod, NOTSILENT);
				break;

			case 5: // Load State
				quit = retval = LoadState(GCSettings.SaveMethod, NOTSILENT);
				break;

			case 6: // Save State
				SaveState(GCSettings.SaveMethod, NOTSILENT);
				break;

			case 7:
				zoom_reset ();
				quit = retval = 1;
				break;

			case -1: // Button B
			case 8: // Return to previous menu
				retval = 0;
				quit = 1;
				break;
		}
	}

	menu = oldmenu;

	return retval;
}

/****************************************************************************
 * Controller Configuration
 *
 * Snes9x 1.50 uses a cmd system to work out which button has been pressed.
 * Here, I simply move the designated value to the gcpadmaps array, which saves
 * on updating the cmd sequences.
 ****************************************************************************/
u32
GetInput (u16 ctrlr_type)
{
	//u32 exp_type;
	u32 pressed;
	pressed=0;
	s8 gc_px = 0;

	while( PAD_ButtonsHeld(0)
#ifdef HW_RVL
	| WPAD_ButtonsHeld(0)
#endif
	) VIDEO_WaitVSync();	// button 'debounce'

	while (pressed == 0)
	{
		VIDEO_WaitVSync();
		// get input based on controller type
		if (ctrlr_type == CTRLR_GCPAD)
		{
			pressed = PAD_ButtonsHeld (0);
			gc_px = PAD_SubStickX (0);
		}
#ifdef HW_RVL
		else
		{
		//	if ( WPAD_Probe( 0, &exp_type) == 0)	// check wiimote and expansion status (first if wiimote is connected & no errors)
		//	{
				pressed = WPAD_ButtonsHeld (0);

		//		if (ctrlr_type != CTRLR_WIIMOTE && exp_type != ctrlr_type+1)	// if we need input from an expansion, and its not connected...
		//			pressed = 0;
		//	}
		}
#endif
		/*** check for exit sequence (c-stick left OR home button) ***/
		if ( (gc_px < -70) || (pressed & WPAD_BUTTON_HOME) || (pressed & WPAD_CLASSIC_BUTTON_HOME) )
			return 0;
	}	// end while
	while( pressed == (PAD_ButtonsHeld(0)
#ifdef HW_RVL
						| WPAD_ButtonsHeld(0)
#endif
						) ) VIDEO_WaitVSync();

	return pressed;
}	// end GetInput()

int cfg_text_count = 7;
char cfg_text[][50] = {
"Remapping          ",
"Press Any Button",
"on the",
"       ",	// identify controller
"                   ",
"Press C-Left or",
"Home to exit"
};

u32
GetButtonMap(u16 ctrlr_type, char* btn_name)
{
	u32 pressed, previous;
	char temp[50] = "";
	uint k;
	pressed = 0; previous = 1;

	switch (ctrlr_type) {
		case CTRLR_NUNCHUK:
			strncpy (cfg_text[3], (char*)"NUNCHUK", 7);
			break;
		case CTRLR_CLASSIC:
			strncpy (cfg_text[3], (char*)"CLASSIC", 7);
			break;
		case CTRLR_GCPAD:
			strncpy (cfg_text[3], (char*)"GC PAD", 7);
			break;
		case CTRLR_WIIMOTE:
			strncpy (cfg_text[3], (char*)"WIIMOTE", 7);
			break;
	};

	/*** note which button we are remapping ***/
	sprintf (temp, (char*)"Remapping ");
	for (k=0; k<9-strlen(btn_name); k++) strcat(temp, " "); // add whitespace padding to align text
	strncat (temp, btn_name, 9);		// nes button we are remapping
	strncpy (cfg_text[0], temp, 19);	// copy this all back to the text we wish to display

	DrawMenu(&cfg_text[0], NULL, cfg_text_count, 1, 20, -1);	// display text

//	while (previous != pressed && pressed == 0);	// get two consecutive button presses (which are the same)
//	{
//		previous = pressed;
//		VIDEO_WaitVSync();	// slow things down a bit so we don't overread the pads
		pressed = GetInput(ctrlr_type);
//	}
	return pressed;
}	// end getButtonMap()

int cfg_btns_count = 12;
char cfg_btns_menu[][50] = {
	"B           -         ",
	"A           -         ",
	"RAPID B     -         ",
	"RAPID A     -         ",
	"SELECT      -         ",
	"START       -         ",
	"UP          -         ",
	"DOWN        -         ",
	"LEFT        -         ",
	"RIGHT       -         ",
	"SPECIAL     -         ",
	"Return to previous"
};

extern unsigned int gcpadmap[];
extern unsigned int wmpadmap[];
extern unsigned int ccpadmap[];
extern unsigned int ncpadmap[];

void
ConfigureButtons (u16 ctrlr_type)
{
	int quit = 0;
	int ret = 0;
	int oldmenu = menu;
	menu = 0;
	char* menu_title = NULL;
	u32 pressed;

	unsigned int* currentpadmap = 0;
	char temp[50] = "";
	int i, j;
	uint k;

	/*** Update Menu Title (based on controller we're configuring) ***/
	switch (ctrlr_type) {
		case CTRLR_NUNCHUK:
			menu_title = (char*)"NES     -  NUNCHUK";
			currentpadmap = ncpadmap;
			break;
		case CTRLR_CLASSIC:
			menu_title = (char*)"NES     -  CLASSIC";
			currentpadmap = ccpadmap;
			break;
		case CTRLR_GCPAD:
			menu_title = (char*)"NES     -   GC PAD";
			currentpadmap = gcpadmap;
			break;
		case CTRLR_WIIMOTE:
			menu_title = (char*)"NES     -  WIIMOTE";
			currentpadmap = wmpadmap;
			break;
	};

	while (quit == 0)
	{
		/*** Update Menu with Current ButtonMap ***/
		for (i=0; i<MAXJP; i++) // nes pad has 8 buttons to config (plus VS coin insert)
		{
			// get current padmap button name to display
			for ( j=0;
					j < ctrlr_def[ctrlr_type].num_btns &&
					currentpadmap[i] != ctrlr_def[ctrlr_type].map[j].btn	// match padmap button press with button names
				; j++ );

			memset (temp, 0, sizeof(temp));
			strncpy (temp, cfg_btns_menu[i], 12);	// copy snes button information
			if (currentpadmap[i] == ctrlr_def[ctrlr_type].map[j].btn)		// check if a match was made
			{
				for (k=0; k<7-strlen(ctrlr_def[ctrlr_type].map[j].name) ;k++) strcat(temp, " "); // add whitespace padding to align text
				strncat (temp, ctrlr_def[ctrlr_type].map[j].name, 6);		// update button map display
			}
			else
				strcat (temp, (char*)"---");								// otherwise, button is 'unmapped'
			strncpy (cfg_btns_menu[i], temp, 19);	// move back updated information

		}

		ret = RunMenu (cfg_btns_menu, cfg_btns_count, menu_title, 16, -1);

		switch (ret)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				/*** Change button map ***/
				// wait for input
				memset (temp, 0, sizeof(temp));
				strncpy(temp, cfg_btns_menu[ret], 6);			// get the name of the snes button we're changing
				pressed = GetButtonMap(ctrlr_type, temp);	// get a button selection from user
				// FIX: check if input is valid for this controller
				if (pressed != 0)	// check if a the button was configured, or if the user exited.
					currentpadmap[ret] = pressed;	// update mapping
				break;

			case -1: /*** Button B ***/
			case 11:
				/*** Return ***/
				quit = 1;
				break;
		}
	}
	menu = oldmenu;
}	// end configurebuttons()

int ctlrmenucount = 8;
char ctlrmenu[][50] = {
	"Four Score",
	"Zapper",
	"Zapper Crosshair",
	"Nunchuk",
	"Classic Controller",
	"Wiimote",
	"Gamecube Pad",
	"Back to Preferences Menu"
};

void
ConfigureControllers ()
{
	int quit = 0;
	int ret = 0;
	int oldmenu = menu;
	menu = 0;

	// disable unavailable controller options if in GC mode
	#ifndef HW_RVL
		ctlrmenu[3][0] = 0;
		ctlrmenu[4][0] = 0;
		ctlrmenu[5][0] = 0;
	#endif

	while (quit == 0)
	{
		sprintf (ctlrmenu[0], "Four Score - %s",
			GCSettings.FSDisable == false ? " ON" : "OFF");

		if (GCSettings.zapper == 0) sprintf (ctlrmenu[1],"Zapper - Disabled");
		else if (GCSettings.zapper == 1) sprintf (ctlrmenu[1],"Zapper - Port 1");
		else if (GCSettings.zapper == 2) sprintf (ctlrmenu[1],"Zapper - Port 2");

		sprintf (ctlrmenu[2], "Zapper Crosshair - %s",
			GCSettings.crosshair == true ? " ON" : "OFF");

		/*** Controller Config Menu ***/
        ret = RunMenu (ctlrmenu, ctlrmenucount, (char*)"Configure Controllers", 20, -1);

		switch (ret)
		{
			case 0: // four score
				GCSettings.FSDisable ^= 1;
				ToggleFourScore(GCSettings.FSDisable, romLoaded);
				break;

			case 1: // zapper
				GCSettings.zapper -= 1; // we do this so Port 2 is first option shown
				if(GCSettings.zapper < 0)
					GCSettings.zapper = 2;
				ToggleZapper(GCSettings.zapper, romLoaded);
				break;

			case 2: // zapper crosshair
				GCSettings.crosshair ^= 1;
				break;

			case 3:
				/*** Configure Nunchuk ***/
				ConfigureButtons (CTRLR_NUNCHUK);
				break;

			case 4:
				/*** Configure Classic ***/
				ConfigureButtons (CTRLR_CLASSIC);
				break;

			case 5:
				/*** Configure Wiimote ***/
				ConfigureButtons (CTRLR_WIIMOTE);
				break;

			case 6:
				/*** Configure GC Pad ***/
				ConfigureButtons (CTRLR_GCPAD);
				break;

			case -1: /*** Button B ***/
			case 7:
				/*** Return ***/
				quit = 1;
				break;
		}
	}

	menu = oldmenu;
}

/****************************************************************************
 * Preferences Menu
 ***************************************************************************/
static int prefmenuCount = 5;
static char prefmenu[][50] = {
	"Controllers",
	"Video",
	"Saving / Loading",
	"Reset Preferences",
	"Back to Main Menu"
};

void
PreferencesMenu ()
{
	int ret = 0;
	int quit = 0;
	int oldmenu = menu;
	menu = 0;
	while (quit == 0)
	{
		ret = RunMenu (prefmenu, prefmenuCount, (char*)"Preferences", 20, -1);

		switch (ret)
		{
			case 0:
				ConfigureControllers ();
				break;

			case 1:
				VideoOptions ();
				break;

			case 2:
				FileOptions ();
				break;

			case 3:
				DefaultSettings ();
				WaitPrompt((char *)"Preferences Reset");
				break;

			case -1: /*** Button B ***/
			case 4:
				SavePrefs(GCSettings.SaveMethod, SILENT);
				quit = 1;
				break;

		}
	}
	menu = oldmenu;
}

/****************************************************************************
 * Main Menu
 ****************************************************************************/
int menucount = 6;
char menuitems[][50] = {
  "Choose Game",
  "Preferences",
  "Game Menu",
  "Credits",
  "Reset System",
  "Exit"
};

void
MainMenu (int selectedMenu)
{
	int quit = 0;
	int ret;

	VIDEO_WaitVSync ();

	while (quit == 0)
	{
		// disable game-specific menu items if a ROM isn't loaded
		if(!romLoaded)
			menuitems[2][0] = '\0';
		else
			sprintf (menuitems[2], "Game Menu");

		if(selectedMenu >= 0)
		{
			ret = selectedMenu;
			selectedMenu = -1; // default back to main menu
		}
		else
		{
			ret = RunMenu (menuitems, menucount, (char*)"Main Menu", 20, -1);
		}

		switch (ret)
		{
			case 0:
				// Load ROM Menu
				quit = LoadManager ();
				break;

			case 1:
				// Preferences
				PreferencesMenu ();
				break;

			case 2:
				// Game Options
				quit = GameMenu ();
				break;

			case 3:
				// Credits
				Credits ();
				WaitButtonA ();
                break;

			case 4:
				// Reset the Gamecube/Wii
			    Reboot();
                break;

			case 5:
				ExitToLoader();
				break;

			case -1: // Button B
				// Return to Game
				if(romLoaded)
					quit = 1;
				break;
		}
	}

	// Wait for buttons to be released
	int count = 0; // how long we've been waiting for the user to release the button
	while(count < 50 && (
		PAD_ButtonsHeld(0)
		#ifdef HW_RVL
		|| WPAD_ButtonsHeld(0)
		#endif
	))
	{
		VIDEO_WaitVSync();
		count++;
	}
}
