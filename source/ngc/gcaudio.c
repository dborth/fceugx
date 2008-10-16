/****************************************************************************
 * FCE Ultra 0.98.12
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * audio.c
 *
 * Audio driver
 ****************************************************************************/

#include <gccore.h>
#define SAMPLERATE 48000
static unsigned char audiobuffer[2][64 * 1024] __attribute__((__aligned__(32)));
/*** Allow for up to 1 full second ***/

/****************************************************************************
 * AudioSwitchBuffers
 *
 * Manages which buffer is played next
 ****************************************************************************/
static int isWriting = 0;	/*** Bool for buffer writes ***/
static int buffSize[2];		/*** Hold size of current buffer ***/
static int whichab = 0;		/*** Which Audio Buffer is in use ***/
static int isPlaying;		/*** Is Playing ***/
static void AudioSwitchBuffers()
{
    if ( buffSize[whichab] ) {
        AUDIO_StopDMA();
        AUDIO_InitDMA((u32)audiobuffer[whichab], buffSize[whichab]);
        DCFlushRange(&audiobuffer[whichab], buffSize[whichab]);
        AUDIO_StartDMA();
        isPlaying = 0;
    }

    whichab ^= 1;
    buffSize[whichab] = 0;
}

void InitialiseSound()
{
    AUDIO_Init(NULL);	/*** Start audio subsystem ***/
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
    AUDIO_RegisterDMACallback( AudioSwitchBuffers );

    buffSize[0] = buffSize[1] = 0;
}

void StartAudio()
{
    AUDIO_StartDMA();
}

void StopAudio()
{
    AUDIO_StopDMA();
}

static inline unsigned short FLIP16(unsigned short b)
{
    return((b<<8)|((b>>8)&0xFF));
}

static inline u32 FLIP32(u32 b)
{
    return( (b<<24) | ((b>>8)&0xFF00) | ((b<<8)&0xFF0000) | ((b>>24)&0xFF) );
}

/****************************************************************************
 * Audio incoming is monaural
 *
 * PlaySound will simply mix to get it right
 ****************************************************************************/
#define AUDIOBUFFER ((50 * SAMPLERATE) / 1000 ) << 4
static int isPlaying = 0;
static short MBuffer[ 8 * 96000 / 50 ];

void PlaySound( unsigned int *Buffer, int count )
{
    int P;
    unsigned short *s = (unsigned short *)&MBuffer[0];
    unsigned int *d = (unsigned int *)&audiobuffer[whichab][buffSize[whichab]];
    unsigned int c;
    int ms;

    isWriting = 1;

    for ( P = 0; P < count; P++ ) {
        MBuffer[P] = Buffer[P];
    }

    /*** Now do Mono - Stereo Conversion ***/
    ms = count;
    do
    {
        c = 0xffff & *s++;
        *d++ = (c | (c<<16));
    } while(--ms);

    buffSize[whichab] += ( count << 2 );
    /*** This is the kicker for the entire audio loop ***/
    if ( isPlaying == 0 )
    {
        if ( buffSize[whichab] > AUDIOBUFFER )
        {
            isPlaying = 1;
            AudioSwitchBuffers();
        }
    }

    isWriting = 0;

}
