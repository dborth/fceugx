/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * cheatmgr.cpp
 *
 * Cheat handling
 ***************************************************************************/

#include <malloc.h>
#include <stdint.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "fileop.h"
#include "filebrowser.h"
#include "menu/menu.h"
#include "drivers/Platform.h"
#include "drivers/FileSystemDriver.h"

#include "fceultra/cheat.h"

int numcheats = 0;

/****************************************************************************
 * LoadCheats
 *
 * Loads cheat file from save buffer
 * Custom version of FCEU_LoadGameCheats()
 ***************************************************************************/

static int LoadCheats ()
{
	unsigned int addr;
	unsigned int val;
	unsigned int status;
	unsigned int type;
	unsigned int compare;
	int x;

	char *namebuf;
	int tc=0;

	char * linebreak = strtok((char *)savebuffer, "\n");
	int line_num = 0;

	while(linebreak != nullptr && line_num < 500)
	{
		line_num++;
		char *tbuf=linebreak;
		int doc=0;

		addr=val=compare=status=type=0;

		if(tbuf[0]=='S')
		{
			tbuf++;
			type=1;
		}
		else type=0;

		if(tbuf[0]=='C')
		{
			tbuf++;
			doc=1;
		}

		if(tbuf[0]==':')
		{
			tbuf++;
			status=0;
		}
		else status=1;

		if(doc)
		{
			char *neo = &tbuf[4+2+2+1+1+1];
			if(sscanf(tbuf, "%04x%*[:]%02x%*[:]%02x", &addr, &val, &compare) != 3)
				continue;
			namebuf=(char *)malloc(strlen(neo)+1);
			strcpy(namebuf, neo);
		}
		else
		{
			char *neo = &tbuf[4+2+1+1];
			if(sscanf(tbuf, "%04x%*[:]%02x", &addr, &val) != 2)
				continue;
			namebuf=(char *)malloc(strlen(neo)+1);
			strcpy(namebuf, neo);
		}

		for(x = 0; x < (int)strlen(namebuf); x++)
		{
			if(namebuf[x] == 10 || namebuf[x] == 13)
			{
				namebuf[x] = 0;
				break;
			}
			else if(namebuf[x] > 0x00 && namebuf[x] < 0x20)
				namebuf[x] = 0x20;
		}

		AddCheatEntry(namebuf,addr,val,doc?compare:-1,0,type);
		tc++;

		// find next line break
		linebreak = strtok(nullptr, "\n");
	}
	RebuildSubCheats();
	return tc;
}

/****************************************************************************
 * SetupCheats
 *
 * Erases any prexisting cheats, loads cheats from a cheat file
 * Called when a ROM is first loaded
 ***************************************************************************/
void
SetupCheats()
{
	FCEU_PowerCheats();
	numcheats = 0;

	if(GameInfo->type == GIT_NSF)
		return;

	char filepath[1024];
	size_t offset = 0;

	if(EmuSettings.saveDevice == DEVICE_AUTO)
		return;

	if(!MakeFilePath(filepath, FILE_CHEAT))
		return;

	AllocSaveBuffer();

	offset = LoadFile(filepath, SILENT);

	// load cheat file if present
	if(offset > 0)
		numcheats = LoadCheats();

	FreeSaveBuffer ();
}

/****************************************************************************
 * Game Genie ROM
 *
 * gg.rom is either an iNES file (header + 16KB PRG + CHR) or a raw dump of
 * exactly 4KB PRG + 256 bytes CHR. Only the 4KB of PRG and the 256 bytes of
 * CHR the Genie uses are copied out of the file, whatever size it is.
 ***************************************************************************/
#define GENIE_ROM_SIZE      (4096 + 256)       // 4KB PRG + 256 bytes CHR
#define GENIE_INES_MIN_SIZE (16 + 16384 + 256) // header + 16KB PRG + CHR start

static const char *genieError = "Game Genie ROM not found!";

//!Why the last FindGameGenie() failed, as a short message fit for ErrorPrompt()
const char *GameGenieError()
{
	return genieError;
}

