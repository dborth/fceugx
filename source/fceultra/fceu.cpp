/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "ppu.h"
#include "sound.h"
#include "netplay.h"
#include "file.h"
#include "utils/endian.h"
#include "utils/memory.h"
#include "utils/crc32.h"

#include "cart.h"
#include "nsf.h"
#include "fds.h"
#include "ines.h"
#include "unif.h"
#include "cheat.h"
#include "palette.h"
#include "state.h"
#include "movie.h"
#include "video.h"
#include "input.h"
#include "file.h"
#include "vsuni.h"
#include "ines.h"
#ifdef __WIN_DRIVER__
#include "drivers/win/pref.h"
#include "utils/xstring.h"

extern void CDLoggerROMClosed();
extern void CDLoggerROMChanged();
extern void ResetDebugStatisticsCounters();
extern void SetMainWindowText();
extern bool isTaseditorRecording();

extern int32 fps_scale;
extern int32 fps_scale_unpaused;
extern int32 fps_scale_frameadvance;
#endif

extern void RefreshThrottleFPS();

#ifdef _S9XLUA_H
#include "fceulua.h"
#endif

//TODO - we really need some kind of global platform-specific options api
#ifdef __WIN_DRIVER__
#include "drivers/win/main.h"
#include "drivers/win/memview.h"
#include "drivers/win/cheat.h"
#include "drivers/win/texthook.h"
#include "drivers/win/ram_search.h"
#include "drivers/win/ramwatch.h"
#include "drivers/win/memwatch.h"
#include "drivers/win/tracer.h"
#else
#ifdef GEKKO
#include "driver.h"
#else
#include "drivers/sdl/sdl.h"
#endif
#endif

#include <fstream>
#include <sstream>
#include <string>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>

using namespace std;

//-----------
//overclocking-related
// overclock the console by adding dummy scanlines to PPU loop or to vblank
// disables DMC DMA, WaveHi filling and image rendering for these dummies
// doesn't work with new PPU
bool overclock_enabled = 0;
bool overclocking = 0;
bool skip_7bit_overclocking = 1; // 7-bit samples have priority over overclocking
int normalscanlines;
int totalscanlines;
int postrenderscanlines = 0;
int vblankscanlines = 0;
//------------

int AFon = 1, AFoff = 1, AutoFireOffset = 0; //For keeping track of autofire settings
bool justLagged = false;
bool frameAdvanceLagSkip = false; //If this is true, frame advance will skip over lag frame (i.e. it will emulate 2 frames instead of 1)
bool AutoSS = false;        //Flagged true when the first auto-savestate is made while a game is loaded, flagged false on game close
bool movieSubtitles = true; //Toggle for displaying movie subtitles
bool DebuggerWasUpdated = false; //To prevent the debugger from updating things without being updated.
bool AutoResumePlay = false;
char romNameWhenClosingEmulator[2048] = {0};


FCEUGI::FCEUGI()
	: filename(0),
	  archiveFilename(0) 
{
	//printf("%08x",opsize); // WTF?!
}

FCEUGI::~FCEUGI() 
{
	if (filename) 
	{
		free(filename);
		filename = NULL;
	}
	if (archiveFilename) 
	{
		free(archiveFilename);
		archiveFilename = NULL;
	}
}

bool CheckFileExists(const char* filename) {
	//This function simply checks to see if the given filename exists
	if (!filename) return false;
	fstream test;
	test.open(filename, fstream::in);

	if (test.fail()) {
		test.close();
		return false;
	} else {
		test.close();
		return true;
	}
}

void FCEU_TogglePPU(void) {
	newppu ^= 1;
	if (newppu) {
		FCEU_DispMessage("New PPU loaded", 0);
		FCEUI_printf("New PPU loaded");
		overclock_enabled = 0;
	} else {
		FCEU_DispMessage("Old PPU loaded", 0);
		FCEUI_printf("Old PPU loaded");
	}
	normalscanlines = (dendy ? 290 : 240)+newppu; // use flag as number!
#ifdef __WIN_DRIVER__
	SetMainWindowText();
#endif
}

static void FCEU_CloseGame(void)
{
	if (GameInfo)
	{
		if (AutoResumePlay)
		{
			// save "-resume" savestate
			FCEUSS_Save(FCEU_MakeFName(FCEUMKF_RESUMESTATE, 0, 0).c_str(), false);
		}

#ifdef __WIN_DRIVER__
		extern char LoadedRomFName[2048];
		if (storePreferences(mass_replace(LoadedRomFName, "|", ".").c_str()))
			FCEUD_PrintError("Couldn't store debugging data");
		CDLoggerROMClosed();
#endif

		if (FCEUnetplay) {
			FCEUD_NetworkClose();
		}

		if (GameInfo->name) {
			free(GameInfo->name);
			GameInfo->name = NULL;
		}

		if (GameInfo->type != GIT_NSF) {
#ifdef __WIN_DRIVER__
			if (disableAutoLSCheats == 2)
				FCEU_FlushGameCheats(0, 1);
			else if (disableAutoLSCheats == 1)
				AskSaveCheat();
			else if (disableAutoLSCheats == 0)
#endif
				FCEU_FlushGameCheats(0, 0);
		}

		GameInterface(GI_CLOSE);

		FCEUI_StopMovie();

		ResetExState(0, 0);

		//clear screen when game is closed
		extern uint8 *XBuf;
		if (XBuf)
			memset(XBuf, 0, 256 * 256);

		FCEU_CloseGenie();

		delete GameInfo;
		GameInfo = NULL;

		currFrameCounter = 0;

		//Reset flags for Undo/Redo/Auto Savestating //adelikat: TODO: maybe this stuff would be cleaner as a struct or class
		lastSavestateMade[0] = 0;
		undoSS = false;
		redoSS = false;
		lastLoadstateMade[0] = 0;
		undoLS = false;
		redoLS = false;
		AutoSS = false;
	}
}


