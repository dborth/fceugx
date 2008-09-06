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

#include "common.h"
#include "pad.h"
#include "menudraw.h"

unsigned char *nesromptr;
bool romLoaded = false;

extern FCEUGI *FCEUGameInfo;
extern int iNESMemLoad( char *rom );

extern unsigned char nesrom[];

#define SAMPLERATE 48000

int GCMemROM()
{
    nesromptr = &nesrom[0];

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
    FCEUI_SetSoundQuality(1);
    FCEUI_SetSoundVolume(100);
    FCEUI_SetLowPass(0);

    InitialisePads();

    if ( iNESMemLoad( (char *)nesromptr ) )
    {
        FCEU_ResetVidSys();
        PowerNES();
        FCEU_ResetPalette();
        FCEU_ResetMessages();	// Save state, status messages, etc.
        SetSoundVariables();
        romLoaded = true;
    }
    else
    {
        WaitPrompt("Invalid game file!");
        romLoaded = false;
        return 0;
    }

    return 1;
}

