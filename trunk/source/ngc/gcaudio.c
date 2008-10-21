/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 * eke-eke October 2008
 *
 * gcaudio.c
 *
 * Audio driver
 ****************************************************************************/

#include <gccore.h>
#include <string.h>

#define SAMPLERATE 48000

static u8 ConfigRequested = 0;
static u8 soundbuffer[2][3840] ATTRIBUTE_ALIGN(32);
static u8 mixbuffer[16000];
static int mixhead = 0;
static int mixtail = 0;
static int whichab = 0;
int IsPlaying = 0;

static int mixercollect( u8 *outbuffer, int len )
{
	u32 *dst = (u32 *)outbuffer;
	u32 *src = (u32 *)mixbuffer;
	int done = 0;

	// Always clear output buffer
	memset(outbuffer, 0, len);

	while ( ( mixtail != mixhead ) && ( done < len ) )
	{
		*dst++ = src[mixtail++];
		if (mixtail == 4000) mixtail = 0;
		done += 4;
	}

	// Realign to 32 bytes for DMA
	mixtail -= ((done&0x1f) >> 2);
	if (mixtail < 0)
		mixtail += 4000;
	done &= ~0x1f;
	if (!done)
		return len >> 1;

	return done;
}

/****************************************************************************
 * AudioSwitchBuffers
 *
 * Manages which buffer is played next
 ****************************************************************************/
void AudioSwitchBuffers()
{
	if ( !ConfigRequested )
	{
		int len = mixercollect( soundbuffer[whichab], 3840 );
		DCFlushRange(soundbuffer[whichab], len);
		AUDIO_InitDMA((u32)soundbuffer[whichab], len);
		AUDIO_StartDMA();
		whichab ^= 1;
		IsPlaying = 1;
	}
	else IsPlaying = 0;
}

void InitialiseSound()
{
	AUDIO_Init(NULL); // Start audio subsystem
	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	AUDIO_RegisterDMACallback( AudioSwitchBuffers );
	memset(soundbuffer, 0, 3840*2);
	memset(mixbuffer, 0, 16000);
}

void StopAudio()
{
	AUDIO_StopDMA();
	ConfigRequested = 1;
	IsPlaying = 0;
}

/****************************************************************************
 * Audio incoming is monaural
 *
 * PlaySound will simply mix to get it right
 ****************************************************************************/
void PlaySound( int *Buffer, int count )
{
	int i;
	u16 sample;
	u32 *dst = (u32 *)mixbuffer;

	for( i = 0; i < count; i++ )
	{
		sample = Buffer[i] & 0xffff;
		dst[mixhead++] = sample | ( sample << 16);
		if (mixhead == 4000)
			mixhead = 0;
	}

	// Restart Sound Processing if stopped
	if (IsPlaying == 0)
	{
		ConfigRequested = 0;
		AudioSwitchBuffers ();
	}
}