uint64 timestampbase;


FCEUGI *GameInfo = NULL;

void (*GameInterface)(GI h);
void (*GameStateRestore)(int version);

readfunc ARead[0x10000];
writefunc BWrite[0x10000];
static readfunc *AReadG;
static writefunc *BWriteG;
static int RWWrap = 0;

//mbg merge 7/18/06 docs
//bit0 indicates whether emulation is paused
//bit1 indicates whether emulation is in frame step mode
int EmulationPaused = 0;
bool frameAdvanceRequested=false;
int frameAdvance_Delay_count = 0;
int frameAdvance_Delay = FRAMEADVANCE_DELAY_DEFAULT;

//indicates that the emulation core just frame advanced (consumed the frame advance state and paused)
bool JustFrameAdvanced = false;

static int *AutosaveStatus; //is it safe to load Auto-savestate
static int AutosaveIndex = 0; //which Auto-savestate we're on
int AutosaveQty = 4; // Number of Autosaves to store
int AutosaveFrequency = 256; // Number of frames between autosaves

// Flag that indicates whether the Auto-save option is enabled or not
int EnableAutosave = 0;

///a wrapper for unzip.c
extern "C" FILE *FCEUI_UTF8fopen_C(const char *n, const char *m) {
	return ::FCEUD_UTF8fopen(n, m);
}

static DECLFW(BNull) {
}

static DECLFR(ANull) {
	return(X.DB);
}

int AllocGenieRW(void) {
	if (!(AReadG = (readfunc*)FCEU_malloc(0x8000 * sizeof(readfunc))))
		return 0;
	if (!(BWriteG = (writefunc*)FCEU_malloc(0x8000 * sizeof(writefunc))))
		return 0;
	RWWrap = 1;
	return 1;
}

void FlushGenieRW(void) {
	int32 x;

	if (RWWrap) {
		for (x = 0; x < 0x8000; x++) {
			ARead[x + 0x8000] = AReadG[x];
			BWrite[x + 0x8000] = BWriteG[x];
		}
		free(AReadG);
		free(BWriteG);
		AReadG = NULL;
		BWriteG = NULL;
		RWWrap = 0;
	}
}

readfunc GetReadHandler(int32 a) {
	if (a >= 0x8000 && RWWrap)
		return AReadG[a - 0x8000];
	else
		return ARead[a];
}

void SetReadHandler(int32 start, int32 end, readfunc func) {
	int32 x;

	if (!func)
		func = ANull;

	if (RWWrap)
		for (x = end; x >= start; x--) {
			if (x >= 0x8000)
				AReadG[x - 0x8000] = func;
			else
				ARead[x] = func;
		}
	else
		for (x = end; x >= start; x--)
			ARead[x] = func;
}

writefunc GetWriteHandler(int32 a) {
	if (RWWrap && a >= 0x8000)
		return BWriteG[a - 0x8000];
	else
		return BWrite[a];
}

void SetWriteHandler(int32 start, int32 end, writefunc func) {
	int32 x;

	if (!func)
		func = BNull;

	if (RWWrap)
		for (x = end; x >= start; x--) {
			if (x >= 0x8000)
				BWriteG[x - 0x8000] = func;
			else
				BWrite[x] = func;
		}
	else
		for (x = end; x >= start; x--)
			BWrite[x] = func;
}

uint8 *RAM;

//---------
//windows might need to allocate these differently, so we have some special code

static void AllocBuffers() {
	RAM = (uint8*)FCEU_gmalloc(0x800);
}

static void FreeBuffers() {
	FCEU_free(RAM);
    RAM = NULL;
}
//------

uint8 PAL = 0;

static DECLFW(BRAML) {
	RAM[A] = V;
}

static DECLFW(BRAMH) {
	RAM[A & 0x7FF] = V;
}

static DECLFR(ARAML) {
	return RAM[A];
}

static DECLFR(ARAMH) {
	return RAM[A & 0x7FF];
}


void ResetGameLoaded(void) {
	if (GameInfo) FCEU_CloseGame();
	EmulationPaused = 0; //mbg 5/8/08 - loading games while paused was bad news. maybe this fixes it
	GameStateRestore = 0;
	PPU_hook = NULL;
	GameHBIRQHook = NULL;
	FFCEUX_PPURead = NULL;
	FFCEUX_PPUWrite = NULL;
	if (GameExpSound.Kill)
		GameExpSound.Kill();
	memset(&GameExpSound, 0, sizeof(GameExpSound));
	MapIRQHook = NULL;
	MMC5Hack = 0;
	PEC586Hack = 0;
	QTAIHack = 0;
	PAL &= 1;
	default_palette_selection = 0;
}

int UNIFLoad(const char *name, FCEUFILE *fp);
int iNESLoad(const char *name, FCEUFILE *fp, int OverwriteVidMode);
int FDSLoad(const char *name, FCEUFILE *fp);
int NSFLoad(const char *name, FCEUFILE *fp);

