/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 * eke-eke October 2008
 *
 * gcaudio.h
 *
 * Audio driver
 ****************************************************************************/

void InitialiseAudio();
void ResetAudio();
void PlaySound( int *Buffer, int samples );
void SwitchAudioMode(int mode);
void ShutdownAudio();
