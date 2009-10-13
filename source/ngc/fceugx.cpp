/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceugx.cpp
 *
 * This file controls overall program flow. Most things start and end here!
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gctypes.h>
#include <ogc/system.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <sys/iosupport.h>

#include "fceugx.h"
#include "fceuconfig.h"
#include "fceuload.h"
#include "fceustate.h"
#include "fceuram.h"
#include "fceusupport.h"
#include "menu.h"
#include "preferences.h"
#include "fileop.h"
#include "filebrowser.h"
#include "networkop.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "pad.h"
#include "filelist.h"
#include "gui/gui.h"

#ifdef HW_RVL
#include <di/di.h>
#endif

#include "FreeTypeGX.h"

#include "types.h"

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

extern "C" {
extern void __exception_setreload(int t);
}

static int fskipc = 0;
static int fskip = 0;
static uint8 *gfx=0;
static int32 *sound=0;
static int32 ssize=0;
int ScreenshotRequested = 0;
int ConfigRequested = 0;
int ShutdownRequested = 0;
int ResetRequested = 0;
int ExitRequested = 0;
char appPath[1024] = { 0 };
int frameskip = 0;
int turbomode = 0;
unsigned char * nesrom = NULL;

/****************************************************************************
 * Shutdown / Reboot / Exit
 ***************************************************************************/

static void ExitCleanup()
{
	ShutdownAudio();
	StopGX();

	HaltDeviceThread();
	UnmountAllFAT();

#ifdef HW_RVL
	DI_Close();
#endif
}

#ifdef HW_DOL
	#define PSOSDLOADID 0x7c6000a6
	int *psoid = (int *) 0x80001800;
	void (*PSOReload) () = (void (*)()) 0x80001800;
#endif

void ExitApp()
{
#ifdef HW_RVL
	ShutoffRumble();
#endif

	SavePrefs(SILENT);

	if (romLoaded && !ConfigRequested && GCSettings.AutoSave == 1)
		SaveRAMAuto(SILENT);

	ExitCleanup();

	if(ShutdownRequested)
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);

	#ifdef HW_RVL
	if(GCSettings.ExitAction == 0) // Auto
	{
		char * sig = (char *)0x80001804;
		if(
			sig[0] == 'S' &&
			sig[1] == 'T' &&
			sig[2] == 'U' &&
			sig[3] == 'B' &&
			sig[4] == 'H' &&
			sig[5] == 'A' &&
			sig[6] == 'X' &&
			sig[7] == 'X')
			GCSettings.ExitAction = 3; // Exit to HBC
		else
			GCSettings.ExitAction = 1; // HBC not found
	}
	#endif

	if(GCSettings.ExitAction == 1) // Exit to Menu
	{
		#ifdef HW_RVL
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		#else
			#define SOFTRESET_ADR ((volatile u32*)0xCC003024)
			*SOFTRESET_ADR = 0x00000000;
		#endif
	}
	else if(GCSettings.ExitAction == 2) // Shutdown Wii
	{
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	}
	else // Exit to Loader
	{
		#ifdef HW_RVL
			exit(0);
		#else
			if (psoid[0] == PSOSDLOADID)
				PSOReload();
		#endif
	}
}

#ifdef HW_RVL
void ShutdownCB()
{
	ShutdownRequested = 1;
}
void ResetCB()
{
	ResetRequested = 1;
}
#endif

#ifdef HW_DOL
/****************************************************************************
 * ipl_set_config
 * lowlevel Qoob Modchip disable
 ***************************************************************************/

static void ipl_set_config(unsigned char c)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	unsigned long val,addr;
	addr=0xc0000000;
	val = c << 24;
	exi[0] = ((((exi[0]) & 0x405) | 256) | 48);	//select IPL
	//write addr of IPL
	exi[0 * 5 + 4] = addr;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);
	//write the ipl we want to send
	exi[0 * 5 + 4] = val;
	exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
	while (exi[0 * 5 + 3] & 1);

	exi[0] &= 0x405;	//deselect IPL
}
#endif

/****************************************************************************
 * USB Gecko Debugging
 ***************************************************************************/

static bool gecko = false;
static mutex_t gecko_mutex = 0;

