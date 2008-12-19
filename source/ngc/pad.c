/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * pad.c
 *
 * Controller input
 ****************************************************************************/

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <math.h>
#include "driver.h"
#include "fceu.h"
#include "input.h"

#include "fceugx.h"
#include "pad.h"
#include "gcaudio.h"
#include "menu.h"
#include "gcvideo.h"
#include "filesel.h"

extern bool romLoaded;

static uint32 JSReturn = 0;
void *InputDPR;

INPUTC *zapperdata[2];
unsigned int myzappers[2][3];

extern INPUTC *FCEU_InitZapper(int w);

unsigned int nespadmap[11]; // Original NES controller buttons
unsigned int gcpadmap[11]; // Gamecube controller Padmap
unsigned int wmpadmap[11]; // Wiimote Padmap
unsigned int ccpadmap[11]; // Classic Controller Padmap
unsigned int ncpadmap[11]; // Nunchuk + wiimote Padmap

void ResetControls()
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
	i=0;
	gcpadmap[i++] = PAD_BUTTON_B;
	gcpadmap[i++] = PAD_BUTTON_A;
	gcpadmap[i++] = PAD_BUTTON_Y;
	gcpadmap[i++] = PAD_BUTTON_X;
	gcpadmap[i++] = PAD_TRIGGER_Z;
	gcpadmap[i++] = PAD_BUTTON_START;
	gcpadmap[i++] = PAD_BUTTON_UP;
	gcpadmap[i++] = PAD_BUTTON_DOWN;
	gcpadmap[i++] = PAD_BUTTON_LEFT;
	gcpadmap[i++] = PAD_BUTTON_RIGHT;
	gcpadmap[i++] = PAD_TRIGGER_L;

	/*** Wiimote Padmap ***/
	i=0;
	wmpadmap[i++] = WPAD_BUTTON_1;
	wmpadmap[i++] = WPAD_BUTTON_2;
	wmpadmap[i++] = 0;
	wmpadmap[i++] = 0;
	wmpadmap[i++] = WPAD_BUTTON_MINUS;
	wmpadmap[i++] = WPAD_BUTTON_PLUS;
	wmpadmap[i++] = WPAD_BUTTON_RIGHT;
	wmpadmap[i++] = WPAD_BUTTON_LEFT;
	wmpadmap[i++] = WPAD_BUTTON_UP;
	wmpadmap[i++] = WPAD_BUTTON_DOWN;
	wmpadmap[i++] = WPAD_BUTTON_A;

	/*** Classic Controller Padmap ***/
	i=0;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_Y;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_B;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_X;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_A;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_MINUS;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_PLUS;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_UP;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_DOWN;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_LEFT;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_RIGHT;
	ccpadmap[i++] = WPAD_CLASSIC_BUTTON_FULL_L;

	/*** Nunchuk + wiimote Padmap ***/
	i=0;
	ncpadmap[i++] = WPAD_NUNCHUK_BUTTON_C;
	ncpadmap[i++] = WPAD_NUNCHUK_BUTTON_Z;
	ncpadmap[i++] = 0;
	ncpadmap[i++] = 0;
	ncpadmap[i++] = WPAD_BUTTON_MINUS;
	ncpadmap[i++] = WPAD_BUTTON_PLUS;
	ncpadmap[i++] = WPAD_BUTTON_UP;
	ncpadmap[i++] = WPAD_BUTTON_DOWN;
	ncpadmap[i++] = WPAD_BUTTON_LEFT;
	ncpadmap[i++] = WPAD_BUTTON_RIGHT;
	ncpadmap[i++] = WPAD_BUTTON_A;
}

/****************************************************************************
 * Initialise Pads
 ****************************************************************************/
void InitialisePads()
{
	InputDPR = &JSReturn;
	FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, 0);
	FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, 0);

	ToggleFourScore(GCSettings.FourScore, true);
	ToggleZapper(GCSettings.zapper, true);
}

void ToggleFourScore(int set, bool loaded)
{
	if(loaded)
		FCEUI_DisableFourScore(set);
}

