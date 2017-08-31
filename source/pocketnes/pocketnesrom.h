/* pocketnesrom.h - functions to find uncompressed NES ROM images
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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char name[32];
	uint32_t filesize;
	uint32_t flags;
	uint32_t spritefollow;
	uint32_t reserved;
} pocketnes_romheader;

/* Finds the first PocketNES ROM header in the given data block by looking for
the segment 4E45531A (N,E,S,^Z) in the ROM itself. If no valid data is found,
this method will return NULL. */
const pocketnes_romheader* pocketnes_first_rom(const void* data, size_t length);

/* Returns a pointer to the next PocketNES ROM header in the data. If the
location where the next ROM header would be does not contain a 'N,E,S,^Z'
segment at the start of the ROM, this method will return NULL. */
const pocketnes_romheader* pocketnes_next_rom(const void* data, size_t length, const pocketnes_romheader* first_rom);

/* Returns true if the given data region looks like a PocketNES ROM header
(based on the 'N,E,S,^Z' segment), or false otherwise. */
int pocketnes_is_romheader(const void* data);

/* Returns the checksum that PocketNES would use for this ROM.
You can pass the PocketNES ROM header or the NES ROM itself. */
uint32_t pocketnes_get_checksum(const void* rom);

#ifdef __cplusplus
}
#endif
