/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * filter.h
 *
 * Filters Header File
 ****************************************************************************/
#ifndef _FILTER_H_
#define _FILTER_H_

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef intptr_t pint;

enum RenderFilter {
	FILTER_NONE = 0,

	FILTER_HQ2X,
	FILTER_HQ2XS,
	FILTER_HQ2XBOLD,

	NUM_FILTERS
};

typedef void (*TFilterMethod)(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);

extern TFilterMethod FilterMethod;

extern unsigned char filtermem[];

//
// Prototypes
//
void SelectFilterMethod ();
void RenderPlain (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);
TFilterMethod FilterToMethod (RenderFilter filterID);
const char* GetFilterName (RenderFilter filterID);
int GetFilterScale(RenderFilter filterID);
template<int GuiScale> void RenderHQ2X (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);
void InitLUTs();

#endif