//name should be UTF-8, hopefully, or else there may be trouble
FCEUGI *FCEUI_LoadGameVirtual(const char *name, int OverwriteVidMode, bool silent)
{
	//----------
	//attempt to open the files
	FCEUFILE *fp;
	char fullname[2048];	// this name contains both archive name and ROM file name
	int lastpal = PAL;
	int lastdendy = dendy;

	const char* romextensions[] = { "nes", "fds", "nsf", 0 };

	// indicator for if the operaton was canceled by user
	// currently there's only one situation:
	// the user clicked cancel form the open from archive dialog
	int userCancel = 0;
	fp = FCEU_fopen(name, 0, "rb", 0, -1, romextensions, &userCancel);

	if (!fp)
	{
		// Although !fp, if the operation was canceled from archive select dialog box, don't show the error message;
		if (!silent && !userCancel)
			FCEU_PrintError("Error opening \"%s\"!", name);

		return 0;
	}
	else if (fp->archiveFilename != "")
	{
		strcpy(fullname, fp->archiveFilename.c_str());
		strcat(fullname, "|");
		strcat(fullname, fp->filename.c_str());
	} else
		strcpy(fullname, name);

	// reset loaded game BEFORE it's loading.
	ResetGameLoaded();
	//file opened ok. start loading.
	FCEU_printf("Loading %s...\n\n", fullname);
	GetFileBase(fp->filename.c_str());
	//reset parameters so they're cleared just in case a format's loader doesn't know to do the clearing
	MasterRomInfoParams = TMasterRomInfoParams();

	if (!AutosaveStatus)
		AutosaveStatus = (int*)FCEU_dmalloc(sizeof(int) * AutosaveQty);
	for (AutosaveIndex = 0; AutosaveIndex < AutosaveQty; ++AutosaveIndex)
		AutosaveStatus[AutosaveIndex] = 0;

	FCEU_CloseGame();
	GameInfo = new FCEUGI();
	memset( (void*)GameInfo, 0, sizeof(FCEUGI));

	GameInfo->filename = strdup(fp->filename.c_str());
	if (fp->archiveFilename != "")
		GameInfo->archiveFilename = strdup(fp->archiveFilename.c_str());
	GameInfo->archiveCount = fp->archiveCount;

	GameInfo->soundchan = 0;
	GameInfo->soundrate = 0;
	GameInfo->name = 0;
	GameInfo->type = GIT_CART;
	GameInfo->vidsys = GIV_USER;
	GameInfo->input[0] = GameInfo->input[1] = SI_UNSET;
	GameInfo->inputfc = SIFC_UNSET;
	GameInfo->cspecial = SIS_NONE;

	//try to load each different format
	bool FCEUXLoad(const char *name, FCEUFILE * fp);

	int load_result;
	load_result = iNESLoad(fullname, fp, OverwriteVidMode);
	if (load_result == LOADER_INVALID_FORMAT)
	{
		load_result = NSFLoad(fullname, fp);
		if (load_result == LOADER_INVALID_FORMAT)
		{
			load_result = UNIFLoad(fullname, fp);
			if (load_result == LOADER_INVALID_FORMAT)
			{
				load_result = FDSLoad(fullname, fp);
			}
		}
	}	
	if (load_result == LOADER_OK)
	{

#ifdef __WIN_DRIVER__
		// ################################## Start of SP CODE ###########################
		extern char LoadedRomFName[2048];
		extern int loadDebugDataFailed;

		if ((loadDebugDataFailed = loadPreferences(mass_replace(LoadedRomFName, "|", ".").c_str())))
			if (!silent)
				FCEU_printf("Couldn't load debugging data.\n");

		// ################################## End of SP CODE ###########################
#endif

		if (OverwriteVidMode)
			FCEU_ResetVidSys();

		if (GameInfo->type != GIT_NSF && 
			FSettings.GameGenie && 
			FCEU_OpenGenie())
		{
			FCEUI_SetGameGenie(false);
#ifdef __WIN_DRIVER__
			genie = 0;
#endif
		}

		PowerNES();

		if (GameInfo->type != GIT_NSF)
			FCEU_LoadGamePalette();

		FCEU_ResetPalette();
		FCEU_ResetMessages();   // Save state, status messages, etc.

		if (!lastpal && PAL) {
			FCEU_DispMessage("PAL mode set", 0);
			FCEUI_printf("PAL mode set\n");
		}
		else if (!lastdendy && dendy) {
			// this won't happen, since we don't autodetect dendy, but maybe someday we will?
			FCEU_DispMessage("Dendy mode set", 0);
			FCEUI_printf("Dendy mode set\n");
		}
		else if ((lastpal || lastdendy) && !(PAL || dendy)) {
			FCEU_DispMessage("NTSC mode set", 0);
			FCEUI_printf("NTSC mode set\n");
		}

		if (GameInfo->type != GIT_NSF && !disableAutoLSCheats)
			FCEU_LoadGameCheats(0);

		if (AutoResumePlay)
		{
			// load "-resume" savestate
			if (FCEUSS_Load(FCEU_MakeFName(FCEUMKF_RESUMESTATE, 0, 0).c_str(), false))
				FCEU_DispMessage("Old play session resumed.", 0);
		}

		ResetScreenshotsCounter();

#ifdef __WIN_DRIVER__
		DoDebuggerDataReload(); // Reloads data without reopening window
		CDLoggerROMChanged();
		if (hMemView) UpdateColorTable();
		if (hCheat)
		{
			UpdateCheatsAdded();
			UpdateCheatRelatedWindow();
		}
		if (FrozenAddressCount)
			FCEU_DispMessage("%d cheats active", 0, FrozenAddressCount);
#endif
	}
	else {
		if (!silent)
		{
			switch (load_result)
			{
			case LOADER_UNHANDLED_ERROR:
				FCEU_PrintError("An error occurred while loading the file.");
				break;
			case LOADER_INVALID_FORMAT:
				FCEU_PrintError("Unknown ROM file format.");
				break;
			}
		}

		delete GameInfo;
		GameInfo = 0;
	}

	FCEU_fclose(fp);
	return GameInfo;
}

FCEUGI *FCEUI_LoadGame(const char *name, int OverwriteVidMode, bool silent)
{
	return FCEUI_LoadGameVirtual(name, OverwriteVidMode, silent);
}


