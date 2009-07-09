/****************************************************************************
 * FCE Ultra 0.98.12
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

#include "fceugx.h"
#include "pad.h"
#include "gcaudio.h"
#include "menu.h"
#include "gcvideo.h"
#include "filebrowser.h"
#include "button_mapping.h"
#include "gui/gui.h"
#include "fceuload.h"

extern "C" {
#include "driver.h"
#include "fceu.h"
#include "input.h"
extern INPUTC *FCEU_InitZapper(int w);
}

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
u32 btnmap[2][4][12]; // button mapping

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
		FCEUI_DisableFourScore(false);
	else
		FCEUI_DisableFourScore(true);

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
		if(nesGameType == 2) p = 0;
		else p = 1;
		zapperdata[p] = FCEU_InitZapper(p);
		FCEUI_SetInput(p, SI_ZAPPER, myzappers[p], 1);
	}
}

/****************************************************************************
 * UpdatePads
 *
 * called by postRetraceCallback in InitGCVideo - scans pad and wpad
 ***************************************************************************/
void UpdatePads()
{
	#ifdef HW_RVL
	WPAD_ScanPads();
	#endif
	PAD_ScanPads();

	for(int i=3; i >= 0; i--)
	{
		#ifdef HW_RVL
		memcpy(&userInput[i].wpad, WPAD_Data(i), sizeof(WPADData));
		#endif

		userInput[i].chan = i;
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

#ifdef HW_RVL

/****************************************************************************
 * ShutoffRumble
 ***************************************************************************/

void ShutoffRumble()
{
	for(int i=0;i<4;i++)
	{
		WPAD_Rumble(i, 0);
		rumbleCount[i] = 0;
	}
}

/****************************************************************************
 * DoRumble
 ***************************************************************************/

void DoRumble(int i)
{
	if(!GCSettings.Rumble) return;
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

s8 WPAD_StickX(u8 chan,u8 right)
{
	float mag = 0.0;
	float ang = 0.0;
	WPADData *data = WPAD_Data(chan);

	switch (data->exp.type)
	{
		case WPAD_EXP_NUNCHUK:
		case WPAD_EXP_GUITARHERO3:
			if (right == 0)
			{
				mag = data->exp.nunchuk.js.mag;
				ang = data->exp.nunchuk.js.ang;
			}
			break;

		case WPAD_EXP_CLASSIC:
			if (right == 0)
			{
				mag = data->exp.classic.ljs.mag;
				ang = data->exp.classic.ljs.ang;
			}
			else
			{
				mag = data->exp.classic.rjs.mag;
				ang = data->exp.classic.rjs.ang;
			}
			break;

		default:
			break;
	}

	/* calculate X value (angle need to be converted into radian) */
	if (mag > 1.0) mag = 1.0;
	else if (mag < -1.0) mag = -1.0;
	double val = mag * sin((PI * ang)/180.0f);

	return (s8)(val * 128.0f);
}

s8 WPAD_StickY(u8 chan, u8 right)
{
	float mag = 0.0;
	float ang = 0.0;
	WPADData *data = WPAD_Data(chan);

	switch (data->exp.type)
	{
		case WPAD_EXP_NUNCHUK:
		case WPAD_EXP_GUITARHERO3:
			if (right == 0)
			{
				mag = data->exp.nunchuk.js.mag;
				ang = data->exp.nunchuk.js.ang;
			}
			break;

			case WPAD_EXP_CLASSIC:
			if (right == 0)
			{
				mag = data->exp.classic.ljs.mag;
				ang = data->exp.classic.ljs.ang;
			}
			else
			{
				mag = data->exp.classic.rjs.mag;
				ang = data->exp.classic.rjs.ang;
			}
			break;

		default:
			break;
	}

	/* calculate X value (angle need to be converted into radian) */
	if (mag > 1.0) mag = 1.0;
	else if (mag < -1.0) mag = -1.0;
	double val = mag * cos((PI * ang)/180.0f);

	return (s8)(val * 128.0f);
}
#endif

// hold zapper cursor positions
static int pos_x = 0;
static int pos_y = 0;

static void UpdateCursorPosition (int pad)
{
	#define ZAPPERPADCAL 20

	// gc left joystick
	signed char pad_x = PAD_StickX (pad);
	signed char pad_y = PAD_StickY (pad);

	if (pad_x > ZAPPERPADCAL){
		pos_x += (pad_x*1.0)/ZAPPERPADCAL;
		if (pos_x > 256) pos_x = 256;
	}
	if (pad_x < -ZAPPERPADCAL){
		pos_x -= (pad_x*-1.0)/ZAPPERPADCAL;
		if (pos_x < 0) pos_x = 0;
	}

	if (pad_y < -ZAPPERPADCAL){
		pos_y += (pad_y*-1.0)/ZAPPERPADCAL;
		if (pos_y > 224) pos_y = 224;
	}
	if (pad_y > ZAPPERPADCAL){
		pos_y -= (pad_y*1.0)/ZAPPERPADCAL;
		if (pos_y < 0) pos_y = 0;
	}

#ifdef HW_RVL
	struct ir_t ir;		// wiimote ir
	WPAD_IR(pad, &ir);
	if (ir.valid)
	{
		pos_x = (ir.x * 256) / 640;
		pos_y = (ir.y * 224) / 480;
	}
	else
	{
		signed char wm_ax = WPAD_StickX (pad, 0);
		signed char wm_ay = WPAD_StickY (pad, 0);

		if (wm_ax > ZAPPERPADCAL){
			pos_x += (wm_ax*1.0)/ZAPPERPADCAL;
			if (pos_x > 256) pos_x = 256;
		}
		if (wm_ax < -ZAPPERPADCAL){
			pos_x -= (wm_ax*-1.0)/ZAPPERPADCAL;
			if (pos_x < 0) pos_x = 0;
		}

		if (wm_ay < -ZAPPERPADCAL){
			pos_y += (wm_ay*-1.0)/ZAPPERPADCAL;
			if (pos_y > 224) pos_y = 224;
		}
		if (wm_ay > ZAPPERPADCAL){
			pos_y -= (wm_ay*1.0)/ZAPPERPADCAL;
			if (pos_y < 0) pos_y = 0;
		}
	}
#endif
}

/****************************************************************************
 * Convert GC Joystick Readings to JOY
 ****************************************************************************/
static int RAPID_SKIP = 2; // frames to skip between rapid button presses
static int RAPID_PRESS = 2; // number of rapid button presses to execute
static int rapidbutton[4][2] = {{0}};

static unsigned char DecodeJoy( unsigned short pad )
{
	signed char pad_x = PAD_StickX (pad);
	signed char pad_y = PAD_StickY (pad);
	u32 jp = PAD_ButtonsHeld (pad);
	unsigned char J = 0;

	#ifdef HW_RVL
	signed char wm_ax = 0;
	signed char wm_ay = 0;
	u32 wp = 0;
	wm_ax = WPAD_StickX ((u8)pad, 0);
	wm_ay = WPAD_StickY ((u8)pad, 0);
	wp = WPAD_ButtonsHeld (pad);

	u32 exp_type;
	if ( WPAD_Probe(pad, &exp_type) != 0 ) exp_type = WPAD_EXP_NONE;
	#endif

	/***
	Gamecube Joystick input
	***/
	// Is XY inside the "zone"?
	if (pad_x * pad_x + pad_y * pad_y > PADCAL * PADCAL)
	{
		if (pad_x > 0 && pad_y == 0) J |= JOY_RIGHT;
		if (pad_x < 0 && pad_y == 0) J |= JOY_LEFT;
		if (pad_x == 0 && pad_y > 0) J |= JOY_UP;
		if (pad_x == 0 && pad_y < 0) J |= JOY_DOWN;

		if (pad_x != 0 && pad_y != 0)
		{
			if ((float)pad_y / pad_x >= -2.41421356237 && (float)pad_y / pad_x < 2.41421356237)
			{
				if (pad_x >= 0)
					J |= JOY_RIGHT;
				else
					J |= JOY_LEFT;
			}

			if ((float)pad_x / pad_y >= -2.41421356237 && (float)pad_x / pad_y < 2.41421356237)
			{
				if (pad_y >= 0)
					J |= JOY_UP;
				else
					J |= JOY_DOWN;
			}
		}
	}
#ifdef HW_RVL
	/***
	Wii Joystick (classic, nunchuk) input
	***/
	// Is XY inside the "zone"?
	if (wm_ax * wm_ax + wm_ay * wm_ay > PADCAL * PADCAL)
	{
		/*** we don't want division by zero ***/
		if (wm_ax > 0 && wm_ay == 0)
			J |= JOY_RIGHT;
		if (wm_ax < 0 && wm_ay == 0)
			J |= JOY_LEFT;
		if (wm_ax == 0 && wm_ay > 0)
			J |= JOY_UP;
		if (wm_ax == 0 && wm_ay < 0)
			J |= JOY_DOWN;

		if (wm_ax != 0 && wm_ay != 0)
		{

			/*** Recalc left / right ***/
			float t;

			t = (float) wm_ay / wm_ax;
			if (t >= -2.41421356237 && t < 2.41421356237)
			{
				if (wm_ax >= 0)
					J |= JOY_RIGHT;
				else
					J |= JOY_LEFT;
			}

			/*** Recalc up / down ***/
			t = (float) wm_ax / wm_ay;
			if (t >= -2.41421356237 && t < 2.41421356237)
			{
				if (wm_ay >= 0)
					J |= JOY_UP;
				else
					J |= JOY_DOWN;
			}
		}
	}
#endif

	// Report pressed buttons (gamepads)
	int i;
	for (i = 0; i < MAXJP; i++)
	{
		if ( (jp & btnmap[CTRL_PAD][CTRLR_GCPAD][i])											// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_PAD][CTRLR_WIIMOTE][i]) )	// wiimote
		|| ( (exp_type == WPAD_EXP_CLASSIC) && (wp & btnmap[CTRL_PAD][CTRLR_CLASSIC][i]) )	// classic controller
		|| ( (exp_type == WPAD_EXP_NUNCHUK) && (wp & btnmap[CTRL_PAD][CTRLR_NUNCHUK][i]) )	// nunchuk + wiimote
		#endif
		)
		{
			// if zapper is on, ignore all buttons except START and SELECT
			if(GCSettings.Controller != CTRL_ZAPPER || nespadmap[i] == JOY_START || nespadmap[i] == JOY_SELECT)
			{
				if(nespadmap[i] == RAPID_A)
				{
					// activate rapid fire for A button
					rapidbutton[pad][0] = RAPID_PRESS;
				}
				else if(nespadmap[i] == RAPID_B)
				{
					// activate rapid fire for B button
					rapidbutton[pad][1] = RAPID_PRESS;
				}
				else if(nespadmap[i] > 0)
				{
					J |= nespadmap[i];
				}
				else
				{
					if(nesGameType == 4) // FDS
					{
						/* the commands shouldn't be issued in parallel so
						 * we'll delay them so the virtual FDS has a chance
						 * to process them
						 */
						FDSSwitchRequested = 1;
					}
					else
						FCEUI_VSUniCoin(); // insert coin for VS Games
				}
			}
		}
	}

	// rapid fire buttons
	if(FrameTimer % RAPID_SKIP == 0) // only press button every X frames
	{
		if(rapidbutton[pad][0] > 0) // rapid A
		{
			J |= JOY_A;
			rapidbutton[pad][0]--;
		}
		if(rapidbutton[pad][1] > 0) // rapid B
		{
			J |= JOY_B;
			rapidbutton[pad][1]--;
		}
	}

	// zapper enabled
	if(GCSettings.Controller == CTRL_ZAPPER)
	{
		int z = 1; // NES port # (0 or 1)

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
		}

		// VS zapper games
		if ( (jp & btnmap[CTRL_ZAPPER][CTRLR_GCPAD][1])	// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_ZAPPER][CTRLR_WIIMOTE][1]) )	// wiimote
		#endif
		)
		{
			FCEUI_VSUniCoin(); // insert coin for VS zapper games
		}

		// cursor position
		UpdateCursorPosition(0); // update cursor for wiimote 1
		myzappers[z][0] = pos_x;
		myzappers[z][1] = pos_y;

		// Report changes to FCE Ultra
		zapperdata[z]->Update(z,myzappers[z],0);
	}

	return J;
}

bool MenuRequested()
{
	for(int i=0; i<4; i++)
	{
		if (
			(userInput[i].pad.substickX < -70) ||
			(userInput[i].pad.btns_h & PAD_BUTTON_START &&
			userInput[i].pad.btns_h & PAD_BUTTON_A &&
			userInput[i].pad.btns_h & PAD_BUTTON_B &&
			userInput[i].pad.btns_h & PAD_TRIGGER_Z
			 ) ||
			(userInput[i].wpad.btns_h & WPAD_BUTTON_HOME) ||
			(userInput[i].wpad.btns_h & WPAD_CLASSIC_BUTTON_HOME)
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
	if(userInput[0].pad.substickX > 70 || userInput[0].WPAD_Stick(1,0) > 70)
		frameskip = 3;
	else
		frameskip = 0;

	// request to go back to menu
	if(MenuRequested())
		ScreenshotRequested = 1; // go to the menu

	for (i = 0; i < 4; i++)
		pad[i] = DecodeJoy(i);

	JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;
}
