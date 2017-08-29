/* pocketnesrom.c - functions to find uncompressed NES ROM images
stored within PocketNES ROMs

Copyright (C) 2016 libertyernie

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

When compiling in Visual Studio, set the project to compile
as C++ code (Properties -> C/C++ -> Advanced -> Compile As.)
*/

#include <stdlib.h>
#include <string.h>
#include "pocketnesrom.h"

const char NES_WORD[4] = { 'N','E','S',0x1A };

/* Finds the first PocketNES ROM header in the given data block by looking for
the segment 4E45531A (N,E,S,^Z) in the ROM itself. If no valid data is found,
this method will return NULL. */
const pocketnes_romheader* pocketnes_first_rom(const void* data, size_t length) {
	const char* ptr = (const char*)data;
	const char* end = ptr + length;
	int logo_pos = 0;
	while (ptr < end) {
		if (*ptr == NES_WORD[logo_pos]) {
			// match
			logo_pos++;
			if (logo_pos == 4) { // matched all of GB logo - on last byte (0x133)
				// check if length fits
				const pocketnes_romheader* candidate = (pocketnes_romheader*)(ptr - 3 - sizeof(pocketnes_romheader));
				size_t filesize = candidate->filesize;
				if (*(uint16_t *)"\0\xff" < 0x100) {
					uint32_t buffer;
					((char*)&buffer)[0] = ((char*)&filesize)[3];
					((char*)&buffer)[1] = ((char*)&filesize)[2];
					((char*)&buffer)[2] = ((char*)&filesize)[1];
					((char*)&buffer)[3] = ((char*)&filesize)[0];
					filesize = buffer;
				}
				const char* candidate_end_ptr = (ptr - 3) + filesize;
				if (candidate_end_ptr <= end) {
					return candidate;
				} else {
					// no match, try again
					logo_pos = 0;
				}
			}
		}
		else {
			// no match, try again
			if (logo_pos > 0) {
				ptr -= logo_pos;
				logo_pos = 0;
			}
		}
		ptr++;
	}
	return NULL;
}

/* Returns a pointer to the next PocketNES ROM header in the data. If the
location where the next ROM header would be does not contain a 'N,E,S,^Z'
segment at the start of the ROM, this method will return NULL. */
const pocketnes_romheader* pocketnes_next_rom(const void* data, size_t length, const pocketnes_romheader* first_rom) {
	size_t diff = (const char*)first_rom - (const char*)data;
	if (diff > length) {
		return NULL;
	}
	size_t effective_length = length - diff;
	if (effective_length <= 0x200) {
		// Assume there will never be a ROM this small
		return NULL;
	}
	return pocketnes_first_rom(
		(const char*)first_rom + 4 + sizeof(pocketnes_romheader),
		effective_length - 4 - sizeof(pocketnes_romheader));
}

/* Returns true if the given data region looks like a PocketNES ROM header
(based on the 'N,E,S,^Z' segment), or false otherwise. */
int pocketnes_is_romheader(const void* data) {
	const char* rom_ptr = (const char*)data + sizeof(pocketnes_romheader);
	return memcmp(rom_ptr, NES_WORD, 4) == 0;
}

/* Returns the checksum that PocketNES would use for this ROM.
You can pass the PocketNES ROM header or the NES ROM itself. */
uint32_t pocketnes_get_checksum(const void* rom) {
	const uint8_t* p = (const uint8_t*)rom;

	// Checksum should not include NES ROM format header
	if (memcmp(p, NES_WORD, 4) == 0) {
		p += 16;
	}

	// TODO: add support for compressed roms

	uint32_t sum = 0;
	int i;
	for (i = 0; i<128; i++) {
		sum += *p | (*(p + 1) << 8) | (*(p + 2) << 16) | (*(p + 3) << 24);
		p += 128;
	}
	return sum;
}
