/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 * eke-eke October 2008
 *
 * gcaudio.h
 *
 * Audio driver
 ****************************************************************************/

void InitialiseAudio();
void StopAudio();
void ResetAudio();
void PlaySound( int *Buffer, int samples );
