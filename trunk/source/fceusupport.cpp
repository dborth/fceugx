/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * fceusupport.cpp
 *
 * FCEU Support Functions
 ****************************************************************************/

#include <gctypes.h>
#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "menu.h"

bool turbo = false;

/**
 * Closes a game.  Frees memory, and deinitializes the drivers.
 */
int
CloseGame()
{
    if(!romLoaded) {
        return(0);
    }
    FCEUI_CloseGame();
    GameInfo = 0;
    return(1);
}

// File Control
FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
    return NULL;
}

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *n, const char *m)
{
	return NULL;
}

bool FCEUD_ShouldDrawInputAids()
{
	return GCSettings.crosshair;
}

// General Logging
void FCEUD_PrintError(const char *errormsg)
{
	//if(GuiLoaded())
	//	ErrorPrompt(errormsg);
}

void FCEUD_Message(const char *text)
{
}

void FCEUD_VideoChanged()
{
}

static unsigned int keys[256] = {0,}; // with repeat

unsigned int *GetKeyboard(void)
{
	return(keys);
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }

// main interface to FCE Ultra
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if(Buffer && Count > 0)
		PlaySound(Buffer, Count); // play sound
	if(XBuf)
		RenderFrame(XBuf); // output video frame
	GetJoy(); // check controller input
}

//CAK: Stereoscopic 3D Update functions
//CAK: This buffer saves the previous frame (usually the left eye)
//     so it can be mixed with the current frame (usually the right eye)
uint8 XBufLeft[256*256];

//CAK: This uses the previous frame for the right eye, and the current frame for the left eye
//     It then sets the previous frame to the current frame. This is only for the Orb-3D game, and hasn't
//     been tested yet. Probably more than one frame difference will be needed, which requires a circluar
//     buffer of several frames.
void FCEUD_UpdatePulfrich(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if(Buffer && Count > 0)
		PlaySound(Buffer, Count); // play sound
	if(XBuf) {
		RenderStereoFrames(XBuf, XBufLeft); // output video frame
		memcpy(XBufLeft, XBuf, sizeof(XBufLeft)); // output video frame
	}
	GetJoy(); // check controller input
}

//CAK: This doesn't actually draw anything, it just saves the frame in a buffer while we wait
//     for the corresponding right frame.
void FCEUD_UpdateLeft(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if (Buffer && Count > 0)
		PlaySound(Buffer, Count); // play sound
	if (XBuf)
		memcpy(XBufLeft, XBuf, sizeof(XBufLeft)); // output video frame
	GetJoy(); // check controller input
}

//CAK: This draws the saved left frame and the passed in right frame together in 3D.
//     How it so isn't relevant to this function, since the stereoscopic methods are handled in gcvideo.cpp
void FCEUD_UpdateRight(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if (Buffer && Count > 0)
		PlaySound(Buffer, Count); // play sound
	if (XBuf)
		RenderStereoFrames(XBufLeft, XBuf); // output video frames
	GetJoy(); // check controller input
}

// Netplay
int FCEUD_SendData(void *data, uint32 len)
{
    return 1;
}

int FCEUD_RecvData(void *data, uint32 len)
{
    return 0;
}

void FCEUD_NetworkClose(void)
{
}

void FCEUD_NetplayText(uint8 *text)
{
}

// dummy functions

#define DUMMY(f) void f(void) { }
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_TurboOn)
DUMMY(FCEUD_TurboOff)
DUMMY(FCEUD_TurboToggle)
DUMMY(FCEUD_SaveStateAs)
DUMMY(FCEUD_LoadStateFrom)
DUMMY(FCEUD_MovieRecordTo)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_SoundToggle)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }
int FCEUD_ShowStatusIcon(void) { return 0; }
bool FCEUI_AviIsRecording(void) { return 0; }
bool FCEUI_AviEnableHUDrecording() { return 0; }
void FCEUI_SetAviEnableHUDrecording(bool enable) { }
bool FCEUI_AviDisableMovieMessages() { return true; }
const char *FCEUD_GetCompilerString() { return NULL; }
void FCEUI_UseInputPreset(int preset) { }
void FCEUD_SoundVolumeAdjust(int n) { }
void FCEUD_SetEmulationSpeed(int cmd) { }
