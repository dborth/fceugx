/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 *
 * pad.cpp
 *
 * Controller input
 ****************************************************************************/

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "pad.h"
#include "menu.h"
#include "gcvideo.h"
#include "filebrowser.h"
#include "button_mapping.h"
#include "fceuload.h"
#include "libgui/Gui.h"
#include "drivers/ogc/wiidrc.h"

#define ANALOG_SENSITIVITY 30

int playerMapping[4] = {0,1,2,3};

static uint32 JSReturn = 0;
void *InputDPR;

static INPUTC *zapperdata[2];
static unsigned int myzappers[2][3];

uint32_t nespadmap[11]; // Original NES controller buttons
uint32_t zapperpadmap[11]; // Original NES Zapper controller buttons
uint32_t btnmap[2][6][12]; // button mapping

void ResetControls(int consoleCtrl, int hardwareProfile)
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
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_GAMECUBE))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_TRIGGER_ZL; // GC Z button
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_PLUS;   // GC Start
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_GAMECUBE][i++] = GUI_TRIGGER_L;
	}

	/*** Wiimote Padmap (Sideways NES mapping) ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_1;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_2;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_WIIMOTE][i++] = GUI_BTN_A;
	}

	/*** Classic Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_CLASSIC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_CLASSIC][i++] = GUI_TRIGGER_L;
	}

	/*** Wii U Pro Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_WUPC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_WUPC][i++] = GUI_TRIGGER_L;
	}

	/*** Wii U Gamepad (DRC) Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_DRC))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_A;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_B;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_X;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_Y;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_TRIGGER_L;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_TRIGGER_R;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_DRC][i++] = GUI_BTN_RIGHT;
	}

	/*** Nunchuk + Wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && hardwareProfile == GUI_HW_NUNCHUK))
	{
		i=0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_TRIGGER_L;  // Nunchuk C mapped to L
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_TRIGGER_ZL; // Nunchuk Z mapped to ZL
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = 0;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_MINUS;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_PLUS;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_UP;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_DOWN;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_LEFT;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_RIGHT;
		btnmap[CTRL_PAD][GUI_HW_NUNCHUK][i++] = GUI_BTN_A;
	}

	/*** Zapper : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && hardwareProfile == GUI_HW_GAMECUBE))
	{
		i=0;
		btnmap[CTRL_ZAPPER][GUI_HW_GAMECUBE][i++] = GUI_BTN_A; // shoot
		btnmap[CTRL_ZAPPER][GUI_HW_GAMECUBE][i++] = GUI_BTN_B; // insert coin
	}

	/*** Zapper : Wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_ZAPPER && hardwareProfile == GUI_HW_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_ZAPPER][GUI_HW_WIIMOTE][i++] = GUI_BTN_B; // shoot
		btnmap[CTRL_ZAPPER][GUI_HW_WIIMOTE][i++] = GUI_BTN_A; // insert coin
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

static inline float clampf(float v, float lo, float hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/****************************************************************************
 * Hardware Mapping Helpers
 * Translates raw libogc hardware bits to our generic UI masks
 ***************************************************************************/
static uint32_t MapPADToGeneric(uint32_t pad_btns)
{
	uint32_t mask = GUI_BTN_NONE;
	if (pad_btns & PAD_BUTTON_A)      mask |= GUI_BTN_A;
	if (pad_btns & PAD_BUTTON_B)      mask |= GUI_BTN_B;
	if (pad_btns & PAD_BUTTON_X)      mask |= GUI_BTN_X;
	if (pad_btns & PAD_BUTTON_Y)      mask |= GUI_BTN_Y;
	if (pad_btns & PAD_BUTTON_UP)     mask |= GUI_BTN_UP;
	if (pad_btns & PAD_BUTTON_DOWN)   mask |= GUI_BTN_DOWN;
	if (pad_btns & PAD_BUTTON_LEFT)   mask |= GUI_BTN_LEFT;
	if (pad_btns & PAD_BUTTON_RIGHT)  mask |= GUI_BTN_RIGHT;
	if (pad_btns & PAD_BUTTON_START)  mask |= GUI_BTN_PLUS;
	if (pad_btns & PAD_TRIGGER_L)     mask |= GUI_TRIGGER_L;
	if (pad_btns & PAD_TRIGGER_R)     mask |= GUI_TRIGGER_R;
	if (pad_btns & PAD_TRIGGER_Z)     mask |= GUI_TRIGGER_ZR;
	return mask;
}

