/// \file
/// \brief Sound filtering code

#include "types.h"

#include "sound.h"
#include "x6502.h"
#include "fceu.h"
#include "filter.h"

#include "fcoeffs.h"

#include <cmath>
#include <cstdio>

static int32 sq2coeffs[SQ2NCOEFFS];
static int32 coeffs[NCOEFFS];

static uint32 mrindex;
static uint32 mrratio;

// Output filter state, kept across calls (see SexyFilterReset)
static int64 sexyAcc1=0,sexyAcc2=0;
static int64 sexy2Acc=0;

/// Clears the output filters' history. The high-pass in SexyFilter remembers
/// the average level of everything it has filtered, so a new game that starts
/// silent would otherwise open with the previous game's level decaying away
/// as a low thump. Call when the sound hardware is powered on.
void SexyFilterReset(void)
{
 sexyAcc1=sexyAcc2=0;
 sexy2Acc=0;
}

void SexyFilter2(int32 *in, int32 count)
{
 #ifdef moo
 static int64 acc=0;
 double x,p;
 int64 c;

 x=2*M_PI*6000/FSettings.SndRate;
 p=((double)2-cos(x)) - sqrt(pow((double)2-cos(x),2) -1 );

 c=p*0x100000;
 //printf("%f\n",(double)c/0x100000);
 #endif
 while(count--)
 {
  int64 dropcurrent;
  dropcurrent=( int32(uint32(*in)<<16)-sexy2Acc)>>3;

  sexy2Acc+=dropcurrent;
  *in=sexy2Acc>>16;
  in++;
  //acc=((int64)0x100000-c)* *in + ((c*acc)>>20);
  //*in=acc>>20;
  //in++;
 }
}

void SexyFilter(int32 *in, int32 *out, int32 count)
{
 int32 mul1,mul2,vmul;

 mul1=(94<<16)/FSettings.SndRate;
 mul2=(24<<16)/FSettings.SndRate;
 vmul=(FSettings.SoundVolume<<16)*3/4/100;

 //FCEU_DispMessage("SoundVolume %d, vmul %d",0,FSettings.SoundVolume,vmul);
 if(FSettings.soundq) vmul/=4;
 else vmul*=2;			/* TODO:  Increase volume in low quality sound rendering code itself */

 while(count)
 {
  int64 ino=(int64)*in*vmul;
  sexyAcc1+=((ino-sexyAcc1)*mul1)>>16;
  sexyAcc2+=((ino-sexyAcc1-sexyAcc2)*mul2)>>16;
  //printf("%d ",*in);
  *in=0;
  {
   int32 t=(sexyAcc1-ino+sexyAcc2)>>16;
   //if(t>32767 || t<-32768) printf("Flow: %d\n",t);
   if(t>32767) t=32767;
   if(t<-32768) t=-32768;
   *out=t;
  }
  in++;
  out++;
  count--;
 }
}

/* Returns number of samples written to out. */
/* leftover is set to the number of samples that need to be copied
   from the end of in to the beginning of in.
*/

//static uint32 mva=1000;

/* This filtering code assumes that almost all input values stay below 32767.
   Do not adjust the volume in the wlookup tables and the expansion sound
   code to be higher, or you *might* overflow the FIR code.
*/

/* Symmetric FIR evaluated at two adjacent input positions (for the linear
   interpolation between them).

   The coefficient table is symmetric (coeffs[k] == coeffs[n-1-k]), so taps k
   and n-1-k share a coefficient and their input samples can be added first,
   halving the multiplies. Neighbouring iterations also share input samples, so
   those are carried in registers instead of reloaded. The result differs from
   the straightforward per-tap sum by at most 1 LSB of output (the >>6 rounding
   is applied to each pair instead of each tap).

   'base' is the first input sample of the window, n must be even.
   A product may be up to twice as large as before: inputs must stay below ~44000
   (the existing "stay below 32767" rule still applies, see above). */
static inline void FIRPair(const int32 *base, const int32 *coef, uint32 n, int32 &accOut, int32 &acc2Out)
{
	int32 acc = 0, acc2 = 0;
	int32 h1 = base[n + 1];   // base[n-k+1], k = 0
	int32 l0 = base[1];       // base[k+1],   k = 0
	const int32 *hi = base + n;     // base[n-k]
	const int32 *lo = base + 2;     // base[k+2]
	for (uint32 c = n >> 1; c; c--)
	{
		const int32 d = *coef++;
		const int32 h0 = *hi--;
		const int32 l1 = *lo++;
		acc  += ((h0 + l0) * d) >> 6;
		acc2 += ((h1 + l1) * d) >> 6;
		h1 = h0;
		l0 = l1;
	}
	accOut = acc;
	acc2Out = acc2;
}

/* ---- Sparse FIR evaluation --------------------------------------------------
   The signal fed to the filter is the sum of a few channels that each change
   only at counter reloads, so it is piecewise constant with long runs. The
   filter is linear, so a step of size d at position pos in the window adds
   d * (sum of the coefficients that see the step) to the output. With the prefix
   sums of the coefficients (firPrefix[m] = sum of the first m coefficients),
   one output costs one multiply per value change inside the window instead of
   one per tap:

     F(b) = in[b] * firPrefix[n]  +  sum over changes b < pos <= b+n of
                                     delta(pos) * firPrefix[b+n+1-pos]

   which equals sum_k coef[k] * in[b+n-k]. The sums are exact (64-bit), shifted
   down by 6 once at the end, where the direct sum rounds every tap. The two
   agree to well under 1 LSB of output. Frames with too many changes
   (e.g. an ultrasonic triangle that changes on every cycle) fall back to the
   direct FIR, so the worst case costs what it did before. */
#define MAX_FIR_EVENTS 8192