//Return: Flag that indicates whether the function was succesful or not.
bool FCEUI_Initialize() {
	srand(time(0));

	if (!FCEU_InitVirtualVideo()) {
		return false;
	}

	AllocBuffers();

	// Initialize some parts of the settings structure
	//mbg 5/7/08 - I changed the ntsc settings to match pal.
	//this is more for precision emulation, instead of entertainment, which is what fceux is all about nowadays
	memset(&FSettings, 0, sizeof(FSettings));
	//FSettings.UsrFirstSLine[0]=8;
	FSettings.UsrFirstSLine[0] = 0;
	FSettings.UsrFirstSLine[1] = 0;
	//FSettings.UsrLastSLine[0]=231;
	FSettings.UsrLastSLine[0] = 239;
	FSettings.UsrLastSLine[1] = 239;
	FSettings.SoundVolume = 150;      //0-150 scale
	FSettings.TriangleVolume = 256;   //0-256 scale (256 is max volume)
	FSettings.Square1Volume = 256;    //0-256 scale (256 is max volume)
	FSettings.Square2Volume = 256;    //0-256 scale (256 is max volume)
	FSettings.NoiseVolume = 256;      //0-256 scale (256 is max volume)
	FSettings.PCMVolume = 256;        //0-256 scale (256 is max volume)

	FCEUPPU_Init();

	X6502_Init();

	return true;
}

void FCEUI_Kill(void) {
	#ifdef _S9XLUA_H
	FCEU_LuaStop();
	#endif
	FCEU_KillVirtualVideo();
	FCEU_KillGenie();
	FreeBuffers();
}

int rapidAlternator = 0;
//int AutoFirePattern[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };
int AutoFirePatternLength = 2;

void SetAutoFirePattern(int onframes, int offframes) 
{
	//int i;
	//for (i = 0; i < onframes && i < 8; i++) {
	//	AutoFirePattern[i] = 1;
	//}
	//for (; i < 8; i++) {
	//	AutoFirePattern[i] = 0;
	//}
	//if (onframes + offframes < 2) {
	//	AutoFirePatternLength = 2;
	//} else if (onframes + offframes > 8) {
	//	AutoFirePatternLength = 8;
	//} else {
	//	AutoFirePatternLength = onframes + offframes;
	//}
	AutoFirePatternLength = onframes + offframes;
	AFon = onframes; AFoff = offframes;
}

void GetAutoFirePattern( int *onframes, int *offframes)
{
	if ( onframes )
	{
		*onframes = AFon;
	}
	if ( offframes )
	{
		*offframes = AFoff;
	}
}

void SetAutoFireOffset(int offset)
{
	if (offset < 0 || offset > 8) return;
	AutoFireOffset = offset;
}

bool GetAutoFireState(int btnIdx)
{
	return rapidAlternator;
}

void AutoFire(void) 
{
	static int counter = 0;
	if (justLagged == false)
	{
		//counter = (counter + 1) % (8 * 7 * 5 * 3);
		counter = (counter + 1) % AutoFirePatternLength;
	}
	//If recording a movie, use the frame # for the autofire so the offset
	//doesn't get screwed up when loading.
	if (FCEUMOV_Mode(MOVIEMODE_RECORD | MOVIEMODE_PLAY)) 
	{
		//rapidAlternator = AutoFirePattern[(AutoFireOffset + FCEUMOV_GetFrame()) % AutoFirePatternLength]; //adelikat: TODO: Think through this, MOVIEMODE_FINISHED should not use movie data for auto-fire?
		//adelikat: TODO: Think through this, MOVIEMODE_FINISHED should not use movie data for auto-fire?
		rapidAlternator = ( (AutoFireOffset + FCEUMOV_GetFrame()) % AutoFirePatternLength ) < AFon; 
	}
	else
	{
		//rapidAlternator = AutoFirePattern[(AutoFireOffset + counter) % AutoFirePatternLength];
		rapidAlternator = ( (AutoFireOffset + counter) % AutoFirePatternLength ) < AFon;
	}
}

void UpdateAutosave(void);

#ifdef __QT_DRIVER__
extern unsigned int frameAdvHoldTimer;
#endif

///Emulates a single frame.

