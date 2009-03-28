/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * pad.h
 *
 * Controller input
 ****************************************************************************/

#ifndef _PAD_H_
#define _PAD_H_

#include <gccore.h>
#include <wiiuse/wpad.h>

#define PI 				3.14159265f
#define PADCAL			50
#define MAXJP 			11
#define RAPID_A 		256
#define RAPID_B			512

enum
{
	TRIGGER_SIMPLE,
	TRIGGER_BUTTON_ONLY,
	TRIGGER_BUTTON_ONLY_IN_FOCUS
};

typedef struct _paddata {
	u16 btns_d;
	u16 btns_u;
	u16 btns_h;
	s8 stickX;
	s8 stickY;
	s8 substickX;
	s8 substickY;
	u8 triggerL;
	u8 triggerR;
} PADData;

class GuiTrigger
{
	public:
		GuiTrigger();
		~GuiTrigger();
		void SetSimpleTrigger(s32 ch, u32 wiibtns, u16 gcbtns);
		void SetButtonOnlyTrigger(s32 ch, u32 wiibtns, u16 gcbtns);
		void SetButtonOnlyInFocusTrigger(s32 ch, u32 wiibtns, u16 gcbtns);
		s8 WPAD_Stick(u8 right, int axis);
		bool Left();
		bool Right();
		bool Up();
		bool Down();

		u8 type;
		s32 chan;
		WPADData wpad;
		PADData pad;
};

extern GuiTrigger userInput[4];
extern int rumbleRequest[4];
extern u32 btnmap[2][4][12];

void ResetControls();
void ShutoffRumble();
void DoRumble(int i);
s8 WPAD_StickX(u8 chan,u8 right);
s8 WPAD_StickY(u8 chan, u8 right);
void InitialisePads();
void GetJoy();
void ToggleFourScore(int set);
void ToggleZapper(int set);
void DrawCursor();

#endif
