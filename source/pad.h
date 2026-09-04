/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * pad.h
 *
 * Controller input
 ****************************************************************************/

#ifndef _PAD_H_
#define _PAD_H_

#include "fceugx.h"
#include "drivers/InputData.h"

#define MAXJP 			11

extern int playerMapping[4];
extern uint32_t btnmap[CTRL_BTN_MAPPINGS][INPUT_HW_MAX][MAXJP];

void SetControllers();
void ResetControls(int cc = -1, int wc = -1);
void GetJoy();
bool isMenuRequested();

#endif
