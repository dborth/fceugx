/****************************************************************************
 * Gamecube Input
 *
 * Use JOY1 and JOY2
 ****************************************************************************/

#include <gccore.h>
#include "../../driver.h"
#include "../../fceu.h"

/* PADStatus joypads[4]; */
static uint32 JSReturn = 0;
unsigned short skipa[4] = {0, 0, 0, 0};
unsigned short skipb[4] = {0, 0, 0, 0};

unsigned short op[4] = {0, 0, 0, 0};

extern int ConfigScreen();

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

unsigned short gcpadmap[] = { PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_START, PAD_TRIGGER_Z, PAD_BUTTON_X, PAD_BUTTON_Y,
				PAD_BUTTON_UP, PAD_BUTTON_DOWN, PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT };

unsigned int nespadmap[] = { JOY_A, JOY_B, JOY_START, JOY_SELECT, JOY_A, JOY_B,
			      JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT };

/****************************************************************************
 * Convert GC Joystick Readings to JOY
 ****************************************************************************/
int PADTUR = 2;

unsigned char DecodeJoy( unsigned short pp )
{
	unsigned short p = PAD_ButtonsHeld(pp);

	unsigned char J = 0;

	int i;

	if ((skipa[pp] == 0) || ((op[pp] & gcpadmap[4]) == 0)) {
		nespadmap[4] = JOY_A;
		skipa[pp] = PADTUR;
	}

	if ((skipb[pp] == 0) || ((op[pp] & gcpadmap[5]) == 0))  {
		nespadmap[5] = JOY_B;
		skipb[pp] = PADTUR;
	}

	for (i = 0; i < 10; i++) {
		if (p & gcpadmap[i])
			J |= nespadmap[i];
	}

	if (skipa[pp] > 0){
		nespadmap[4] = 0;
		skipa[pp]--;
	}

	if (skipb[pp] > 0){
		nespadmap[5] = 0;
		skipb[pp]--;
	}

	op[pp] = p;

	return J;
}

/****************************************************************************
 * V 1.0.1
 * 
 * Additional check for Analog X/Y
 ****************************************************************************/
int PADCAL = 70;

unsigned char GetAnalog(int Joy)
{

	signed char x, y;
	unsigned char i = 0;

	x = PAD_StickX(Joy);
	y = PAD_StickY(Joy);

	if (x * x + y * y > PADCAL * PADCAL) {

		if (x > 0 && y == 0) return JOY_RIGHT;
		if (x < 0 && y == 0) return JOY_LEFT;
		if (x == 0 && y > 0) return JOY_UP;
		if (x == 0 && y < 0) return JOY_DOWN;

		if ((float)y / x >= -2.41421356237 && (float)y / x < 2.41421356237) {
			if (x >= 0)
				i |= JOY_RIGHT;
			else
				i |= JOY_LEFT;
		}

		if ((float)x / y >= -2.41421356237 && (float)x / y < 2.41421356237) {
			if (y >= 0)
				i |= JOY_UP;
			else
				i |= JOY_DOWN;
		}

	}

	return i;
}

int GetJoy()
{
	unsigned char pad[4];
	short i;
	int t = 0;

	void (*PSOReload)() = (void(*)())0x80001800;

	/*** Before checking anything else, look for PSOReload ***/
	if ( PAD_ButtonsHeld(0) == ( PAD_BUTTON_B | PAD_BUTTON_X | PAD_BUTTON_START ) )
		PSOReload();

	/*** Look for config menu ***/
	signed char px;
	px = PAD_SubStickX (0); 
	if (((px < -70)) || (PAD_ButtonsHeld(0) == ( PAD_TRIGGER_L | PAD_TRIGGER_R ))) {
		t = ConfigScreen();
		if (t == 1) {
			 return 1;
		}
	}
	for (i = 0; i < 4; i++)
		pad[i] = DecodeJoy(i) | GetAnalog(i);
	

	JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;

	return 0;
}

