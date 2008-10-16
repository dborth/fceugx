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

#include "fceuconfig.h"
#include "pad.h"
#include "menu.h"
#include "fceustate.h"

// Original NES controller buttons
// All other pads are mapped to this
unsigned int nespadmap[] = {
	JOY_B, JOY_A,
	JOY_START, JOY_SELECT,
	JOY_UP, JOY_DOWN,
	JOY_LEFT, JOY_RIGHT
};

/*** Gamecube controller Padmap ***/
unsigned int gcpadmap[] = {
	PAD_BUTTON_B, PAD_BUTTON_A,
	PAD_TRIGGER_L, PAD_TRIGGER_R,
	PAD_BUTTON_UP, PAD_BUTTON_DOWN,
	PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT
};
/*** Wiimote Padmap ***/
unsigned int wmpadmap[] = {
	WPAD_BUTTON_1, WPAD_BUTTON_2,
	WPAD_BUTTON_MINUS, WPAD_BUTTON_PLUS,
	WPAD_BUTTON_RIGHT, WPAD_BUTTON_LEFT,
	WPAD_BUTTON_UP, WPAD_BUTTON_DOWN
};
/*** Classic Controller Padmap ***/
unsigned int ccpadmap[] = {
	WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_B,
	WPAD_CLASSIC_BUTTON_MINUS, WPAD_CLASSIC_BUTTON_PLUS,
	WPAD_CLASSIC_BUTTON_UP, WPAD_CLASSIC_BUTTON_DOWN,
	WPAD_CLASSIC_BUTTON_LEFT, WPAD_CLASSIC_BUTTON_RIGHT
};
/*** Nunchuk + wiimote Padmap ***/
unsigned int ncpadmap[] = {
	WPAD_NUNCHUK_BUTTON_C, WPAD_NUNCHUK_BUTTON_Z,
	WPAD_BUTTON_MINUS, WPAD_BUTTON_PLUS,
	WPAD_BUTTON_UP, WPAD_BUTTON_DOWN,
	WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT
};

static uint32 JSReturn = 0;

/****************************************************************************
 * Initialise Pads
 ****************************************************************************/
void InitialisePads()
{
    int attrib = 0;
    void *InputDPR;

    FCEUI_DisableFourScore(1);

    InputDPR = &JSReturn;
    FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, attrib);
    FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, attrib);
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

/****************************************************************************
 * Convert GC Joystick Readings to JOY
 ****************************************************************************/
u8 PADTUR = 2;

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

	/*** Report pressed buttons (gamepads) ***/
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
		J |= nespadmap[i];
	}
    return J;
}

void GetJoy()
{
    unsigned char pad[4];
    short i;

    s8 gc_px = PAD_SubStickX (0);
    u32 jp = PAD_ButtonsHeld (0); // gamecube controller button info

    #ifdef HW_RVL
    s8 wm_sx = WPAD_StickX (0,1);
    u32 wm_pb = WPAD_ButtonsHeld (0); // wiimote / expansion button info
    #endif

    // request to go back to menu
    if ((gc_px < -70) || (jp & PAD_BUTTON_START)
    #ifdef HW_RVL
    		 || (wm_pb & WPAD_BUTTON_HOME)
    		 || (wm_pb & WPAD_CLASSIC_BUTTON_HOME)
    		 || (wm_sx < -70)
    #endif
    )
	{
    	AUDIO_StopDMA();

    	if (GCSettings.AutoLoad == 1)
    		SaveState(GCSettings.SaveMethod, SILENT);

    	MainMenu(4);
	}
	else
	{
		for (i = 0; i < 4; i++)
			pad[i] = DecodeJoy(i);

		JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;
	}
}
