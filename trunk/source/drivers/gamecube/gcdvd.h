#include <sdcard.h>

#define MAXJOLIET 256
#define MAXFILES 1000

typedef struct {
    char filename[MAXJOLIET];
    char sdcardpath[SDCARD_MAX_PATH_LEN];
    u64 offset;
    unsigned int length;
    char flags;
}FILEENTRIES;

extern FILEENTRIES filelist[MAXFILES];
