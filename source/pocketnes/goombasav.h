/* goombasav.h - functions to handle Goomba / Goomba Color SRAM

Copyright (C) 2014-2017 libertyernie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

https://github.com/libertyernie/goombasav

When compiling in Visual Studio, set all goombasav files to compile
as C++ code (Properties -> C/C++ -> Advanced -> Compile As.)
*/

#ifndef __GOOMBASAV_H
#define __GOOMBASAV_H

#include <stdint.h>
#define GOOMBA_COLOR_SRAM_SIZE 65536
#define GOOMBA_COLOR_AVAILABLE_SIZE 57344
#define GOOMBA_STATEID 0x57a731dc
#define POCKETNES_STATEID 0x57a731d7
#define POCKETNES_STATEID2 0x57a731d8
#define SMSADVANCE_STATEID 0x57a731dc
#define GOOMBA_STATESAVE 0
#define GOOMBA_SRAMSAVE 1
#define GOOMBA_CONFIGSAVE 2
#define GOOMBA_PALETTE 5

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t goomba_size_t; // want a consistent size for printf. This is an alias for uint32_t, but this makes it clear that we're counting the size of something.

/* 16-bit and 32-bit values in the stateheader are stored as little endian
(native to the GBA's ARM chip as well as x86 processors.) Use this function
after getting or before setting a value if your code might run on a big-endian
processor (e.g. PowerPC.) */
uint16_t little_endian_conv_16(uint16_t value);

/* 16-bit and 32-bit values in the stateheader are stored as little endian
(native to the GBA's ARM chip as well as x86 processors.) Use this function
after getting or before setting a value if your code might run on a big-endian
processor (e.g. PowerPC.) */
uint32_t little_endian_conv_32(uint32_t value);

typedef struct {
	uint16_t size;
	uint16_t type;	//=CONFIGSAVE
} configdata;

typedef struct {
	uint16_t size;  //=48 (0x30)
	uint16_t type;	//=CONFIGSAVE
	char bordercolor;
	char palettebank;
	char misc;
	char reserved3;
	uint32_t sram_checksum;	//checksum of rom using SRAM e000-ffff	
	uint32_t zero;	//=0
	char reserved4[32];  //="CFG"
} goomba_configdata;

typedef struct {
	uint16_t size;  //=48 (0x30)
	uint16_t type;	//=CONFIGSAVE
	char displaytype;
	char misc;
	char reserved2;
	char reserved3;
	uint32_t sram_checksum;	//checksum of rom using SRAM e000-ffff	
	uint32_t zero;	//=0
	char reserved4[32];  //="CFG"
} pocketnes_configdata;

typedef struct {
	uint16_t size;  //=52 (0x34)
	uint16_t type;	//=CONFIGSAVE
	char displaytype;
	char gammavalue;
	char region;			// bit 0 & 1 = region.
	char sleepflick;
	char config;
	char bcolor;
	char reserved1;
	char reserved2;
	uint32_t sram_checksum;	//checksum of rom using SRAM e000-ffff	
	uint32_t zero;	//=0
	char reserved3[32];	//="CFG"
} smsadvance_configdata;

typedef struct {
	uint16_t size;	//header+data
	uint16_t type;	//=STATESAVE or SRAMSAVE
	uint32_t uncompressed_size;
	uint32_t framecount;
	uint32_t checksum;
	char title[32];
} stateheader;

typedef struct {
	const char* sleep;
	const char* autoload_state;
	const char* gamma;
} configdata_misc_strings;

/**
* Returns the last error encountered by goombasav (for functions that return
* NULL on error.) This string is stored statically by goombasav and its
* contents may change over time.
*/
const char* goomba_last_error();

/**
* Set the string buffer used by goomba_last_error to a custom value. If the
* input string is too long, the resulting goomba_last_error string will be
* truncated.
* Returns the number of bytes copied to the buffer (at present, the maximum
* is 255.)
*/
size_t goomba_set_last_error(const char* msg);

