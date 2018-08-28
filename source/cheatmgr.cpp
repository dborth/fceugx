/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2009
 *
 * cheatmgr.cpp
 *
 * Cheat handling
 ***************************************************************************/

#include <malloc.h>
#include <gctypes.h>

#include "fceugx.h"
#include "fceusupport.h"
#include "fileop.h"
#include "filebrowser.h"
#include "menu.h"

#include "fceultra/cheat.h"

int numcheats = 0;

/****************************************************************************
 * LoadCheats
 *
 * Loads cheat file from save buffer
 * Custom version of FCEU_LoadGameCheats()
 ***************************************************************************/

static int LoadCheats (int length)
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

	while(linebreak != NULL && line_num < 500)
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
			char *neo=&tbuf[4+2+2+1+1+1];
			if(sscanf(tbuf,"%04x%*[:]%02x%*[:]%02x",&addr,&val,&compare)!=3)
				continue;
			namebuf=(char *)malloc(strlen(neo)+1);
			strcpy(namebuf,neo);
		}
		else
		{
			char *neo=&tbuf[4+2+1+1];
			if(sscanf(tbuf,"%04x%*[:]%02x",&addr,&val)!=2)
				continue;
			namebuf=(char *)malloc(strlen(neo)+1);
			strcpy(namebuf,neo);
		}

		for(x=0;x<(int)strlen(namebuf);x++)
		{
			if(namebuf[x]==10 || namebuf[x]==13)
			{
				namebuf[x]=0;
				break;
			}
			else if(namebuf[x]<0x20) namebuf[x]=' ';
		}

		AddCheatEntry(namebuf,addr,val,doc?compare:-1,status,type);
		FCEUI_ToggleCheat(tc); // turn cheat off
		tc++;

		// find next line break
		linebreak = strtok(NULL, "\n");
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

	int device = GCSettings.SaveMethod;

	if(device == DEVICE_AUTO)
		device = autoSaveMethod(SILENT);

	if(device == DEVICE_AUTO)
		return;

	if(!MakeFilePath(filepath, FILE_CHEAT))
		return;

	AllocSaveBuffer();

	offset = LoadFile(filepath, SILENT);

	// load cheat file if present
	if(offset > 0)
		numcheats = LoadCheats(offset);

	FreeSaveBuffer ();
}

bool FindGameGenie()
{
	if (GENIEROM)
		return true;

	if(GCSettings.LoadMethod == DEVICE_AUTO)
		GCSettings.LoadMethod = autoLoadMethod();

	if(GCSettings.LoadMethod == DEVICE_AUTO)
		return false;

	AllocSaveBuffer();

	size_t romSize = 0;
	char filepath[1024];

	sprintf (filepath, "%s%s/gg.rom", pathPrefix[GCSettings.LoadMethod], APPFOLDER);
	romSize = LoadFile(filepath, SILENT);
	if(romSize == 0 && strlen(appPath) > 0)
	{
		sprintf (filepath, "%s/gg.rom", appPath);
		romSize = LoadFile(filepath, SILENT);
	}

	if (romSize > 0)
	{
		GENIEROM=(uint8 *)malloc(4096+1024);

		if(savebuffer[0]==0x4E)  /* iNES ROM image */
		{
			memcpy(GENIEROM,savebuffer+16,4096);
			memcpy(GENIEROM+4096,savebuffer+16400,256);
		}
		else
		{
			memcpy(GENIEROM,savebuffer,4352);
		}

		/* Workaround for the FCE Ultra CHR page size only being 1KB */
		for(int x=0; x<4; x++)
			memcpy(GENIEROM+4096+(x<<8),GENIEROM+4096,256);
	}
	
	FreeSaveBuffer ();

	if(romSize > 0)
		return true;
	
	return false;
}

void OpenGameGenie()
{
	if(!GCSettings.gamegenie)
		geniestage=0;
	else if (FindGameGenie())
		geniestage=1;
}