static ssize_t __out_write(struct _reent *r, int fd, const char *ptr, size_t len)
{
	u32 level;

	if (!ptr || len <= 0 || !gecko)
		return -1;

	LWP_MutexLock(gecko_mutex);
	level = IRQ_Disable();
	usb_sendbuffer(1, ptr, len);
	IRQ_Restore(level);
	LWP_MutexUnlock(gecko_mutex);
	return len;
}

const devoptab_t gecko_out = {
	"stdout",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__out_write,// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};

void USBGeckoOutput()
{
	LWP_MutexInit(&gecko_mutex, false);
	gecko = usb_isgeckoalive(1);
	
	devoptab_list[STD_OUT] = &gecko_out;
	devoptab_list[STD_ERR] = &gecko_out;
}

/****************************************************************************
 * main
 * This is where it all happens!
 ***************************************************************************/

int main(int argc, char *argv[])
{
	//USBGeckoOutput(); // uncomment to enable USB gecko output
	__exception_setreload(8);
	
	#ifdef HW_DOL
	ipl_set_config(6); // disable Qoob modchip
	#endif

	#ifdef HW_RVL
	DI_Close(); // fixes some black screen issues
	DI_Init();	// first
	#endif

	InitDeviceThread();
	InitGCVideo (); // Initialize video
	SetupPads();
	ResetVideo_Menu (); // change to menu video mode

	#ifdef HW_RVL
	// Wii Power/Reset buttons
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);
	#endif

	// Initialize DVD subsystem (GameCube only)
	#ifdef HW_DOL
	DVD_Init ();
	#endif

	// store path app was loaded from
#ifdef HW_RVL
	if(argc > 0 && argv[0] != NULL)
		CreateAppPath(argv[0]);
#endif

	MountAllFAT(); // Initialize libFAT for SD and USB

	DefaultSettings(); // Set defaults

	// Audio
	InitialiseAudio();

	// Initialize font system
	InitFreeType((u8*)font_ttf, font_ttf_size);

	InitGUIThreads();

	// allocate memory to store rom
	nesrom = (unsigned char *)memalign(32,1024*1024*3); // 3 MB should be plenty

	/*** Minimal Emulation Loop ***/
	if (!FCEUI_Initialize())
		ExitApp();

	FCEUI_SetGameGenie(1); // 0 - OFF, 1 - ON

	memset(FDSBIOS, 0, sizeof(FDSBIOS)); // clear FDS BIOS memory

	FCEUI_SetSoundQuality(1); // 0 - low, 1 - high, 2 - high (alt.)
	int currentTiming = 0;

    while (1) // main loop
    {
    	// go back to checking if devices were inserted/removed
		// since we're entering the menu
    	ResumeDeviceThread();

		SwitchAudioMode(1);

		if(!romLoaded)
			MainMenu(MENU_GAMESELECTION);
		else
			MainMenu(MENU_GAME);

		if(currentTiming != GCSettings.timing)
			FCEUI_SetVidSystem(GCSettings.timing); // causes a small 'pop' in the audio

		currentTiming = GCSettings.timing;
		ConfigRequested = 0;
		ScreenshotRequested = 0;
		SwitchAudioMode(0);

		// stop checking if devices were removed/inserted
		// since we're starting emulation again
		HaltDeviceThread();

		ResetVideo_Emu();
		SetControllers();
		setFrameTimer(); // set frametimer method before emulation
		SetPalette();
		FCEUI_DisableSpriteLimitation(GCSettings.spritelimit ^ 1);

		fskip=0;
		fskipc=0;
		frameskip=0;

		while(1) // emulation loop
		{
			fskip = 0;
			
			if(turbomode)
			{
				fskip = 1;
								
				if(fskipc >= 18)
				{
					fskipc = 0;
					fskip = 0;
				}
				else
				{
					fskipc++;
				}
			}
			else if(frameskip > 0)
			{
				fskip = 1;
				
				if(fskipc >= frameskip)
				{
					fskipc = 0;
					fskip = 0;
				}
				else
				{
					fskipc++;
				}
			}

			FCEUI_Emulate(&gfx, &sound, &ssize, fskip);
			FCEUD_Update(gfx, sound, ssize);
			SyncSpeed();

			if(ResetRequested)
			{
				PowerNES(); // reset game
				ResetRequested = 0;
			}
			if(ConfigRequested)
			{
				ConfigRequested = 0;
				ResetVideo_Menu();
				break;
			}
			#ifdef HW_RVL
			if(ShutdownRequested)
				ExitApp();
			#endif

		} // emulation loop
    } // main loop
}