/* Extracts the 4KB PRG + 256 byte CHR the core wants from a gg.rom file image.
 * out must hold GENIE_ROM_SIZE bytes. Pure function - no I/O - and never reads
 * past file[size-1]. Returns false and sets *err if the image can't be a real
 * Game Genie ROM (running one would hang on a blank gray screen). */
// GG_PARSE_BEGIN
static bool ParseGenieRom(const uint8_t *file, size_t size, uint8_t *out, const char **err)
{
	const uint8_t *prg, *chr;

	if(size >= 4 && memcmp(file, "NES\x1A", 4) == 0) // iNES
	{
		if(size < GENIE_INES_MIN_SIZE)
		{
			*err = "gg.rom is too small!";
			return false;
		}
		prg = file + 16;
		chr = file + 16 + 16384;
	}
	else // raw: anything but an exact 4KB PRG + 256 byte CHR is some other file
	{
		if(size != GENIE_ROM_SIZE)
		{
			*err = "gg.rom is not a Game Genie ROM!";
			return false;
		}
		prg = file;
		chr = file + 4096;
	}

	memcpy(out, prg, 4096);
	memcpy(out + 4096, chr, 256);

	// The Genie runs from $8000-$FFFF, so its reset vector (last bytes of the 4KB) must point there
	uint16_t resetVec = out[0xFFC] | (out[0xFFD] << 8);
	if(resetVec < 0x8000)
	{
		*err = "gg.rom is not a valid Game Genie ROM!";
		return false;
	}

	// Blank font data = blank screen, whatever the code does
	bool chrBlank = true;
	for(int i = 0; i < 256; i++)
		if(out[4096 + i] != 0) { chrBlank = false; break; }
	if(chrBlank)
	{
		*err = "gg.rom has no graphics data!";
		return false;
	}
	return true;
}
// GG_PARSE_END

//!Loads and validates gg.rom (once; the result is kept). On failure GameGenieError() says why.
bool FindGameGenie()
{
	if (GENIEROM)
		return true;

	genieError = "Game Genie ROM not found!";

	if(EmuSettings.loadDevice == DEVICE_AUTO)
		return false;

	AllocSaveBuffer();

	size_t romSize = 0;
	char filepath[1024];

	platform->getFileSystem()->getPath(filepath, EmuSettings.loadDevice, APPFOLDER, "gg.rom");
	romSize = LoadFile(filepath, SILENT);
	if(romSize == 0 && strlen(appPath) > 0)
	{
		sprintf (filepath, "%s/gg.rom", appPath);
		romSize = LoadFile(filepath, SILENT);
	}

	bool ok = false;

	if (romSize > 0)
	{
		uint8_t rom[GENIE_ROM_SIZE];
		const char *err = NULL;

		if(ParseGenieRom(savebuffer, romSize, rom, &err))
		{
			GENIEROM = (uint8 *)malloc(4096+1024);
			if(GENIEROM)
			{
				memcpy(GENIEROM, rom, GENIE_ROM_SIZE);

				/* Workaround for the FCE Ultra CHR page size only being 1KB */
				for(int x=0; x<4; x++)
					memcpy(GENIEROM+4096+(x<<8),GENIEROM+4096,256);
				ok = true;
				genieError = NULL;
			}
			else
				genieError = "Out of memory!";
		}
		else
			genieError = err;
	}

	FreeSaveBuffer ();
	return ok;
}

//!Starts (or not) the Game Genie for the game about to be powered on, per the Game Genie setting.
//!If it is on but gg.rom is unusable, says why, switches the setting off and starts the game without it.
void OpenGameGenie()
{
	geniestage = 0;

	if(!EmuSettings.gameGenie)
		return;

	if(FindGameGenie())
		geniestage = 1;
	else
	{
		ErrorPrompt(GameGenieError());
		EmuSettings.gameGenie = false;
	}
}

//!Applies the current Game Genie setting to a game that is already running; call before PowerNES() on a reset
void ResetGameGenie()
{
	FCEU_CloseGenie(); // hands the cartridge's own handlers back if the Genie was still in place
	OpenGameGenie();
}
