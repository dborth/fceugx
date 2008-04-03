/****************************************************************************
 * NES Memory Load Game
 *
 * This performs the functions of LoadGame and iNESLoad from a single module
 * Helper function for GameCube injected ROMS
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../types.h"
/* Switch to asm for faster compilation
 *#include "../../roms/nesrom.h"
 */
#include "../../git.h"
#include "../../driver.h"
#include "../../palette.h"
#include "../../fceu.h"
#include "../../sound.h"

unsigned char *nesromptr;
extern FCEUGI *FCEUGameInfo;
extern int iNESMemLoad( char *rom );
extern void InitialisePads();

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
     FCEUI_SetSoundQuality(0);
     FCEUI_SetSoundVolume(100);
     FCEUI_SetLowPass(0);

     InitialisePads();

     if ( iNESMemLoad( nesromptr ) )
     {
        FCEU_ResetVidSys();
        PowerNES();
		FCEU_ResetPalette();
		FCEU_ResetMessages();	// Save state, status messages, etc.
		SetSoundVariables();
     }
     else
     {
		WaitPrompt("Bad cartridge!");
        return -1;
     }
     
     return 0;;
}