static uint32 firEvPos[MAX_FIR_EVENTS];    // index of each change
static int32 firEvDelta[MAX_FIR_EVENTS];   // new value minus previous value
static int32 firPrefix[SQ2NCOEFFS+2];      // [m] = coef[0]+..+coef[m-1] (exact), [n+1] = 0

/* Lists the value changes in in[1..inlen-1]. Returns -1 if there are more than
   MAX_FIR_EVENTS (the direct FIR is used then). */
static int FIRBuildEvents(const int32 *in, uint32 inlen)
{
	int n=0;
	int32 prev=in[0];
	for(uint32 i=1;i<inlen;i++)
	{
		const int32 v=in[i];
		if(v!=prev)
		{
			if(n==MAX_FIR_EVENTS) return -1;
			firEvPos[n]=i;
			firEvDelta[n]=(int32)((uint32)v-(uint32)prev);
			n++;
			prev=v;
		}
	}
	return n;
}

/* Same result as FIRPair for the window starting at 'in[b]', n taps.
   'ei' is the first change after the window start of the previous call (windows
   only move forward within one NeoFilterSound call). */
static inline void FIRSparse(const int32 *in, uint32 b, uint32 n, int nev, int &ei, int32 &accOut, int32 &acc2Out)
{
	while(ei<nev && firEvPos[ei]<=b) ei++;

	const int64 total=firPrefix[n];
	int64 a=(int64)in[b]*total;
	int64 a2=(int64)in[b+1]*total;
	const uint32 lim=b+n+1;
	for(int j=ei;j<nev && firEvPos[j]<=lim;j++)
	{
		const uint32 r=lim-firEvPos[j];
		const int64 d=firEvDelta[j];
		a+=d*firPrefix[r];
		a2+=d*firPrefix[r+1];
	}
	accOut=(int32)(a>>6);
	acc2Out=(int32)(a2>>6);
}

int32 NeoFilterSound(int32 *in, int32 *out, uint32 inlen, int32 *leftover)
{
	uint32 x;
	uint32 max;
	int32 *outsave=out;
	int32 count=0;

//	for(x=0;x<inlen;x++)
//	{
//	 if(in[x]>mva){ mva=in[x]; printf("%ld\n",in[x]);}
//	}
        max=(inlen-1)<<16;

	{
		const uint32 nco=(FSettings.soundq==2)?SQ2NCOEFFS:NCOEFFS;
		const int32 *coef=(FSettings.soundq==2)?sq2coeffs:coeffs;
		const int nev=FIRBuildEvents(in,inlen);
		int ei=0;

		for(x=mrindex;x<max;x+=mrratio)
		{
			int32 acc,acc2;

			if(nev>=0)
				FIRSparse(in,(x>>16)-nco,nco,nev,ei,acc,acc2);
			else
				FIRPair(&in[(x>>16)-nco],coef,nco,acc,acc2);

			acc=((int64)acc*(65536-(x&65535))+(int64)acc2*(x&65535))>>(16+11);
			*out=acc;
			out++;
			count++;
		}
	}

	mrindex=x-max;

	if(FSettings.soundq==2)
	{
         mrindex+=SQ2NCOEFFS*65536;
         *leftover=SQ2NCOEFFS+1;
	}
	else
	{
         mrindex+=NCOEFFS*65536;
         *leftover=NCOEFFS+1;
	}

	if(GameExpSound.NeoFill)
	 GameExpSound.NeoFill(outsave,count);

	SexyFilter(outsave,outsave,count);
	if(FSettings.lowpass)
	 SexyFilter2(outsave,count);
	return(count);
}

void MakeFilters(int32 rate)
{
 const int32 *tabs[8]={C44100NTSC,C44100PAL,C48000NTSC,C48000PAL,C96000NTSC,
        C96000PAL, nullptr, nullptr};
 const int32 *sq2tabs[8]={SQ2C44100NTSC,SQ2C44100PAL,SQ2C48000NTSC,SQ2C48000PAL,
	SQ2C96000NTSC,SQ2C96000PAL, nullptr, nullptr};

 const int32 *tmp;
 int32 x;
 uint32 nco;

 if(FSettings.soundq==2)
  nco=SQ2NCOEFFS;
 else
  nco=NCOEFFS;

 mrindex=(nco+1)<<16;
 mrratio=(PAL?(int64)(PAL_CPU*65536):(int64)(NTSC_CPU*65536))/rate;

 if(FSettings.soundq==2)
  tmp=sq2tabs[(PAL?1:0)|(rate==48000?2:0)|(rate==96000?4:0)];
 else
  tmp=tabs[(PAL?1:0)|(rate==48000?2:0)|(rate==96000?4:0)];

 if(FSettings.soundq==2)
  for(x=0;x<SQ2NCOEFFS>>1;x++)
   sq2coeffs[x]=sq2coeffs[SQ2NCOEFFS-1-x]=tmp[x];
 else
  for(x=0;x<NCOEFFS>>1;x++)
   coeffs[x]=coeffs[NCOEFFS-1-x]=tmp[x];

 /* Prefix sums for the sparse FIR (see FIRSparse) */
 {
  const int32 *c=(FSettings.soundq==2)?sq2coeffs:coeffs;
  int64 sum=0;
  firPrefix[0]=0;
  for(x=0;x<(int32)nco;x++)
  {
   sum+=c[x];
   firPrefix[x+1]=(int32)sum;
  }
  firPrefix[nco+1]=0;
 }

 #ifdef MOO
 /* Some tests involving precision and error. */
 {
  static int64 acc=0;
  int x;
  for(x=0;x<SQ2NCOEFFS;x++)
   acc+=(int64)32767*sq2coeffs[x];
  printf("Foo: %lld\n",acc);
 }
 #endif
}
