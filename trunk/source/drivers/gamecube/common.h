/****************************************************************************
 * Common module
 ****************************************************************************/

#include "../../driver.h"
#include "../common/config.h"

/* Message logging(non-netplay messages, usually) for all. */
extern int NoWaiting;
extern FCEUGI *GI;
void DSMFix(unsigned int msg);
void StopSound(void);

extern int eoptions;

#define EO_BGRUN          1

#define EO_CPALETTE       4
#define EO_NOSPRLIM       8
#define EO_FSAFTERLOAD   32
#define EO_FOAFTERSTART  64
#define EO_NOTHROTTLE   128
#define EO_CLIPSIDES    256
#define EO_SNAPNAME	512
#define EO_HIDEMENU    2048
#define EO_HIGHPRIO    4096
#define EO_FORCEASPECT  8192
#define EO_FORCEISCALE  16384
#define EO_NOFOURSCORE  32768