///Skip may be passed in, if FRAMESKIP is #defined, to cause this to emulate more than one frame
void FCEUI_Emulate(uint8 **pXBuf, int32 **SoundBuf, int32 *SoundBufSize, int skip) {
	//skip initiates frame skip if 1, or frame skip and sound skip if 2
	int r, ssize;

	JustFrameAdvanced = false;

	if (frameAdvanceRequested)
	{
#ifdef __QT_DRIVER__
		uint32_t frameAdvanceDelayScaled = frameAdvance_Delay * (PAL ? 20 : 16);

		if ( frameAdvanceDelayScaled < 1 )
		{
			frameAdvanceDelayScaled = 1;
		}
		if ( (frameAdvance_Delay_count == 0) || (frameAdvHoldTimer >= frameAdvanceDelayScaled) )
		{
			EmulationPaused = EMULATIONPAUSED_FA;
		}
		if (frameAdvance_Delay_count < frameAdvanceDelayScaled)
		{
			frameAdvance_Delay_count++;
		}
#else
		if (frameAdvance_Delay_count == 0 || frameAdvance_Delay_count >= frameAdvance_Delay)
			EmulationPaused = EMULATIONPAUSED_FA;
		if (frameAdvance_Delay_count < frameAdvance_Delay)
			frameAdvance_Delay_count++;
#endif
	}

	if (EmulationPaused & EMULATIONPAUSED_FA)
	{
		// the user is holding Frame Advance key
		// clear paused flag temporarily
		EmulationPaused &= ~EMULATIONPAUSED_PAUSED;
#ifdef __WIN_DRIVER__
		// different emulation speed when holding Frame Advance
		if (fps_scale_frameadvance > 0)
		{
			fps_scale = fps_scale_frameadvance;
			RefreshThrottleFPS();
		}
#endif
	} else
	{
#ifdef __WIN_DRIVER__
		if (fps_scale_frameadvance > 0)
		{
			// restore emulation speed when Frame Advance is not held
			fps_scale = fps_scale_unpaused;
			RefreshThrottleFPS();
		}
#endif
		if (EmulationPaused & EMULATIONPAUSED_PAUSED)
		{
			// emulator is paused
			memcpy(XBuf, XBackBuf, 256*256);
			FCEU_PutImage();
			*pXBuf = XBuf;
			*SoundBuf = WaveFinal;
			*SoundBufSize = 0;
			return;
		}
	}

	AutoFire();
	UpdateAutosave();

#ifdef _S9XLUA_H
	FCEU_LuaFrameBoundary();
#endif

	FCEU_UpdateInput();
	lagFlag = 1;

#ifdef _S9XLUA_H
	CallRegisteredLuaFunctions(LUACALL_BEFOREEMULATION);
#endif

	if (geniestage != 1) FCEU_ApplyPeriodicCheats();
	r = FCEUPPU_Loop(skip);

	if (skip != 2) ssize = FlushEmulateSound();  //If skip = 2 we are skipping sound processing

	//flush tracer once a frame, since we're likely to end up back at a user interaction loop after this with emulation paused
	FCEUD_FlushTrace();

#ifdef _S9XLUA_H
	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);
#endif

	FCEU_PutImage();

#ifdef __WIN_DRIVER__
	//These Windows only dialogs need to be updated only once per frame so they are included here
	// CaH4e3: can't see why, this is only cause problems with selection
	// adelikat: selection is only a problem when not paused, it should be paused to select, we want to see the values update
	// owomomo: use an OWNERDATA CListCtrl to partially solve the problem
	UpdateCheatList();
	UpdateTextHooker();
	Update_RAM_Search(); // Update_RAM_Watch() is also called.
	RamChange();
	//FCEUI_AviVideoUpdate(XBuf);

	extern int KillFCEUXonFrame;
	if (KillFCEUXonFrame && (FCEUMOV_GetFrame() >= KillFCEUXonFrame))
		DoFCEUExit();
#else
		extern int KillFCEUXonFrame;
	if (KillFCEUXonFrame && (FCEUMOV_GetFrame() >= KillFCEUXonFrame))
		exit(0);
#endif

	timestampbase += timestamp;
	timestamp = 0;
	soundtimestamp = 0;

	*pXBuf = skip ? 0 : XBuf;
	if (skip == 2) { //If skip = 2, then bypass sound
		*SoundBuf = 0;
		*SoundBufSize = 0;
	} else {
		*SoundBuf = WaveFinal;
		*SoundBufSize = ssize;
	}

	if ((EmulationPaused & EMULATIONPAUSED_FA) && (!frameAdvanceLagSkip || !lagFlag))
	//Lots of conditions here.  EmulationPaused & EMULATIONPAUSED_FA must be true.  In addition frameAdvanceLagSkip or lagFlag must be false
	// When Frame Advance is held, emulator is automatically paused after emulating one frame (or several lag frames)
	{
		EmulationPaused = EMULATIONPAUSED_PAUSED;		   // restore EMULATIONPAUSED_PAUSED flag and clear EMULATIONPAUSED_FA flag
		JustFrameAdvanced = true;
        #ifdef __WIN_DRIVER__
		if (soundoptions & SO_MUTEFA)  //mute the frame advance if the user requested it
			*SoundBufSize = 0;         //keep sound muted
		#endif
	}

	if (lagFlag) {
		lagCounter++;
		justLagged = true;
	} else justLagged = false;

	if (movieSubtitles)
		ProcessSubtitles();
}

void FCEUI_CloseGame(void) {
	if (!FCEU_IsValidUI(FCEUI_CLOSEGAME))
		return;

	FCEU_CloseGame();
}

void ResetNES(void) {
	FCEUMOV_AddCommand(FCEUNPCMD_RESET);
	if (!GameInfo) return;
	GameInterface(GI_RESETM2);
	FCEUSND_Reset();
	FCEUPPU_Reset();
	X6502_Reset();

	// clear back baffer
	extern uint8 *XBackBuf;
	memset(XBackBuf, 0, 256 * 256);

	// FCEU_DispMessage("Reset", 0);
}


int RAMInitSeed = 0;
int RAMInitOption = 0;

