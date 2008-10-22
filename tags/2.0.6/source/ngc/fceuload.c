/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * fceload.c
 *
 * NES Memory Load Game
 *
 * This performs the functions of LoadGame and iNESLoad from a single module
 * Helper function for GameCube injected ROMS
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "git.h"
#include "driver.h"
#include "palette.h"
#include "fceu.h"
#include "sound.h"
#include "file.h"

#include "common.h"
#include "pad.h"
#include "menudraw.h"
#include "fceuconfig.h"
#include "fileop.h"
#include "filesel.h"
#include "smbop.h"

unsigned char *nesrom;
bool romLoaded = false;

extern FCEUGI *FCEUGameInfo;

#define SAMPLERATE 48000

FCEUFILE *fceufp = NULL;
MEMWRAP *fceumem = NULL;
unsigned char * fceuFileData = NULL;

void MakeFCEUFile(char * membuffer, int length)
{
	if(fceufp != NULL)
	{
		free(fceuFileData);
		free(fceumem);
		free(fceufp);
		fceuFileData = NULL;
		fceumem = NULL;
		fceufp = NULL;
	}

	fceufp =(FCEUFILE *)malloc(sizeof(FCEUFILE));
	fceufp->type=3;
	fceumem = (MEMWRAP *)malloc(sizeof(MEMWRAP));
	fceumem->location=0;
	fceumem->size=length;
	fceuFileData = (unsigned char *)malloc(length);
	memcpy(fceuFileData, membuffer, length);
	fceumem->data=fceuFileData;
	fceufp->fp = fceumem;
}

extern int FDSLoad(const char *name, FCEUFILE *fp);
extern int iNESLoad(const char *name, FCEUFILE *fp);
extern int UNIFLoad(const char *name, FCEUFILE *fp);
extern int NSFLoad(FCEUFILE *fp);
extern uint8 FDSBIOS[8192];

int GCMemROM(int method, int size)
{
    ResetGameLoaded();

    /*** Allocate and clear GameInfo ***/

    FCEUGameInfo = malloc(sizeof(FCEUGI));
    memset(FCEUGameInfo, 0, sizeof(FCEUGI));

    /*** Set some default values ***/
    FCEUGameInfo->soundchan = 1;
    FCEUGameInfo->soundrate = SAMPLERATE;
    FCEUGameInfo->name=0;
    FCEUGameInfo->type=GIT_CART;
    FCEUGameInfo->vidsys=GIV_USER;
    FCEUGameInfo->input[0]=FCEUGameInfo->input[1]=-1;
    FCEUGameInfo->inputfc=-1;
    FCEUGameInfo->cspecial=0;

    /*** Set internal sound information ***/
    FCEUI_Sound(SAMPLERATE);
    FCEUI_SetSoundVolume(100); // 0-100
    FCEUI_SetSoundQuality(2); // 0 - low, 1 - high, 2 - high (alt.)
    FCEUI_SetLowPass(0);

    InitialisePads();

    MakeFCEUFile((char *)nesrom, size);

    nesGameType = 0;

    if(iNESLoad(NULL, fceufp))
		nesGameType = 1;
	else if(UNIFLoad(NULL,fceufp))
		nesGameType = 2;
	else if(NSFLoad(fceufp))
		nesGameType = 3;
	else
	{
		// read FDS BIOS into FDSBIOS - should be 8192 bytes
		if(FDSBIOS[1] == 0)
		{
			int biosSize = 0;
			char * tmpbuffer = (char *)malloc(64 * 1024);

			char filepath[1024];

			switch (method)
			{
				case METHOD_SD:
				case METHOD_USB:
					sprintf(filepath, "%s/%s/disksys.rom", ROOTFATDIR, GCSettings.LoadFolder);
					biosSize = LoadBufferFromFAT(tmpbuffer, filepath, NOTSILENT);
					break;
				case METHOD_SMB:
					sprintf(filepath, "%s/disksys.rom", GCSettings.LoadFolder);
					biosSize = LoadBufferFromSMB(tmpbuffer, filepath, 0, NOTSILENT);
					break;
			}

			if(biosSize == 8192)
			{
				memcpy(FDSBIOS, tmpbuffer, 8192);
			}
			else
			{
				if(biosSize > 0)
					WaitPrompt("FDS BIOS file is invalid!");

				return 0; // BIOS not loaded, do not load game
			}
			free(tmpbuffer);
		}
		// load game
		if(FDSLoad(NULL,fceufp))
			nesGameType = 4;
	}

    if (nesGameType > 0)
    {
        FCEU_ResetVidSys();
        PowerNES();
        FCEU_ResetPalette();
        FCEU_ResetMessages();	// Save state, status messages, etc.
        SetSoundVariables();
        romLoaded = true;
        return 1;
    }
    else
    {
        WaitPrompt("Invalid game file!");
        romLoaded = false;
        return 0;
    }
}
