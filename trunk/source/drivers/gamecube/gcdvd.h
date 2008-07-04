
#define MAXJOLIET 256
#define MAXFILES 1000

typedef struct {
	char filename[MAXJOLIET];
	char sdcardpath[256];
	u64 offset;
	unsigned int length;
	char flags;
}FILEENTRIES;

extern FILEENTRIES filelist[MAXFILES];
