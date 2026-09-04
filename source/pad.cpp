/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * pad.cpp
 *
 * Controller input
 ****************************************************************************/

#include <gccore.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "menu.h"
#include "gcvideo.h"
#include "filebrowser.h"
#include "button_mapping.h"
#include "fceuload.h"
#include "libgui/Gui.h"

#define ANALOG_SENSITIVITY 30
#define RAPID_A 		256
#define RAPID_B			512

int playerMapping[4] = {0,1,2,3};

static uint32 JSReturn = 0;
void *InputDPR;

static INPUTC *zapperdata[2];
static unsigned int myzappers[2][3];

uint32_t nespadmap[MAXJP]; // Original NES controller buttons
uint32_t zapperpadmap[MAXJP]; // Original NES Zapper controller buttons
uint32_t btnmap[CTRL_BTN_MAPPINGS][GUI_HW_MAX][MAXJP]; // button mapping

void ResetControls(int consoleCtrl, int hardwareProfile)
{
	int i;

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
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_GAMECUBE))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_TRIGGER_ZL; // GC Z button
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_PLUS;   // GC Start
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = INPUT_TRIGGER_L;
	}

	/*** Wiimote Padmap (Sideways NES mapping) ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_1;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_2;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = INPUT_BTN_A;
	}

	/*** Classic Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_CLASSIC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = INPUT_TRIGGER_L;
	}

	/*** Wii U Pro Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_WUPC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = INPUT_TRIGGER_L;
	}

	/*** Wii U Gamepad (DRC) Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_DRC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = INPUT_TRIGGER_L;
	}

	/*** Nunchuk + Wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_NUNCHUK))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_TRIGGER_L;  // Nunchuk C mapped to L
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_TRIGGER_ZL; // Nunchuk Z mapped to ZL
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = INPUT_BTN_A;
	}

	/*** Zapper : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && hardwareProfile == GUI_HW_GAMECUBE))
	{
		btnmap[CTRL_ZAPPER][GUI_HW_GAMECUBE][0] = INPUT_BTN_A; // shoot
		btnmap[CTRL_ZAPPER][GUI_HW_GAMECUBE][1] = INPUT_BTN_B; // insert coin
	}

	/*** Zapper : Wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && hardwareProfile == GUI_HW_WIIMOTE))
	{
		btnmap[CTRL_ZAPPER][GUI_HW_WIIMOTE][0] = INPUT_BTN_B; // shoot
		btnmap[CTRL_ZAPPER][GUI_HW_WIIMOTE][1] = INPUT_BTN_A; // insert coin
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
	zapperdata[0]=nullptr;
	zapperdata[1]=nullptr;
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

// hold zapper cursor positions
static int pos_x = 0;
static int pos_y = 0;

static void UpdateCursorPosition(int chan)
{
	if (!controller[chan]) return;
	const InputPadData& pad = controller[chan]->getPadData();

	// If we have an active IR pointer, snap directly to coordinates
	if (pad.validPointer)
	{
		pos_x = (int)((pad.cursor_x * 256.0f) / 640.0f);
		pos_y = (int)((pad.cursor_y * 224.0f) / 480.0f);
	}
	else
	{
		// Convert unified analog stick to cursor movement
		float sensitivity = (float)ANALOG_SENSITIVITY / 128.0f;
		float stickX = pad.stickX;
		float stickY = pad.stickY;

		if (std::abs(stickX) > sensitivity) {
			pos_x += (int)(stickX * 6.4f);
		}

		if (std::abs(stickY) > sensitivity) {
			pos_y -= (int)(stickY * 6.4f); // y-axis is inverted visually
		}
	}

	// Clamp to virtual FCE Ultra NES bounds
	if (pos_x > 256) pos_x = 256;
	if (pos_x < 0) pos_x = 0;
	if (pos_y > 224) pos_y = 224;
	if (pos_y < 0) pos_y = 0;
}

/****************************************************************************
 * Convert GC Joystick Readings to JOY
 ****************************************************************************/

extern int rapidAlternator;

