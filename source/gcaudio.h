/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 * eke-eke October 2008
 *
 * gcaudio.h
 *
 * Audio driver
 ****************************************************************************/

#ifndef _GCAUDIO_H_
#define _GCAUDIO_H_

#include "fceultra/types.h"

void InitialiseAudio();
void ResetAudio();
void PlaySound( int32 *Buffer, int samples );
void SwitchAudioMode(int mode);
void ShutdownAudio();
void UpdateSampleRate(int rate);
void SetSampleRate();

#endif
