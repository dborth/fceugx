/*****************************************************************************
 * Gamecube XenoGC Identifier
 *
 * Functions to determine if the DVD is in fact a XenoGC boot disc
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int IsXenoGCImage( char *buffer )
{

	/*** All Xeno GC Homebrew Boot have id GBLPGL ***/
	if ( memcmp( buffer, "GBLPGL", 6 ) )
		return 0;

	if ( memcmp( &buffer[0x20], "GAMECUBE \"EL TORITO\" BOOTLOADER", 31 ) )
		return 0;

	return 1;
}