static unsigned char DecodeJoy(unsigned short chan)
{
	if (!controller[chan]) return 0;
	const InputPadData& pad = controller[chan]->getPadData();
	unsigned char J = 0;

	// Unified Stick to D-Pad translation
	float sensitivity = (float)ANALOG_SENSITIVITY / 128.0f;

	if (pad.stickY > sensitivity) J |= JOY_UP;
	else if (pad.stickY < -sensitivity) J |= JOY_DOWN;
	if (pad.stickX < -sensitivity) J |= JOY_LEFT;
	else if (pad.stickX > sensitivity) J |= JOY_RIGHT;

	bool zapper_triggered = false;

	// Zapper Logic
	if (GCSettings.Controller == CTRL_ZAPPER)
	{
		int z = (GameInfo->type == GIT_VSUNI) ? 0 : 1;
		myzappers[z][2] = 0; // reset trigger

		// Poll all active hardware types for Zapper controls
		for (uint32_t hw = 0; hw < GUI_HW_MAX; hw++)
		{
			if (!pad.hw_connected[hw]) continue;
			uint32_t hw_held = pad.hw_buttons_h[hw];

			if (hw_held & btnmap[CTRL_ZAPPER][hw][0]) {
				myzappers[z][2] |= 2; // Shoot
				zapper_triggered = true;
			}
			if (hw_held & btnmap[CTRL_ZAPPER][hw][1]) {
				FCEUI_VSUniCoin(); // Coin
				zapper_triggered = true;
			}
		}

		int zapperChan = 0;

		if (controller[1] && controller[1]->getPadData().validPointer) {
			zapperChan = 1;
		}

		UpdateCursorPosition(zapperChan);
		myzappers[z][0] = pos_x;
		myzappers[z][1] = pos_y;

		if (zapperdata[z]) {
			zapperdata[z]->Update(z, myzappers[z], 0);
		}
	}

	// Standard Gamepad Logic
	for (int i = 0; i < MAXJP; i++)
	{
		bool button_pressed = false;

		// Check if ANY connected hardware is triggering this specific NES mapping
		for (uint32_t hw = 0; hw < GUI_HW_MAX; hw++)
		{
			if (!pad.hw_connected[hw]) continue;
			if (pad.hw_buttons_h[hw] & btnmap[CTRL_PAD][hw][i]) {
				button_pressed = true;
				break;
			}
		}

		if (button_pressed && !zapper_triggered) {
			if (rapidAlternator && nespadmap[i] == RAPID_A) {
				J |= JOY_A;
			}
			else if (rapidAlternator && nespadmap[i] == RAPID_B) {
				J |= JOY_B;
			}
			else if (nespadmap[i] > 0) {
				J |= nespadmap[i];
			}
			else {
				if (GameInfo->type == GIT_FDS) {
					if (FDSSwitchRequested == 0) FDSSwitchRequested = 1;
				} else {
					FCEUI_VSUniCoin();
				}
			}
		}
	}

	return J;
}

bool isMenuRequested()
{
	for(int i=0; i<4; i++)
	{
		if (!controller[i]) continue;
		const InputPadData& pad = controller[i]->getPadData();

		bool rightStickLeft = (pad.substickX < -0.55f);
		bool homePressed = (pad.buttons_h & INPUT_BTN_HOME);

		bool lPlusRPlusStart = (pad.buttons_h & INPUT_TRIGGER_L) &&
							   (pad.buttons_h & INPUT_TRIGGER_R) &&
							   (pad.buttons_h & INPUT_BTN_PLUS);

		bool oneTwoPlus = (pad.buttons_h & INPUT_BTN_1) &&
						  (pad.buttons_h & INPUT_BTN_2) &&
						  (pad.buttons_h & INPUT_BTN_PLUS);

		if (GCSettings.GamepadMenuToggle == GAMEPAD_MENU_TOGGLE_HOME_RIGHTSTICK)
		{
			if (rightStickLeft || homePressed) return true;
		}
		else if (GCSettings.GamepadMenuToggle == GAMEPAD_MENU_TOGGLE_LRSTART_12PLUS)
		{
			if (lPlusRPlusStart || oneTwoPlus) return true;
		}
		else // All toggle options enabled
		{
			if (rightStickLeft || homePressed || lPlusRPlusStart || oneTwoPlus) return true;
		}
	}
	return false;
}

bool IsTurboModeInputPressed()
{
	if (!controller[0]) return false;
	const InputPadData& pad = controller[0]->getPadData();

	switch(GCSettings.TurboModeButton)
	{
		case TURBO_BUTTON_RSTICK:
			return (pad.substickX > 0.55f);
		case TURBO_BUTTON_A:
			return (pad.buttons_h & INPUT_BTN_A);
		case TURBO_BUTTON_B:
			return (pad.buttons_h & INPUT_BTN_B);
		case TURBO_BUTTON_X:
			return (pad.buttons_h & INPUT_BTN_X);
		case TURBO_BUTTON_Y:
			return (pad.buttons_h & INPUT_BTN_Y);
		case TURBO_BUTTON_L:
			return (pad.buttons_h & INPUT_TRIGGER_L);
		case TURBO_BUTTON_R:
			return (pad.buttons_h & INPUT_TRIGGER_R);
		case TURBO_BUTTON_ZL:
			return (pad.buttons_h & INPUT_TRIGGER_ZL);
		case TURBO_BUTTON_ZR:
			return (pad.buttons_h & INPUT_TRIGGER_ZR);
		case TURBO_BUTTON_Z: // Fallback generic Z trigger
			return (pad.buttons_h & INPUT_TRIGGER_ZL);
		case TURBO_BUTTON_C: // Fallback generic C trigger
			return (pad.buttons_h & INPUT_TRIGGER_L);
		case TURBO_BUTTON_1:
			return (pad.buttons_h & INPUT_BTN_1);
		case TURBO_BUTTON_2:
			return (pad.buttons_h & INPUT_BTN_2);
		case TURBO_BUTTON_PLUS:
			return (pad.buttons_h & INPUT_BTN_PLUS);
		case TURBO_BUTTON_MINUS:
			return (pad.buttons_h & INPUT_BTN_MINUS);
		default:
			return false;
	}
}

void GetJoy()
{
	JSReturn = 0; // reset buttons pressed
	unsigned char pad[4];
	short i;

	platform->getInput()->update();

	// Turbo mode
	// RIGHT on c-stick and on classic ctrlr right joystick
	if (GCSettings.TurboModeEnabled)
	{
		turbomode = IsTurboModeInputPressed();
	}
	// request to go back to menu
	if(isMenuRequested())
		MenuRequested = true; // go to the menu

	for (i = 0; i < 4; i++)
		pad[playerMapping[i]] = DecodeJoy(i);

	JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;
}
