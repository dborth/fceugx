/// \file
/// \brief Contains methods related to the build configuration

#include "types.h"
#include "version.h"
#include "fceu.h"
#include "driver.h"
#include "utils/memory.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

static char *aboutString = 0;

#ifndef FCEUX_BUILD_TIMESTAMP
#define FCEUX_BUILD_TIMESTAMP  __TIME__ " " __DATE__
#endif

// returns a string suitable for use in an aboutbox
const char *FCEUI_GetAboutString(void) 
{
	const char *aboutTemplate =
		FCEU_NAME_AND_VERSION "\n\n"
		"Administrators:\n"
		"zeromus, feos\n"
		"\n"
		"Current Contributors:\n"
		"CaH4e3, rainwarrior, owomomo, punkrockguy318\n"
		"\n"
		"Past Contributors:\n"
		"xhainingx, gocha, AnS, mjbudd77\n"
		"\n"
		"FCEUX 2.0:\n"
		"mz, nitsujrehtona, SP, Ugly Joe,\n"
		"Plombo, qeed, QFox, Shinydoofy\n"
		"ugetab, Acmlm, DWEdit\n"
		"\n"
		"Previous versions:\n"
		"FCE - Bero\n"
		"FCEU - Xodnizel\n"
		"FCEU XD - Bbitmaster & Parasyte\n"
		"FCEU XD SP - Sebastian Porst\n"
		"FCEU MM - CaH4e3\n"
		"FCEU TAS - blip & nitsuja\n"
		"FCEU TAS+ - Luke Gustafson\n"
		"\n"
		"Logo/icon:\n"
		"Terwilf\n"
		"\n"
		"FCEUX is dedicated to the fallen heroes\n"
		"of NES emulation. In Memoriam --\n"
		"ugetab\n"
		"\n"
		"\n"
		FCEUX_BUILD_TIMESTAMP "\n";

	if (aboutString) return aboutString;

	const char *compilerString = FCEUD_GetCompilerString();

	//allocate the string and concatenate the template with the compiler string
	if (!(aboutString = (char*)FCEU_dmalloc(strlen(aboutTemplate) + strlen(compilerString) + 1)))
        return NULL;

	sprintf(aboutString,"%s%s",aboutTemplate,compilerString);
	return aboutString;
}
