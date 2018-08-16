/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * pad.cpp
 *
 * Controller input
 ****************************************************************************/

#include <gccore.h>
#include <math.h>
#include <ogc/lwp_watchdog.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "gcaudio.h"
#include "menu.h"
#include "gcvideo.h"
#include "filebrowser.h"
#include "button_mapping.h"
#include "fceuload.h"
#include "gui/gui.h"

int rumbleRequest[4] = {0,0,0,0};
GuiTrigger userInput[4];

#ifdef HW_RVL
static int rumbleCount[4] = {0,0,0,0};
#endif

static uint32 JSReturn = 0;
void *InputDPR;

static INPUTC *zapperdata[2];
static unsigned int myzappers[2][3];

u32 nespadmap[11]; // Original NES controller buttons
u32 zapperpadmap[11]; // Original NES Zapper controller buttons
u32 btnmap[2][5][12]; // button mapping

void ResetControls(int consoleCtrl, int wiiCtrl)
{
	int i = 0;

	// Original NES controller buttons
	// All other pads are mapped to this
	i=0;
	nespadmap[i++] = JOY_B;
	nespadmap[i++] = JOY_A;
	nespadmap[i++] = RAPID_B;
	nespadmap[i++] = RAPID_A; // rapid press A/B buttons
	nespadmap[i++] = JOY_SELECT;
	nespadmap[i++] = JOY_START;
	nespadmap[i++] = JOY_UP;
	nespadmap[i++] = JOY_DOWN;
	nespadmap[i++] = JOY_LEFT;
	nespadmap[i++] = JOY_RIGHT;
	nespadmap[i++] = 0; // insert coin for VS games, insert/eject/select disk for FDS

	/*** Gamecube controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_TRIGGER_Z;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_START;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_TRIGGER_L;
	}

	/*** Wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_1;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_2;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A;
	}

	/*** Classic Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_CLASSIC))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_FULL_L;
	}

	/*** Classic Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_WUPC))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_FULL_L;
	}

	/*** Nunchuk + wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_NUNCHUK))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_NUNCHUK_BUTTON_C;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_NUNCHUK_BUTTON_Z;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_A;
	}

	/*** Zapper : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_ZAPPER][CTRLR_GCPAD][i++] = PAD_BUTTON_A; // shoot
		btnmap[CTRL_ZAPPER][CTRLR_GCPAD][i++] = PAD_BUTTON_B; // insert coin
	}

	/*** Zapper : wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_B; // shoot
		btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A; // insert coin
	}
}

/****************************************************************************
 * SetControllers
 ***************************************************************************/
void SetControllers()
{
	if(!romLoaded)
		return;

	InputDPR = &JSReturn;

	if(GCSettings.Controller == CTRL_PAD4)
		FCEUI_SetInputFourscore(1);
	else
		FCEUI_SetInputFourscore(0);

	// set defaults
	zapperdata[0]=NULL;
	zapperdata[1]=NULL;
	myzappers[0][0]=myzappers[1][0]=128;
	myzappers[0][1]=myzappers[1][1]=120;
	myzappers[0][2]=myzappers[1][2]=0;

	// Default ports back to gamepad
	FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, 0);
	FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, 0);

	if(GCSettings.Controller == CTRL_ZAPPER)
	{
		int p;
		if(GameInfo->type == GIT_VSUNI) p = 0;
		else p = 1;
		zapperdata[p] = FCEU_InitZapper(p);
		FCEUI_SetInput(p, SI_ZAPPER, myzappers[p], 1);
	}
}

/****************************************************************************
 * UpdatePads
 *
 * Scans pad and wpad
 ***************************************************************************/

void
UpdatePads()
{
	#ifdef HW_RVL
	WPAD_ScanPads();
	#endif
	
	PAD_ScanPads();

	for(int i=3; i >= 0; i--)
	{
		userInput[i].pad.btns_d = PAD_ButtonsDown(i);
		userInput[i].pad.btns_u = PAD_ButtonsUp(i);
		userInput[i].pad.btns_h = PAD_ButtonsHeld(i);
		userInput[i].pad.stickX = PAD_StickX(i);
		userInput[i].pad.stickY = PAD_StickY(i);
		userInput[i].pad.substickX = PAD_SubStickX(i);
		userInput[i].pad.substickY = PAD_SubStickY(i);
		userInput[i].pad.triggerL = PAD_TriggerL(i);
		userInput[i].pad.triggerR = PAD_TriggerR(i);
	}
}

/****************************************************************************
 * SetupPads
 *
 * Sets up userInput triggers for use
 ***************************************************************************/
void
SetupPads()
{
	PAD_Init();

	#ifdef HW_RVL
	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);
	#endif

	for(int i=0; i < 4; i++)
	{
		userInput[i].chan = i;
		#ifdef HW_RVL
		userInput[i].wpad = WPAD_Data(i);
		#endif
	}
}

