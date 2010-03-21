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

#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "gcaudio.h"
#include "gcvideo.h"
#include "menu.h"

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

std::fstream* FCEUD_UTF8_fstream(const char *fn, const char *m)
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
DUMMY(FCEUD_SaveStateAs)
DUMMY(FCEUD_LoadStateFrom)
DUMMY(FCEUD_MovieRecordTo)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }
int FCEUD_ShowStatusIcon(void) { return 0; }
bool FCEUI_AviIsRecording(void) { return 0; }