u64 splitmix64(u32 input) {
	u64 z = (input + 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

static inline u64 xoroshiro128plus_rotl(const u64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

u64 xoroshiro128plus_s[2];
void xoroshiro128plus_seed(u32 input)
{
//http://xoroshiro.di.unimi.it/splitmix64.c
	u64 x = input;

	u64 z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	xoroshiro128plus_s[0] = z ^ (z >> 31);
	
	z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	xoroshiro128plus_s[1] = z ^ (z >> 31);
}

//http://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
u64 xoroshiro128plus_next() {
	const u64 s0 = xoroshiro128plus_s[0];
	u64 s1 = xoroshiro128plus_s[1];
	const u64 result = s0 + s1;

	s1 ^= s0;
	xoroshiro128plus_s[0] = xoroshiro128plus_rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
	xoroshiro128plus_s[1] = xoroshiro128plus_rotl(s1, 36); // c

	return result;
}

void FCEU_MemoryRand(uint8 *ptr, uint32 size, bool default_zero) {
	int x = 0;

	while (size) {
		uint8 v = 0;
		switch (RAMInitOption)
		{
			default:
			case 0:
				if (!default_zero) v = (x & 4) ? 0xFF : 0x00;
				else               v = 0x00;
				break;
			case 1: v = 0xFF; break;
			case 2: v = 0x00; break;
			case 3: v = (u8)(xoroshiro128plus_next()); break;

			// the default is this 8 byte pattern: 00 00 00 00 FF FF FF FF
			// it has been used in FCEUX since time immemorial

			// Some games to examine uninitialied RAM problems with:
			// * Cybernoid - music option starts turned off with default pattern
			// * Huang Di - debug mode is enabled with default pattern
			// * Minna no Taabou no Nakayoshi Daisakusen - fails to boot with some patterns
			// * F-15 City War - high score table
			// * 1942 - high score table
			// * Cheetahmen II - may start in different levels with different RAM startup
		}
		*ptr = v;
		x++;
		size--;
		ptr++;
	}
}

void hand(X6502 *X, int type, uint32 A) {
}

void PowerNES(void) {
	FCEUMOV_AddCommand(FCEUNPCMD_POWER);
	if (!GameInfo) return;

	//reseed random, unless we're in a movie
	extern int disableBatteryLoading;
	if(FCEUMOV_Mode(MOVIEMODE_INACTIVE) && !disableBatteryLoading)
	{
		RAMInitSeed = rand() ^ (u32)xoroshiro128plus_next();
	}

	//always reseed the PRNG with the current seed, for deterministic results (for that seed)
	xoroshiro128plus_seed(RAMInitSeed);

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2, 0, RAM);

	FCEU_GeniePower();

	FCEU_MemoryRand(RAM, 0x800);

	SetReadHandler(0x0000, 0xFFFF, ANull);
	SetWriteHandler(0x0000, 0xFFFF, BNull);

	SetReadHandler(0, 0x7FF, ARAML);
	SetWriteHandler(0, 0x7FF, BRAML);

	SetReadHandler(0x800, 0x1FFF, ARAMH);	// Part of a little
	SetWriteHandler(0x800, 0x1FFF, BRAMH);	//hack for a small speed boost.

	InitializeInput();
	FCEUSND_Power();
	FCEUPPU_Power();

	//Have the external game hardware "powered" after the internal NES stuff.  Needed for the NSF code and VS System code.
	GameInterface(GI_POWER);
	if (GameInfo->type == GIT_VSUNI)
		FCEU_VSUniPower();

	//if we are in a movie, then reset the saveram
	extern int disableBatteryLoading;
	if (disableBatteryLoading)
		GameInterface(GI_RESETSAVE);

	timestampbase = 0;
	X6502_Power();
#ifdef __WIN_DRIVER__
	ResetDebugStatisticsCounters();
#endif
	FCEU_PowerCheats();
	LagCounterReset();
	// clear back buffer
	extern uint8 *XBackBuf;
	memset(XBackBuf, 0, 256 * 256);

#ifdef __WIN_DRIVER__
	Update_RAM_Search(); // Update_RAM_Watch() is also called.
#endif

	// FCEU_DispMessage("Power on", 0);
}

void FCEU_ResetVidSys(void) {
	int w;

	if (GameInfo->vidsys == GIV_NTSC)
		w = 0;
	else if (GameInfo->vidsys == GIV_PAL) {
		w = 1;
		dendy = 0;
	} else
		w = FSettings.PAL;

	PAL = w ? 1 : 0;

	if (PAL)
		dendy = 0;

	if (newppu)
		overclock_enabled = 0;

	normalscanlines = (dendy ? 290 : 240)+newppu; // use flag as number!
	totalscanlines = normalscanlines + (overclock_enabled ? postrenderscanlines : 0);
	FCEUPPU_SetVideoSystem(w || dendy);
	SetSoundVariables();
}

FCEUS FSettings;

void FCEU_printf(const char *format, ...) 
{
#ifndef GEKKO
	char temp[2048];

	va_list ap;

	va_start(ap, format);
	vsnprintf(temp, sizeof(temp), format, ap);
	FCEUD_Message(temp);

#if 0
	FILE *ofile;
	ofile = fopen("stdout.txt", "ab");
	fwrite(temp, 1, strlen(temp), ofile);
	fclose(ofile);
#endif

	va_end(ap);
#endif
}

void FCEU_PrintError(const char *format, ...) 
{
#ifndef GEKKO
	char temp[2048];

	va_list ap;

	va_start(ap, format);
	vsnprintf(temp, sizeof(temp), format, ap);
	FCEUD_PrintError(temp);

	va_end(ap);
#endif
}

void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall) {
	FSettings.UsrFirstSLine[0] = ntscf;
	FSettings.UsrLastSLine[0] = ntscl;
	FSettings.UsrFirstSLine[1] = palf;
	FSettings.UsrLastSLine[1] = pall;
	if (PAL || dendy) {
		FSettings.FirstSLine = FSettings.UsrFirstSLine[1];
		FSettings.LastSLine = FSettings.UsrLastSLine[1];
	} else {
		FSettings.FirstSLine = FSettings.UsrFirstSLine[0];
		FSettings.LastSLine = FSettings.UsrLastSLine[0];
	}
}

void FCEUI_SetVidSystem(int a) {
	FSettings.PAL = a ? 1 : 0;
	if (GameInfo) {
		FCEU_ResetVidSys();
		FCEU_ResetPalette();
		FCEUD_VideoChanged();
	}
}

int FCEUI_GetCurrentVidSystem(int *slstart, int *slend) {
	if (slstart)
		*slstart = FSettings.FirstSLine;
	if (slend)
		*slend = FSettings.LastSLine;
	return(PAL);
}

#ifndef GEKKO
int  FCEUI_GetRegion(void)
{
	int region;

	if ( pal_emulation )
	{
		region = 1;
	}
	else if ( dendy )
	{
		region = 2;
	}
	else
	{
		region = 0;
	}
	return region;
}

void FCEUI_SetRegion(int region, int notify) 
{
	switch (region) {
		case 0: // NTSC
			normalscanlines = 240;
			pal_emulation = 0;
			dendy = 0;

			if (notify)
			{
				FCEU_DispMessage("NTSC mode set", 0);
				FCEUI_printf("NTSC mode set\n");
			}
			break;
		case 1: // PAL
			normalscanlines = 240;
			pal_emulation = 1;
			dendy = 0;

			if (notify)
			{
				FCEU_DispMessage("PAL mode set", 0);
				FCEUI_printf("PAL mode set\n");
			}
			break;
		case 2: // Dendy
			normalscanlines = 290;
			pal_emulation = 0;
			dendy = 1;

			if (notify)
			{
				FCEU_DispMessage("Dendy mode set", 0);
				FCEUI_printf("Dendy mode set\n");
			}
			break;
	}
	normalscanlines += newppu;
	totalscanlines = normalscanlines + (overclock_enabled ? postrenderscanlines : 0);
	FCEUI_SetVidSystem(pal_emulation);
	RefreshThrottleFPS();
#ifdef __WIN_DRIVER__
	UpdateCheckedMenuItems();
	PushCurrentVideoSettings();
#endif
}
#endif

//Enable or disable Game Genie option.
void FCEUI_SetGameGenie(bool a) {
	FSettings.GameGenie = a;
}

//this variable isn't used at all, snap is always name-based
//void FCEUI_SetSnapName(bool a)
//{
//	FSettings.SnapName = a;
//}

int32 FCEUI_GetDesiredFPS(void) {
	if (PAL || dendy)
		return(838977920);  // ~50.007
	else
		return(1008307711);  // ~60.1
}

int FCEUI_EmulationPaused(void)
{
	return (EmulationPaused & EMULATIONPAUSED_PAUSED);
}

int FCEUI_EmulationFrameStepped()
{
	return (EmulationPaused & EMULATIONPAUSED_FA);
}

void FCEUI_ClearEmulationFrameStepped()
{
	EmulationPaused &= ~EMULATIONPAUSED_FA;
}

//mbg merge 7/18/06 added
//ideally maybe we shouldnt be using this, but i need it for quick merging
void FCEUI_SetEmulationPaused(int val) {
	EmulationPaused = val;
	if(EmulationPaused)
		FCEUD_FlushTrace();
}

void FCEUI_ToggleEmulationPause(void)
{
	EmulationPaused = (EmulationPaused & EMULATIONPAUSED_PAUSED) ^ EMULATIONPAUSED_PAUSED;
	DebuggerWasUpdated = false;
	if(EmulationPaused)
		FCEUD_FlushTrace();
}

void FCEUI_FrameAdvanceEnd(void) {
	frameAdvanceRequested = false;
}

void FCEUI_FrameAdvance(void) {
	frameAdvanceRequested = true;
	frameAdvance_Delay_count = 0;
}

static int AutosaveCounter = 0;

void UpdateAutosave(void) {
	if (!EnableAutosave || turbo)
		return;

	char * f;
	if (++AutosaveCounter >= AutosaveFrequency) {
		AutosaveCounter = 0;
		AutosaveIndex = (AutosaveIndex + 1) % AutosaveQty;
		f = strdup(FCEU_MakeFName(FCEUMKF_AUTOSTATE, AutosaveIndex, 0).c_str());
		FCEUSS_Save(f, false);
		AutoSS = true;  //Flag that an auto-savestate was made
		free(f);
        f = NULL;
		AutosaveStatus[AutosaveIndex] = 1;
	}
}

void FCEUI_RewindToLastAutosave(void) {
	if (!EnableAutosave || !AutoSS)
		return;

	if (AutosaveStatus[AutosaveIndex] == 1) {
		char * f;
		f = strdup(FCEU_MakeFName(FCEUMKF_AUTOSTATE, AutosaveIndex, 0).c_str());
		FCEUSS_Load(f);
		free(f);
        f = NULL;

		//Set pointer to previous available slot
		if (AutosaveStatus[(AutosaveIndex + AutosaveQty - 1) % AutosaveQty] == 1) {
			AutosaveIndex = (AutosaveIndex + AutosaveQty - 1) % AutosaveQty;
		}

		//Reset time to next Auto-save
		AutosaveCounter = 0;
	}
}

int FCEU_TextScanlineOffset(int y) {
	return FSettings.FirstSLine * 256;
}
int FCEU_TextScanlineOffsetFromBottom(int y) {
	return (FSettings.LastSLine - y) * 256;
}

bool FCEU_IsValidUI(EFCEUI ui) {
#ifndef GEKKO
	switch (ui) {
	case FCEUI_OPENGAME:
	case FCEUI_CLOSEGAME:
		if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR)) return false;
		break;
	case FCEUI_RECORDMOVIE:
	case FCEUI_PLAYMOVIE:
	case FCEUI_QUICKSAVE:
	case FCEUI_QUICKLOAD:
	case FCEUI_SAVESTATE:
	case FCEUI_LOADSTATE:
	case FCEUI_NEXTSAVESTATE:
	case FCEUI_PREVIOUSSAVESTATE:
	case FCEUI_VIEWSLOTS:
		if (!GameInfo) return false;
		if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR)) return false;
		break;

	case FCEUI_STOPMOVIE:
	case FCEUI_TOGGLERECORDINGMOVIE:
		return FCEUMOV_Mode(MOVIEMODE_PLAY | MOVIEMODE_RECORD | MOVIEMODE_FINISHED);

	case FCEUI_PLAYFROMBEGINNING:
		return FCEUMOV_Mode(MOVIEMODE_PLAY | MOVIEMODE_RECORD | MOVIEMODE_TASEDITOR | MOVIEMODE_FINISHED);

	case FCEUI_TRUNCATEMOVIE:
		return FCEUMOV_Mode(MOVIEMODE_PLAY | MOVIEMODE_RECORD);

	case FCEUI_STOPAVI:
		return FCEUI_AviIsRecording();

	case FCEUI_TASEDITOR:
		if (!GameInfo) return false;
		break;

	case FCEUI_RESET:
	case FCEUI_POWER:
	case FCEUI_EJECT_DISK:
	case FCEUI_SWITCH_DISK:
	case FCEUI_INSERT_COIN:
		if (!GameInfo) return false;
		if (FCEUMOV_Mode(MOVIEMODE_RECORD)) return true;
