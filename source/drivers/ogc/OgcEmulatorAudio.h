/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2023
 * eke-eke October 2008
 *
 * OgcEmulatorAudio.h
 *
 * Audio driver
 ****************************************************************************/
#pragma once

#include "fceultra/types.h"

// Hardware DMA callback that feeds the sound buffer ring to AUDIO_InitDMA
void AudioSwitchBuffers();

// Halts DMA playback so it cleanly restarts the next time samples arrive
void AudioStop();

void ResetAudio();
void PlaySound( int32 *Buffer, int samples );
void UpdateSampleRate(int rate);
void SetSampleRate();