#ifdef HW_RVL
static uint32_t MapWiimoteToGeneric(uint32_t wpad_btns)
{
	uint32_t mask = GUI_BTN_NONE;

	if (wpad_btns & WPAD_BUTTON_A)     mask |= GUI_BTN_A;
	if (wpad_btns & WPAD_BUTTON_B)     mask |= GUI_BTN_B;
	if (wpad_btns & WPAD_BUTTON_1)     mask |= GUI_BTN_1;
	if (wpad_btns & WPAD_BUTTON_2)     mask |= GUI_BTN_2;
	if (wpad_btns & WPAD_BUTTON_UP)    mask |= GUI_BTN_UP;
	if (wpad_btns & WPAD_BUTTON_DOWN)  mask |= GUI_BTN_DOWN;
	if (wpad_btns & WPAD_BUTTON_LEFT)  mask |= GUI_BTN_LEFT;
	if (wpad_btns & WPAD_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (wpad_btns & WPAD_BUTTON_PLUS)  mask |= GUI_BTN_PLUS;
	if (wpad_btns & WPAD_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (wpad_btns & WPAD_BUTTON_HOME)  mask |= GUI_BTN_HOME;

	return mask;
}

static uint32_t MapNunchukToGeneric(uint32_t wpad_btns)
{
	uint32_t mask = GUI_BTN_NONE;

	if (wpad_btns & WPAD_NUNCHUK_BUTTON_Z) mask |= GUI_TRIGGER_ZL;
	if (wpad_btns & WPAD_NUNCHUK_BUTTON_C) mask |= GUI_TRIGGER_L;

	return mask;
}

static uint32_t MapClassicToGeneric(uint32_t wpad_btns)
{
	uint32_t mask = GUI_BTN_NONE;

	// Classic Controller inputs (upper 16 bits)
	if (wpad_btns & WPAD_CLASSIC_BUTTON_A) mask |= GUI_BTN_A;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_B) mask |= GUI_BTN_B;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_X) mask |= GUI_BTN_X;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_Y) mask |= GUI_BTN_Y;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_UP) mask |= GUI_BTN_UP;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_FULL_L) mask |= GUI_TRIGGER_L;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_FULL_R) mask |= GUI_TRIGGER_R;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_ZL) mask |= GUI_TRIGGER_ZL;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_ZR) mask |= GUI_TRIGGER_ZR;

	return mask;
}