#ifdef HW_RVL
/****************************************************************************
 * ShutoffRumble
 ***************************************************************************/

void ShutoffRumble()
{
	if(CONF_GetPadMotorMode() == 0)
		return;

	for(int i=0;i<4;i++)
	{
		WPAD_Rumble(i, 0);
		rumbleCount[i] = 0;
		rumbleRequest[i] = 0;
	}
}

/****************************************************************************
 * DoRumble
 ***************************************************************************/

void DoRumble(int i)
{
	if(CONF_GetPadMotorMode() == 0 || !GCSettings.Rumble)
		return;

	if(rumbleRequest[i] && rumbleCount[i] < 3)
	{
		WPAD_Rumble(i, 1); // rumble on
		rumbleCount[i]++;
	}
	else if(rumbleRequest[i])
	{
		rumbleCount[i] = 12;
		rumbleRequest[i] = 0;
	}
	else
	{
		if(rumbleCount[i])
			rumbleCount[i]--;
		WPAD_Rumble(i, 0); // rumble off
	}
}
#endif

// hold zapper cursor positions
static int pos_x = 0;
static int pos_y = 0;

static void UpdateCursorPosition (int chan)
{
	#define ZAPPERPADCAL 20
	// gc left joystick

	if (userInput[chan].pad.stickX > ZAPPERPADCAL)
	{
		pos_x += (userInput[chan].pad.stickX*1.0)/ZAPPERPADCAL;
		if (pos_x > 256) pos_x = 256;
	}
	if (userInput[chan].pad.stickX < -ZAPPERPADCAL)
	{
		pos_x -= (userInput[chan].pad.stickX*-1.0)/ZAPPERPADCAL;
		if (pos_x < 0) pos_x = 0;
	}

	if (userInput[chan].pad.stickY < -ZAPPERPADCAL)
	{
		pos_y += (userInput[chan].pad.stickY*-1.0)/ZAPPERPADCAL;
		if (pos_y > 224) pos_y = 224;
	}
	if (userInput[chan].pad.stickY > ZAPPERPADCAL)
	{
		pos_y -= (userInput[chan].pad.stickY*1.0)/ZAPPERPADCAL;
		if (pos_y < 0) pos_y = 0;
	}

#ifdef HW_RVL
	if (userInput[chan].wpad->ir.valid)
	{
		pos_x = (userInput[chan].wpad->ir.x * 256) / 640;
		pos_y = (userInput[chan].wpad->ir.y * 224) / 480;
	}
	else
	{
		s8 wm_ax = userInput[chan].WPAD_StickX(0);
		s8 wm_ay = userInput[chan].WPAD_StickY(0);

		if (wm_ax > ZAPPERPADCAL)
		{
			pos_x += (wm_ax*1.0)/ZAPPERPADCAL;
			if (pos_x > 256) pos_x = 256;
		}
		if (wm_ax < -ZAPPERPADCAL)
		{
			pos_x -= (wm_ax*-1.0)/ZAPPERPADCAL;
			if (pos_x < 0) pos_x = 0;
		}
		if (wm_ay < -ZAPPERPADCAL)
		{
			pos_y += (wm_ay*-1.0)/ZAPPERPADCAL;
			if (pos_y > 224) pos_y = 224;
		}
		if (wm_ay > ZAPPERPADCAL)
		{
			pos_y -= (wm_ay*1.0)/ZAPPERPADCAL;
			if (pos_y < 0) pos_y = 0;
		}
	}
#endif
}

/****************************************************************************
 * Convert GC Joystick Readings to JOY
 ****************************************************************************/

extern int rapidAlternator;

