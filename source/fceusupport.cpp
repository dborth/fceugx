/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceusupport.cpp
 *
 * FCEU Support Functions
 ****************************************************************************/

#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "videosupport.h"
#include "drivers/Platform.h"
#include "menu.h"

bool turbo = false;
bool paldeemphswap = 0;
int dendy;
bool swapDuty;
int KillFCEUXonFrame = 0;

int GetFCEUTiming()
{
	if (EmuSettings.timing == TIMING_DENDY) {
		return TIMING_AUTOMATIC;
	}

	return EmuSettings.timing;
}

void UpdateDendy()
{
	if(EmuSettings.timing == TIMING_DENDY) {
		dendy = 1;
	}
	else {
		dendy = 0;
	}
}

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
FILE *FCEUD_UTF8fopen(const char *, const char *)
{
    return nullptr;
}

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *, const char *)
{
	return nullptr;
}

bool FCEUD_ShouldDrawInputAids()
{
	return EmuSettings.crosshair;
}

// General Logging
void FCEUD_PrintError(const char *)
{

}

void FCEUD_Message(const char *)
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

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord&, std::string &, int) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord&, std::string&, std::string*) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string) { return ArchiveScanRecord(); }

// main interface to FCE Ultra
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if(Buffer && Count > 0)
		platform->getAudio()->getEmulatorAudio()->playSound(Buffer, Count); // play sound
	if(XBuf)
		platform->getVideo()->getEmulatorVideo()->presentFrame(XBuf); // output video frame
	GetJoy(); // check controller input
}

// Stereoscopic 3D Update functions
// This buffer saves the previous frame (usually the left eye)
// so it can be mixed with the current frame (usually the right eye)
uint8 XBufLeft[256*256];

// This uses the previous frame for the right eye, and the current frame for the left eye
// It then sets the previous frame to the current frame. This is only for the Orb-3D game, and hasn't
// been tested yet. Probably more than one frame difference will be needed, which requires a circluar
// buffer of several frames.
void FCEUD_UpdatePulfrich(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if(Buffer && Count > 0)
		platform->getAudio()->getEmulatorAudio()->playSound(Buffer, Count); // play sound
	if(XBuf) {
		platform->getVideo()->getEmulatorVideo()->presentStereoFrame(XBuf, XBufLeft); // output video frame
		memcpy(XBufLeft, XBuf, sizeof(XBufLeft)); // output video frame
	}
	GetJoy(); // check controller input
}

// This doesn't actually draw anything, it just saves the frame in a buffer while we wait
// for the corresponding right frame.
void FCEUD_UpdateLeft(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if (Buffer && Count > 0)
		platform->getAudio()->getEmulatorAudio()->playSound(Buffer, Count); // play sound
	if (XBuf)
		memcpy(XBufLeft, XBuf, sizeof(XBufLeft)); // output video frame
	GetJoy(); // check controller input
}

// This draws the saved left frame and the passed in right frame together in 3D.
void FCEUD_UpdateRight(uint8 *XBuf, int32 *Buffer, int32 Count)
{
	if (Buffer && Count > 0)
		platform->getAudio()->getEmulatorAudio()->playSound(Buffer, Count); // play sound
	if (XBuf)
		platform->getVideo()->getEmulatorVideo()->presentStereoFrame(XBufLeft, XBuf); // output video frames
	GetJoy(); // check controller input
}

// Netplay
int FCEUD_SendData(void *, uint32)
{
    return 1;
}

int FCEUD_RecvData(void *, uint32)
{
    return 0;
}

void FCEUD_NetworkClose(void)
{
}

void FCEUD_NetplayText(uint8 *)
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
DUMMY(FCEUD_FlushTrace)
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_SoundToggle)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char*) { }
int FCEUD_ShowStatusIcon(void) { return 0; }
bool FCEUI_AviIsRecording(void) { return 0; }
bool FCEUI_AviEnableHUDrecording() { return 0; }
void FCEUI_SetAviEnableHUDrecording(bool) { }
bool FCEUI_AviDisableMovieMessages() { return true; }
const char *FCEUD_GetCompilerString() { return nullptr; }
void FCEUI_UseInputPreset(int) { }
void FCEUD_SoundVolumeAdjust(int) { }
void FCEUD_SetEmulationSpeed(int) { }
void GetMouseData(uint32 (&)[3]) { }