static uint32_t MapWiiUGamepadToGeneric(uint32_t drc_btns)
{
	uint32_t mask = GUI_BTN_NONE;
	if (drc_btns & WIIDRC_BUTTON_A) mask |= GUI_BTN_A;
	if (drc_btns & WIIDRC_BUTTON_B) mask |= GUI_BTN_B;
	if (drc_btns & WIIDRC_BUTTON_X) mask |= GUI_BTN_X;
	if (drc_btns & WIIDRC_BUTTON_Y) mask |= GUI_BTN_Y;
	if (drc_btns & WIIDRC_BUTTON_UP) mask |= GUI_BTN_UP;
	if (drc_btns & WIIDRC_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (drc_btns & WIIDRC_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (drc_btns & WIIDRC_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (drc_btns & WIIDRC_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (drc_btns & WIIDRC_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (drc_btns & WIIDRC_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (drc_btns & WIIDRC_BUTTON_L) mask |= GUI_TRIGGER_L;
	if (drc_btns & WIIDRC_BUTTON_R) mask |= GUI_TRIGGER_R;
	if (drc_btns & WIIDRC_BUTTON_ZL) mask |= GUI_TRIGGER_ZL;
	if (drc_btns & WIIDRC_BUTTON_ZR) mask |= GUI_TRIGGER_ZR;
	return mask;
}

/****************************************************************************
 * Analog Normalization Helpers
 ***************************************************************************/
static float NormalizeWPADAnalog(int pos, int min, int max, int center)
{
	if (min == max) return 0.0f;

	// Handle broken 3rd party controller calibration data
	if ((min >= center) || (max <= center)) {
		min = 0; max = 64; center = 32; // Generic fallback
	}

	int offset = pos - center;
	if (offset > 0) {
		return clampf((float)offset / (float)(max - center), 0.0f, 1.0f);
	} else {
		return clampf((float)offset / (float)(center - min), -1.0f, 0.0f);
	}
}
#endif

/****************************************************************************
 * UpdatePads
 * Scans all controllers, combines states, and updates controllers
 ***************************************************************************/
void UpdatePads()
{
	#ifdef HW_RVL
	WiiDRC_ScanPads();
	WPAD_ScanPads();
	#endif

	uint32_t activeGamecubePads = PAD_ScanPads();

	float deltaTime = 1.0f / 60.0f;

	for(int i = 3; i >= 0; i--) {
		GuiInputPadData padData;

		// Process GameCube Controller & Third Party USB Adaptors
		bool gamecubeActive = (activeGamecubePads & (1 << i)) != 0;

		if(gamecubeActive) {
			padData.hw_connected[GUI_HW_GAMECUBE] = true;
			padData.hw_buttons_d[GUI_HW_GAMECUBE] = MapPADToGeneric(PAD_ButtonsDown(i));
			padData.hw_buttons_h[GUI_HW_GAMECUBE] = MapPADToGeneric(PAD_ButtonsHeld(i));
			padData.hw_buttons_r[GUI_HW_GAMECUBE] = MapPADToGeneric(PAD_ButtonsUp(i));
			padData.hw_stickX[GUI_HW_GAMECUBE] = clampf((float)PAD_StickX(i) / 128.0f, -1.0f, 1.0f);
			padData.hw_stickY[GUI_HW_GAMECUBE] = clampf((float)PAD_StickY(i) / 128.0f, -1.0f, 1.0f);
			padData.hw_substickX[GUI_HW_GAMECUBE] = clampf((float)PAD_SubStickX(i) / 128.0f, -1.0f, 1.0f);
			padData.hw_substickY[GUI_HW_GAMECUBE] = clampf((float)PAD_SubStickY(i) / 128.0f, -1.0f, 1.0f);
		}

#ifdef HW_RVL
		// Process Wiimote and Extensions
		uint32_t exp_type = WPAD_EXP_NONE;

		if (WPAD_Probe(i, &exp_type) == WPAD_ERR_NONE) {
			WPADData* wpad = WPAD_Data(i);

			// Always process base Wiimote
			padData.hw_connected[GUI_HW_WIIMOTE] = true;
			padData.battery_level = wpad->battery_level;

			if (exp_type == WPAD_EXP_NONE) {
				padData.hw_buttons_d[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_d);
				padData.hw_buttons_h[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_h);
				padData.hw_buttons_r[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_u);

				if (wpad->ir.valid) {
					padData.validPointer = true;
					padData.isTouch = false;
					padData.cursor_x = wpad->ir.x;
					padData.cursor_y = wpad->ir.y;
					padData.cursor_angle = wpad->ir.angle;
				}

				userInput[i]->setSideways(fabs(wpad->gforce.x) > fabs(wpad->gforce.y));
			}
			else if (exp_type == WPAD_EXP_NUNCHUK) {
				padData.hw_buttons_d[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_d);
				padData.hw_buttons_h[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_h);
				padData.hw_buttons_r[GUI_HW_WIIMOTE] = MapWiimoteToGeneric(wpad->btns_u);

				padData.hw_connected[GUI_HW_NUNCHUK] = true;
				padData.hw_buttons_d[GUI_HW_NUNCHUK] = MapNunchukToGeneric(wpad->btns_d);
				padData.hw_buttons_h[GUI_HW_NUNCHUK] = MapNunchukToGeneric(wpad->btns_h);
				padData.hw_buttons_r[GUI_HW_NUNCHUK] = MapNunchukToGeneric(wpad->btns_u);
				joystick_t* js = &wpad->exp.nunchuk.js;
				padData.hw_stickX[GUI_HW_NUNCHUK] = NormalizeWPADAnalog(js->pos.x, js->min.x, js->max.x, js->center.x);
				padData.hw_stickY[GUI_HW_NUNCHUK] = NormalizeWPADAnalog(js->pos.y, js->min.y, js->max.y, js->center.y);
				userInput[i]->setSideways(false);
			}
			else if (exp_type == WPAD_EXP_CLASSIC) {
				bool isWUPC = (wpad->exp.classic.type == 2);
				int hw = isWUPC ? GUI_HW_WUPC : GUI_HW_CLASSIC;

				padData.hw_connected[hw] = true;
				padData.hw_buttons_d[hw] = MapClassicToGeneric(wpad->btns_d);
				padData.hw_buttons_h[hw] = MapClassicToGeneric(wpad->btns_h);
				padData.hw_buttons_r[hw] = MapClassicToGeneric(wpad->btns_u);

				joystick_t* ljs = &wpad->exp.classic.ljs;
				joystick_t* rjs = &wpad->exp.classic.rjs;
				padData.hw_stickX[hw] = NormalizeWPADAnalog(ljs->pos.x, ljs->min.x, ljs->max.x, ljs->center.x);
				padData.hw_stickY[hw] = NormalizeWPADAnalog(ljs->pos.y, ljs->min.y, ljs->max.y, ljs->center.y);
				padData.hw_substickX[hw] = NormalizeWPADAnalog(rjs->pos.x, rjs->min.x, rjs->max.x, rjs->center.x);
				padData.hw_substickY[hw] = NormalizeWPADAnalog(rjs->pos.y, rjs->min.y, rjs->max.y, rjs->center.y);
				userInput[i]->setSideways(false);
			}
			else {
				userInput[i]->setSideways(false);
			}
		}

		// Process Wii U Gamepad
		if(i == 0 && WiiDRC_Inited() && WiiDRC_Connected()) {
			padData.hw_connected[GUI_HW_DRC] = true;
			padData.hw_buttons_d[GUI_HW_DRC] = MapWiiUGamepadToGeneric(WiiDRC_ButtonsDown());
			padData.hw_buttons_h[GUI_HW_DRC] = MapWiiUGamepadToGeneric(WiiDRC_ButtonsHeld());
			padData.hw_buttons_r[GUI_HW_DRC] = MapWiiUGamepadToGeneric(WiiDRC_ButtonsUp());
			padData.hw_stickX[GUI_HW_DRC] = clampf((float)WiiDRC_lStickX() / 128.0f, -1.0f, 1.0f);
			padData.hw_stickY[GUI_HW_DRC] = clampf((float)WiiDRC_lStickY() / 128.0f, -1.0f, 1.0f);
			padData.hw_substickX[GUI_HW_DRC] = clampf((float)WiiDRC_rStickX() / 128.0f, -1.0f, 1.0f);
			padData.hw_substickY[GUI_HW_DRC] = clampf((float)WiiDRC_rStickY() / 128.0f, -1.0f, 1.0f);
		}
		#endif

		// Merge into unified aggregate state for UI Elements
		for (uint32_t hw = 0; hw < GUI_HW_MAX; hw++) {
			if (!padData.hw_connected[hw])
				continue;

			padData.buttons_d |= padData.hw_buttons_d[hw];
			padData.buttons_h |= padData.hw_buttons_h[hw];
			padData.buttons_r |= padData.hw_buttons_r[hw];

			if (std::abs(padData.hw_stickX[hw]) > std::abs(padData.stickX)) padData.stickX = padData.hw_stickX[hw];
			if (std::abs(padData.hw_stickY[hw]) > std::abs(padData.stickY)) padData.stickY = padData.hw_stickY[hw];
			if (std::abs(padData.hw_substickX[hw]) > std::abs(padData.substickX)) padData.substickX = padData.hw_substickX[hw];
			if (std::abs(padData.hw_substickY[hw]) > std::abs(padData.substickY)) padData.substickY = padData.hw_substickY[hw];
		}

		// Push the finalized, merged payload to the controller abstraction
		userInput[i]->update(padData, deltaTime);
	}
}

// hold zapper cursor positions
static int pos_x = 0;
static int pos_y = 0;

static void UpdateCursorPosition(int chan)
{
	if (!userInput[chan]) return;
	const GuiInputPadData& pad = userInput[chan]->getPadData();

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
	if (!userInput[chan]) return 0;
	const GuiInputPadData& pad = userInput[chan]->getPadData();
	unsigned char J = 0;

	// 1. Unified Stick to D-Pad translation
	float sensitivity = (float)ANALOG_SENSITIVITY / 128.0f;

	if (pad.stickY > sensitivity) J |= JOY_UP;
	else if (pad.stickY < -sensitivity) J |= JOY_DOWN;
	if (pad.stickX < -sensitivity) J |= JOY_LEFT;
	else if (pad.stickX > sensitivity) J |= JOY_RIGHT;

	bool zapper_triggered = false;

	// 2. Zapper Logic
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

		if (userInput[1] && userInput[1]->getPadData().validPointer) {
			zapperChan = 1;
		}

		UpdateCursorPosition(zapperChan);
		myzappers[z][0] = pos_x;
		myzappers[z][1] = pos_y;

		if (zapperdata[z]) {
			zapperdata[z]->Update(z, myzappers[z], 0);
		}
	}

	// 3. Standard Gamepad Logic
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

		if (button_pressed && !zapper_triggered)
		{
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
		if (!userInput[i]) continue;
		const GuiInputPadData& pad = userInput[i]->getPadData();

		bool rightStickLeft = (pad.substickX < -0.55f);
		bool homePressed = (pad.buttons_h & GUI_BTN_HOME);

		bool lPlusRPlusStart = (pad.buttons_h & GUI_TRIGGER_L) &&
							   (pad.buttons_h & GUI_TRIGGER_R) &&
							   (pad.buttons_h & GUI_BTN_PLUS);

		bool oneTwoPlus = (pad.buttons_h & GUI_BTN_1) &&
						  (pad.buttons_h & GUI_BTN_2) &&
						  (pad.buttons_h & GUI_BTN_PLUS);

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
	if (!userInput[0]) return false;
	const GuiInputPadData& pad = userInput[0]->getPadData();

	switch(GCSettings.TurboModeButton)
	{
		case TURBO_BUTTON_RSTICK:
			return (pad.substickX > 0.55f);
		case TURBO_BUTTON_A:
			return (pad.buttons_h & GUI_BTN_A);
		case TURBO_BUTTON_B:
			return (pad.buttons_h & GUI_BTN_B);
		case TURBO_BUTTON_X:
			return (pad.buttons_h & GUI_BTN_X);
		case TURBO_BUTTON_Y:
			return (pad.buttons_h & GUI_BTN_Y);
		case TURBO_BUTTON_L:
			return (pad.buttons_h & GUI_TRIGGER_L);
		case TURBO_BUTTON_R:
			return (pad.buttons_h & GUI_TRIGGER_R);
		case TURBO_BUTTON_ZL:
			return (pad.buttons_h & GUI_TRIGGER_ZL);
		case TURBO_BUTTON_ZR:
			return (pad.buttons_h & GUI_TRIGGER_ZR);
		case TURBO_BUTTON_Z: // Fallback generic Z trigger
			return (pad.buttons_h & GUI_TRIGGER_ZL);
		case TURBO_BUTTON_C: // Fallback generic C trigger
			return (pad.buttons_h & GUI_TRIGGER_L);
		case TURBO_BUTTON_1:
			return (pad.buttons_h & GUI_BTN_1);
		case TURBO_BUTTON_2:
			return (pad.buttons_h & GUI_BTN_2);
		case TURBO_BUTTON_PLUS:
			return (pad.buttons_h & GUI_BTN_PLUS);
		case TURBO_BUTTON_MINUS:
			return (pad.buttons_h & GUI_BTN_MINUS);
		default:
			return false;
	}
}

void GetJoy()
{
	JSReturn = 0; // reset buttons pressed
	unsigned char pad[4];
	short i;

	UpdatePads();

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
