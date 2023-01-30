/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * fceusupport.h
 *
 * FCEU Support Functions
 ****************************************************************************/

#ifndef _FCEUSUPPORT_H_
#define _FCEUSUPPORT_H_

#include <gccore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fceultra/driver.h"
#include "fceultra/fceu.h"
#include "fceultra/input.h"
#include "fceultra/types.h"
#include "fceultra/state.h"
#include "fceultra/ppu.h"
#include "fceultra/cart.h"
#include "fceultra/x6502.h"
#include "fceultra/git.h"
#include "fceultra/palette.h"
#include "fceultra/sound.h"
#include "fceultra/file.h"
#include "fceultra/cheat.h"

extern unsigned char * nesrom;

int GetFCEUTiming();
void UpdateDendy();
void RebuildSubCheats(void);
int AddCheatEntry(const char *name, uint32 addr, uint8 val, int compare, int status, int type);

extern int FDSLoad(const char *name, FCEUFILE *fp);
extern int iNESLoad(const char *name, FCEUFILE *fp, int o);
extern int UNIFLoad(const char *name, FCEUFILE *fp);
extern int NSFLoad(const char *name, FCEUFILE *fp);
extern uint8 *FDSBIOS;
extern uint8 *GENIEROM;
extern FCEUGI *GameInfo;
extern int CloseGame();

extern void PowerNES(void);

extern u32 iNESGameCRC32;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;

extern INPUTC *FCEU_InitZapper(int w);
extern void FCEU_ResetPalette(void);

#define EO_NO8LIM      1
#define EO_SUBASE      2
#define EO_CLIPSIDES   8
#define EO_SNAPNAME    16
#define EO_FOURSCORE	32
#define EO_NOTHROTTLE	64
#define EO_GAMEGENIE	128
#define EO_PAL		256
#define EO_LOWPASS	512
#define EO_AUTOHIDE	1024

extern int NoWaiting;

extern int _sound;
extern long soundrate;
extern long soundbufsize;

int CLImain(int argc, char *argv[]);

// Device management defaults
#define NUM_INPUT_DEVICES 3

// GamePad defaults
#define GAMEPAD_NUM_DEVICES 4
#define GAMEPAD_NUM_BUTTONS 10
extern const char *GamePadNames[GAMEPAD_NUM_BUTTONS];
extern const char *DefaultGamePadDevice[GAMEPAD_NUM_DEVICES];
extern const int DefaultGamePad[GAMEPAD_NUM_DEVICES][GAMEPAD_NUM_BUTTONS];

// PowerPad defaults
#define POWERPAD_NUM_DEVICES 2
#define POWERPAD_NUM_BUTTONS 12
extern const char *PowerPadNames[POWERPAD_NUM_BUTTONS];
extern const char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES];
extern const int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS];

// QuizKing defaults
#define QUIZKING_NUM_BUTTONS 6
extern const char *QuizKingNames[QUIZKING_NUM_BUTTONS];
extern const char *DefaultQuizKingDevice;
extern const int DefaultQuizKing[QUIZKING_NUM_BUTTONS];

// HyperShot defaults
#define HYPERSHOT_NUM_BUTTONS 4
extern const char *HyperShotNames[HYPERSHOT_NUM_BUTTONS];
extern const char *DefaultHyperShotDevice;
extern const int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS];

// Mahjong defaults
#define MAHJONG_NUM_BUTTONS 21
extern const char *MahjongNames[MAHJONG_NUM_BUTTONS];
extern const char *DefaultMahjongDevice;
extern const int DefaultMahjong[MAHJONG_NUM_BUTTONS];

// TopRider defaults
#define TOPRIDER_NUM_BUTTONS 8
extern const char *TopRiderNames[TOPRIDER_NUM_BUTTONS];
extern const char *DefaultTopRiderDevice;
extern const int DefaultTopRider[TOPRIDER_NUM_BUTTONS];

// FTrainer defaults
#define FTRAINER_NUM_BUTTONS 12
extern const char *FTrainerNames[FTRAINER_NUM_BUTTONS];
extern const char *DefaultFTrainerDevice;
extern const int DefaultFTrainer[FTRAINER_NUM_BUTTONS];

// FamilyKeyBoard defaults
#define FAMILYKEYBOARD_NUM_BUTTONS 0x48
extern const char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS];
extern const char *DefaultFamilyKeyBoardDevice;
extern const int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS];

#endif
