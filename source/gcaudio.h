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

void InitialiseAudio();
void ResetAudio();
void PlaySound( int32 *Buffer, int samples );
void SwitchAudioMode(int mode);
void ShutdownAudio();
void UpdateSampleRate(int rate);
void SetSampleRate();