#ifdef __WIN_DRIVER__
		if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR) && isTaseditorRecording()) return true;
#endif
		if (!FCEUMOV_Mode(MOVIEMODE_INACTIVE)) return false;
		break;

	case FCEUI_INPUT_BARCODE:
		if (!GameInfo) return false;
		if (!FCEUMOV_Mode(MOVIEMODE_INACTIVE)) return false;
		break;
	default:
		// Unhandled falls out to end of function
		break;
	}

#endif
	return true;
}

//---------------------
//experimental new mapper and ppu system follows

class FCEUXCart {
public:
int mirroring;
int chrPages, prgPages;
uint32 chrSize, prgSize;
char* CHR, *PRG;

FCEUXCart()
	: CHR(0)
	, PRG(0) {
}

~FCEUXCart() {
	if (CHR) delete[] CHR;
	if (PRG) delete[] PRG;
}

virtual void Power() {
}

protected:
//void SetReadHandler(int32 start, int32 end, readfunc func) {
};

FCEUXCart* cart = 0;

//uint8 Read_ByteFromRom(uint32 A) {
//	if(A>=cart->prgSize) return 0xFF;
//	return cart->PRG[A];
//}
//
//uint8 Read_Unmapped(uint32 A) {
//	return 0xFF;
//}



class NROM : FCEUXCart {
public:
virtual void Power() {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	setprg16(0x8000, 0);
	setprg16(0xC000, ~0);
	setchr8(0);

	vnapage[0] = NTARAM;
	vnapage[2] = NTARAM;
	vnapage[1] = NTARAM + 0x400;
	vnapage[3] = NTARAM + 0x400;
	PPUNTARAM = 0xF;
}
};