static unsigned char DecodeJoy(unsigned short chan)
{
	s8 pad_x = userInput[chan].pad.stickX;
	s8 pad_y = userInput[chan].pad.stickY;
	u32 jp = userInput[chan].pad.btns_h;
	unsigned char J = 0;
	double angle;
	static const double THRES = 0.38268343236508984; // cos(67.5)

	#ifdef HW_RVL
	s8 wm_ax = userInput[chan].WPAD_StickX(0);
	s8 wm_ay = userInput[chan].WPAD_StickY(0);
	u32 wp = userInput[chan].wpad->btns_h;
	bool isWUPC = userInput[chan].wpad->exp.classic.type == 2;

	u32 exp_type;
	if ( WPAD_Probe(chan, &exp_type) != 0 )
		exp_type = WPAD_EXP_NONE;
	#endif

	/***
	Gamecube Joystick input
	***/
	// Is XY inside the "zone"?
	if (pad_x * pad_x + pad_y * pad_y > PADCAL * PADCAL)
	{
		angle = atan2(pad_y, pad_x);
 
		if(cos(angle) > THRES)
			J |= JOY_RIGHT;
		else if(cos(angle) < -THRES)
			J |= JOY_LEFT;
		if(sin(angle) > THRES)
			J |= JOY_UP;
		else if(sin(angle) < -THRES)
			J |= JOY_DOWN;
	}
#ifdef HW_RVL
	/***
	Wii Joystick (classic, nunchuk) input
	***/
	// Is XY inside the "zone"?
	if (wm_ax * wm_ax + wm_ay * wm_ay > PADCAL * PADCAL)
	{
		angle = atan2(wm_ay, wm_ax);
		 
		if(cos(angle) > THRES)
			J |= JOY_RIGHT;
		else if(cos(angle) < -THRES)
			J |= JOY_LEFT;
		if(sin(angle) > THRES)
			J |= JOY_UP;
		else if(sin(angle) < -THRES)
			J |= JOY_DOWN;
	}
#endif

	bool zapper_triggered = false;
	// zapper enabled
	if(GCSettings.Controller == CTRL_ZAPPER)
	{
		int z; // NES port # (0 or 1)

		if(GameInfo->type == GIT_VSUNI)
			z = 0;
		else
			z = 1;

		myzappers[z][2] = 0; // reset trigger to not pressed

		// is trigger pressed?
		if ( (jp & btnmap[CTRL_ZAPPER][CTRLR_GCPAD][0])	// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE][0]) )	// wiimote
		#endif
		)
		{
			// report trigger press
			myzappers[z][2] |= 2;
			zapper_triggered = true;
		}

		// VS zapper games
		if ( (jp & btnmap[CTRL_ZAPPER][CTRLR_GCPAD][1])	// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE][1]) )	// wiimote
		#endif
		)
		{
			FCEUI_VSUniCoin(); // insert coin for VS zapper games
			zapper_triggered = true;
		}

		// cursor position
		int channel = 0;	// by default, use wiimote 1
		#ifdef HW_RVL
		if (userInput[1].wpad->ir.valid)
		{
			channel = 1;	// if wiimote 2 is connected, use wiimote 2 as zapper
		}
		#endif
		UpdateCursorPosition(channel); // update cursor for wiimote
		myzappers[z][0] = pos_x;
		myzappers[z][1] = pos_y;

		// Report changes to FCE Ultra
		zapperdata[z]->Update(z,myzappers[z],0);
	}

	// Report pressed buttons (gamepads)
	int i;
	for (i = 0; i < MAXJP; i++)
	{
		if ( (jp & btnmap[CTRL_PAD][CTRLR_GCPAD][i])										// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_PAD][CTRLR_WIIMOTE][i]) )		// wiimote
		|| ( (exp_type == WPAD_EXP_CLASSIC && !isWUPC) && (wp & btnmap[CTRL_PAD][CTRLR_CLASSIC][i]) )	// classic controller
		|| ( (exp_type == WPAD_EXP_CLASSIC && isWUPC) && (wp & btnmap[CTRL_PAD][CTRLR_WUPC][i]) )		// wii u pro controller
		|| ( (exp_type == WPAD_EXP_NUNCHUK) && (wp & btnmap[CTRL_PAD][CTRLR_NUNCHUK][i]) )	// nunchuk + wiimote
		#endif
		)
		{
			// if zapper is on, ignore all buttons except START and SELECT
			if (!zapper_triggered)
			{
				if(rapidAlternator && nespadmap[i] == RAPID_A)
				{
					// activate rapid fire for A button
					J |= JOY_A;
				}
				else if(rapidAlternator && nespadmap[i] == RAPID_B)
				{
					// activate rapid fire for B button
					J |= JOY_B;
				}
				else if(nespadmap[i] > 0)
				{
					J |= nespadmap[i];
				}
				else
				{
					if(GameInfo->type == GIT_FDS) // FDS
					{
						/* the commands shouldn't be issued in parallel so
						 * we'll delay them so the virtual FDS has a chance
						 * to process them
						 */
						if(FDSSwitchRequested == 0)
							FDSSwitchRequested = 1;
					}
					else
						FCEUI_VSUniCoin(); // insert coin for VS Games
				}
			}
		}
	}

	return J;
}

bool MenuRequested()
{
	for(int i=0; i<4; i++)
	{
		if (
			(userInput[i].pad.substickX < -70)
			#ifdef HW_RVL
			|| (userInput[i].wpad->btns_h & WPAD_BUTTON_HOME) ||
			(userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_HOME)
			#endif
		)
		{
			return true;
		}
	}
	return false;
}

void GetJoy()
{
	JSReturn = 0; // reset buttons pressed
	unsigned char pad[4];
	short i;

	UpdatePads();

	// Turbo mode
	// RIGHT on c-stick and on classic ctrlr right joystick
	if(userInput[0].pad.substickX > 70 || userInput[0].WPAD_StickX(1) > 70)
		turbomode = 1;
	else
		turbomode = 0;

	// request to go back to menu
	if(MenuRequested())
		ScreenshotRequested = 1; // go to the menu

	for (i = 0; i < 4; i++)
		pad[i] = DecodeJoy(i);

	JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;
}