void ToggleZapper(int set, bool loaded)
{
	if(loaded)
	{
		// set defaults
		zapperdata[0]=NULL;
		zapperdata[1]=NULL;
		myzappers[0][0]=myzappers[1][0]=128;
		myzappers[0][1]=myzappers[1][1]=120;
		myzappers[0][2]=myzappers[1][2]=0;

		// Default ports back to gamepad
		FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, 0);
		FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, 0);

		if(set)
		{
			// enable Zapper
			zapperdata[set-1] = FCEU_InitZapper(set-1);
			FCEUI_SetInput(set-1, SI_ZAPPER, myzappers[set-1], 1);
		}
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

// hold zapper cursor positions
int pos_x = 0;
int pos_y = 0;

void UpdateCursorPosition (int pad)
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
int RAPID_SKIP = 2; // frames to skip between rapid button presses
int RAPID_PRESS = 2; // number of rapid button presses to execute
int rapidbutton[4][2] = {{0}};

unsigned char DecodeJoy( unsigned short pad )
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
		if ( (jp & gcpadmap[i])											// gamecube controller
		#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & wmpadmap[i]) )	// wiimote
		|| ( (exp_type == WPAD_EXP_CLASSIC) && (wp & ccpadmap[i]) )	// classic controller
		|| ( (exp_type == WPAD_EXP_NUNCHUK) && (wp & ncpadmap[i]) )	// nunchuk + wiimote
		#endif
		)
		{
			// if zapper is on, ignore all buttons except START and SELECT
			if(!GCSettings.zapper || nespadmap[i] == JOY_START || nespadmap[i] == JOY_SELECT)
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
	if(GCSettings.zapper)
	{
		int z = GCSettings.zapper-1; // NES port # (0 or 1)

		myzappers[z][2] = 0; // reset trigger to not pressed

		// is trigger pressed?
		if ( (jp & PAD_BUTTON_A) // gamecube controller
		#ifdef HW_RVL
		|| (wp & WPAD_BUTTON_A)	// wiimote
		|| (wp & WPAD_BUTTON_B)
		|| (wp & WPAD_CLASSIC_BUTTON_A) // classic controller
		#endif
		)
		{
			// report trigger press
			myzappers[z][2] |= 2;
		}

		// VS zapper games
		if ( (jp & PAD_BUTTON_B) // gamecube controller
		#ifdef HW_RVL
		|| (wp & WPAD_BUTTON_1)	// wiimote
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

void GetJoy()
{
    JSReturn = 0; // reset buttons pressed
	unsigned char pad[4];
    short i;

    s8 gc_px = PAD_SubStickX (0);
    s8 gc_py = PAD_SubStickY (0);
    u32 jp = PAD_ButtonsHeld (0); // gamecube controller button info

    #ifdef HW_RVL
    s8 wm_sx = WPAD_StickX (0,1);
    s8 wm_sy = WPAD_StickY (0,1);
    u32 wm_pb = WPAD_ButtonsHeld (0); // wiimote / expansion button info
    #endif

    // Turbo mode
    // RIGHT on c-stick and on classic ctrlr right joystick
    if(
    	(gc_px > 70)
		#ifdef HW_RVL
		|| (wm_sx > 70)
		#endif
	)
    {
    	frameskip = 3;
    }
    else
    {
    	frameskip = 0;
    }

    // Check for video zoom
	if (GCSettings.Zoom)
	{
		if (gc_py < -36 || gc_py > 36)
		zoom ((float) gc_py / -36);
		#ifdef HW_RVL
			if (wm_sy < -36 || wm_sy > 36)
			zoom ((float) wm_sy / -36);
		#endif
	}

    // request to go back to menu
    if ((gc_px < -70) || ((jp & PAD_BUTTON_START) && (jp & PAD_BUTTON_A))
    #ifdef HW_RVL
    		 || (wm_pb & WPAD_BUTTON_HOME)
    		 || (wm_pb & WPAD_CLASSIC_BUTTON_HOME)
    		 || (wm_sx < -70)
    #endif
    )
	{
    	StopAudio();
    	ConfigRequested = 1;
	}
	else
	{
		for (i = 0; i < 4; i++)
			pad[i] = DecodeJoy(i);

		JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;
	}
}