void FCEUXGameInterface(GI command)
{
	switch (command)
	{
		case GI_POWER:
			cart->Power();
		break;
		default:
			// Unhandled cases
		break;
	}
}

bool FCEUXLoad(const char *name, FCEUFILE *fp) {
	//read ines header
	iNES_HEADER head;
	if (FCEU_fread(&head, 1, 16, fp) != 16)
		return false;

	//validate header
	if (memcmp(&head, "NES\x1a", 4))
		return 0;

	int mapper = (head.ROM_type >> 4);
	mapper |= (head.ROM_type2 & 0xF0);

	//choose what kind of cart to use.
	cart = (FCEUXCart*)new NROM();

	//fceu ines loading code uses 256 here when the romsize is 0.
	cart->prgPages = head.ROM_size;
	if (cart->prgPages == 0) {
		printf("FCEUX: received zero prgpages\n");
		cart->prgPages = 256;
	}

	cart->chrPages = head.VROM_size;

	cart->mirroring = (head.ROM_type & 1);
	if (head.ROM_type & 8) cart->mirroring = 2;

	//skip trainer
	bool hasTrainer = (head.ROM_type & 4) != 0;
	if (hasTrainer) {
		FCEU_fseek(fp, 512, SEEK_CUR);
	}

	//load data
	cart->prgSize = cart->prgPages * 16 * 1024;
	cart->chrSize = cart->chrPages * 8 * 1024;
	cart->PRG = new char[cart->prgSize];
	cart->CHR = new char[cart->chrSize];
	FCEU_fread(cart->PRG, 1, cart->prgSize, fp);
	FCEU_fread(cart->CHR, 1, cart->chrSize, fp);

	//setup the emulator
	GameInterface = FCEUXGameInterface;
	ResetCartMapping();
	SetupCartPRGMapping(0, (uint8*)cart->PRG, cart->prgSize, 0);
	SetupCartCHRMapping(0, (uint8*)cart->CHR, cart->chrSize, 0);

	return true;
}

uint8 FCEU_ReadRomByte(uint32 i) {
	extern iNES_HEADER head;
	if (i < 16)
		return *((unsigned char*)&head + i);
	if (i < 16 + PRGsize[0])
		return PRGptr[0][i - 16];
	if (i < 16 + PRGsize[0] + CHRsize[0])
		return CHRptr[0][i - 16 - PRGsize[0]];
	return 0;
}

void FCEU_WriteRomByte(uint32 i, uint8 value) {
	if (i < 16)
#ifdef __WIN_DRIVER__
		MessageBox(hMemView, "Sorry", "You can't edit the ROM header.", MB_OK | MB_ICONERROR);
#else
		printf("Sorry, you can't edit the ROM header.\n");
#endif
	if (i < 16 + PRGsize[0])
		PRGptr[0][i - 16] = value;
	else if (i < 16 + PRGsize[0] + CHRsize[0])
		CHRptr[0][i - 16 - PRGsize[0]] = value;
}