/**
* Gets a struct containing pointers to three static strings (which do not
* need to be deallocated.)
*/
configdata_misc_strings configdata_get_misc(char misc);

/**
* Returns a multi-line description string that includes all parameters of the
* given stateheader or configdata structure.
* This string is stored in a static character buffer, and subsequent calls to
* stateheader_str or stateheader_summary_str will overwrite this buffer.
*/
const char* stateheader_str(const stateheader* sh);

/**
* Returns a one-line summary string displaying the size and title of the
* stateheader or configdata structure.
* This string is stored in a static character buffer, and subsequent calls to
* stateheader_str or stateheader_summary_str will overwrite this buffer.
*/
const char* stateheader_summary_str(const stateheader* sh);

int stateheader_plausible(const void* sh);

/**
* If a valid stateheader starts at the address given or 4 bytes later,
* returns a pointer to the stateheader. Otherwise, returns NULL.
*/
const stateheader* stateheader_first(const void* gba_data);

/**
* When given a pointer to a stateheader, returns a pointer to where the next
* stateheader will be located (if any). Use stateheader_plausible to
* determine if there really is a header at this location.
*
* If stateheader_plausible determines that the input is not a valid
* stateheader, NULL will be returned.
*/
const stateheader* stateheader_advance(const stateheader* sh);

/**
* Scans for valid stateheaders and allocates an array to store them. The array
* will have a capacity of 64, and any difference between that
* and the number of headers found will be filled with NULL entries. The last
* entry (array[63]) is guaranteed to be NULL.
* NOTE: the gba_data parameter can point to a valid header, or to a sequence
* equal to GOOMBA_STATEID immediately before a valid header.
*/
const stateheader** stateheader_scan(const void* gba_data);

/**
* Returns the stateheader in gba_data with the title field = gbc_title,
* or NULL if there is none. This function is intended for Game Boy (Color)
* ROMs, so only the first 15 bytes of gbc_title will be used in the
* comparison.
*/
const stateheader* stateheader_for(const void* gba_data, const char* gbc_title_ptr);

/**
* Returns the stateheader in gba_data for the given ROM checksum, or NULL if
* there is none.
*/
const stateheader* stateheader_for_checksum(const void* gba_data, uint32_t checksum);

/**
* Returns true if the given data starts with GOOMBA_STATEID (little endian.)
*/
int goomba_is_sram(const void* data);

/**
 * Returns the 32-bit checksum (unsigned) in the configdata header, or -1 if
 * an error occurred.
 */
int64_t goomba_get_configdata_checksum_field(const void* gba_data);

/**
* Computes a simple additive checksum of the compressed data that comes after
* the given header, using anywhere from 1 to 8 bytes.
*/
uint64_t goomba_compressed_data_checksum(const stateheader* sh, int output_bytes);

/**
* If there is save data in 0xe000-0xffff (as signaled by the configdata),
* this function compresses it to where it's supposed to go. In the event that
* the data passed in is already clean, the same pointer will be returned.
* NULL will be returned if an error occurs.
*
* The given pointer must be at least GOOMBA_COLOR_SRAM_SIZE bytes in length.
* If it is longer, any information after GOOMBA_COLOR_SRAM_SIZE bytes will be
* ignored.
*/
char* goomba_cleanup(const void* gba_data_param);

/**
* Allocates memory to store the uncompressed GB/GBC save file extracted from
* the Goomba Color save file stored in header_ptr, or returns NULL if the
* decompression failed.
*/
void* goomba_extract(const void* gba_data, const stateheader* header_ptr, goomba_size_t* size_output);

/**
* Copies data from gba_data to a new buffer allocated with malloc, replacing
* the data of the section pointed to by gba_header with a compressed version
* of the data pointed to by gbc_sram. If gbc_length is 0, the section pointed
* to by gba_header will be removed instead of replaced.
*/
char* goomba_new_sav(const void* gba_data, const void* gba_header, const void* gbc_sram, goomba_size_t gbc_length);

#ifdef __cplusplus
}
#endif
#endif
