/****************************************************************************
 * DVD.CPP
 *
 * This module manages all dvd i/o etc.
 * There is also a simple ISO9660 parser included.
 ****************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sdcard.h>
#include <gctypes.h>

#include "sz.h"
#include "gcdvd.h"

/*** Simplified Directory Entry Record 
  I only care about a couple of values ***/
#define RECLEN 0
#define EXTENT 6
#define FILE_LENGTH 14
#define FILE_FLAGS 25
#define FILENAME_LENGTH 32
#define FILENAME 33

#define PAGESIZE 10

FILEENTRIES filelist[MAXFILES];
int maxfiles = 0;
int offset = 0;
int selection = 0;

/*** DVD Read Buffer ***/

unsigned char readbuffer[2048] ATTRIBUTE_ALIGN(32);
unsigned char DVDreadbuffer[2048] ATTRIBUTE_ALIGN (32);
volatile long *dvd=(volatile long *)0xCC006000;
static unsigned char *inquiry=(unsigned char *)0x80000004;
extern void SendDriveCode( int model );
extern int font_height;
extern bool isZipFile();
extern void writex(int x, int y, int sx, int sy, char *string, unsigned int selected);
extern unsigned char *nesromptr;
extern int IsXenoGCImage( char *buffer );

void GetSDInfo ();
extern int choosenSDSlot;
/*extern void ClearScreen();
  int LoadDVDFile( unsigned char *buffer );
  extern int unzipDVDFile( unsigned char *outbuffer, unsigned int discoffset, unsigned int length);
  extern int CentreTextPosition( char *text );*/

/** true if we the emulator is running on a wii **/
bool isWii = false;

int UseSDCARD = 0;
sd_file * filehandle;
char rootSDdir[SDCARD_MAX_PATH_LEN];
int haveSDdir = 0;
int sdslot = 0;

/****************************************************************************
 * DVD Lowlevel Functions
 *
 * These are here simply because the same functions in libogc are not
 * exposed to the user
 ****************************************************************************/

void dvd_inquiry()
{

    dvd[0] = 0x2e;
    dvd[1] = 0;
    dvd[2] = 0x12000000;
    dvd[3] = 0;
    dvd[4] = 0x20;
    dvd[5] = 0x80000000;
    dvd[6] = 0x20;
    dvd[7] = 3;

    while( dvd[7] & 1 );
    DCFlushRange((void *)0x80000000, 32);
}

void dvd_unlock()
{
    dvd[0] |= 0x00000014;
    dvd[1] = 0x00000000;
    dvd[2] = 0xFF014D41;
    dvd[3] = 0x54534849;
    dvd[4] = 0x54410200;
    dvd[7] = 1;
    while ((dvd[0] & 0x14) == 0) { }
    dvd[0] |= 0x00000014;
    dvd[1] = 0x00000000;
    dvd[2] = 0xFF004456;
    dvd[3] = 0x442D4741;
    dvd[4] = 0x4D450300;
    dvd[7] = 1;
    while ((dvd[0] & 0x14) == 0) { }
}

void dvd_extension()
{
    dvd[0] = 0x2E;
    dvd[1] = 0;

    dvd[2] = 0x55010000;
    dvd[3] = 0;
    dvd[4] = 0;
    dvd[5] = 0;
    dvd[6] = 0;
    dvd[7] = 1; // enable reading!
    while (dvd[7] & 1);
}

#define DEBUG_STOP_DRIVE 0
#define DEBUG_START_DRIVE 0x100
#define DEBUG_ACCEPT_COPY 0x4000
#define DEBUG_DISC_CHECK 0x8000

void dvd_motor_on_extra()
{

    dvd[0] = 0x2e;
    dvd[1] = 0;
    dvd[2] = 0xfe110000 | DEBUG_START_DRIVE | DEBUG_ACCEPT_COPY | DEBUG_DISC_CHECK;
    dvd[3] = 0;
    dvd[4] = 0;
    dvd[5] = 0;
    dvd[6] = 0;
    dvd[7] = 1;
    while ( dvd[7] & 1 );

}

void dvd_motor_off( )
{
    dvd[0] = 0x2e;
    dvd[1] = 0;
    dvd[2] = 0xe3000000;
    dvd[3] = 0;
    dvd[4] = 0;
    dvd[5] = 0;
    dvd[6] = 0;
    dvd[7] = 1; // Do immediate
    while (dvd[7] & 1);

    /*** PSO Stops blackscreen at reload ***/
    dvd[0] = 0x14;
    dvd[1] = 0;
}

void dvd_setstatus()
{
    dvd[0] = 0x2E;
    dvd[1] = 0;

    dvd[2] = 0xee060300;
    dvd[3] = 0;
    dvd[4] = 0;
    dvd[5] = 0;
    dvd[6] = 0;
    dvd[7] = 1; // enable reading!
    while (dvd[7] & 1);
}

unsigned int dvd_read_id(void *dst)
{
    if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
        DCInvalidateRange((void *)dst, 0x20);

    dvd[0] = 0x2E;
    dvd[1] = 0;

    dvd[2] = 0xA8000040;
    dvd[3] = 0;
    dvd[4] = 0x20;
    dvd[5] = (unsigned long)dst;
    dvd[6] = 0x20;
    dvd[7] = 3; // enable reading!

    while (dvd[7] & 1);

    if (dvd[0] & 0x4)
        return 1;
    return 0;
}

unsigned int dvd_read(void *dst, unsigned int len, u64 offset)
{

    unsigned char* buffer = (unsigned char*)(unsigned int)DVDreadbuffer;
    u32 offset32 = (u32)(offset >> 2);

    if (len > 2048 )
        return 1;

    DCInvalidateRange ((void *) buffer, len);

    if (offset < 0x57057C00 || (isWii == true && offset < 0x1FD3E0000LL)) // don't read past the end of the dvd
    {
        dvd[0] = 0x2E;
        dvd[1] = 0;
        dvd[2] = 0xA8000000;
        dvd[3] = offset32;
        dvd[4] = len;
        dvd[5] = (unsigned long)buffer;
        dvd[6] = len;
        dvd[7] = 3; // enable reading!
        while (dvd[7] & 1);
        memcpy (dst, buffer, len);
    } else // Let's not read past end of DVD
        return 1;

    if (dvd[0] & 0x4)               /* Ensure it has completed */
        return 1;

    return 0;

}

void dvd_reset(void)
{
    int i;

    *(unsigned long*)0xcc006004 = 2;
    unsigned long v = *(unsigned long*)0xcc003024;
    *(unsigned long*)0xcc003024 = (v &~4) | 1;

    for ( i = 0; i < 10000; i++ );

    *(unsigned long*)0xcc003024 = v | 5;
}

/****************************************************************************
 * ISO Parsing Functions
 ****************************************************************************/
#define PVDROOT 0x9c
static int IsJoliet = 0;
static u64 rootdir = 0;
static int rootdirlength = 0;
static int shadowroot, shadowlength;
int IsPVD()
{
    int sector = 16;
    //	u32 offset32;

    IsJoliet = rootdir = 0;

    /*** Read the ISO section looking for a valid
      Primary Volume Decriptor.
      Spec says first 8 characters are id ***/
    while ( sector < 32 ) {

        dvd_read( &readbuffer, 2048, sector << 11 );
        if ( memcmp( &readbuffer, "\2CD001\1", 8 ) == 0 ) {
            //memcpy(&offset32, &readbuffer[PVDROOT + EXTENT], 4);
            //memcpy(&rootdirlength, &readbuffer[PVDROOT + FILE_LENGTH], 4);
            //rootdir = (u64)offset32;
            memcpy(&rootdir, &readbuffer[PVDROOT + EXTENT], 4);
            memcpy(&rootdirlength, &readbuffer[PVDROOT + FILE_LENGTH], 4);
            IsJoliet = 2;
            break;
        }

        sector++;
    }

    if ( IsJoliet == 0 ) {
        sector = 16;

        while ( sector < 32 ) {

            if ( memcmp( &readbuffer, "\1CD001\1", 8 ) == 0 ) {
                /*memcpy(&offset32, &readbuffer[PVDROOT + EXTENT], 4);
                  memcpy(&rootdirlength, &readbuffer[PVDROOT + FILE_LENGTH], 4);
                  rootdir = (u64)offset32;*/
                memcpy(&rootdir, &readbuffer[PVDROOT + EXTENT], 4);
                memcpy(&rootdirlength, &readbuffer[PVDROOT + FILE_LENGTH], 4);
                IsJoliet = 1;
                break;
            }

            sector++;
        }
    }

    rootdir <<= 11;
    shadowroot = rootdir;
    shadowlength = rootdirlength;

    return IsJoliet;	
}

/****************************************************************************
 * getfiles
 *
 * Retrieve the current file directory entry
 ****************************************************************************/
static int diroffset = 0;
int getfiles( int filecount )
{
    char fname[256];
    char *ptr;
    char *filename;
    char *filenamelength;
    char *rr;
    int j;
    //u32 offset32;

    /*** Do some basic checks ***/
    if ( filecount == MAXFILES ) return 0;
    if ( diroffset >= 2048 ) return 0;

    /*** Now decode this entry ***/
    if ( readbuffer[diroffset] != 0 )
    {

        ptr = (char *)&readbuffer[0];
        ptr += diroffset;		
        filename = ptr + FILENAME;
        filenamelength = ptr + FILENAME_LENGTH;

        if ( diroffset + readbuffer[diroffset] > 2048 ) return 0;

        if ( *filenamelength ) {

            memset(&fname, 0, 128);

            /*** Return the values needed ***/
            if ( IsJoliet == 1 ) strcpy(fname, filename);
            else {
                for ( j = 0; j < ( *filenamelength >> 1 ); j++ ) {
                    fname[j] = filename[j*2+1];
                }

                fname[j] = 0;

                if ( strlen(fname) >= MAXJOLIET ) fname[MAXJOLIET-1] = 0;

                if ( strlen(fname) == 0 ) fname[0] = filename[0];
            }

            if ( strlen(fname) == 0 ) strcpy(fname,"ROOT");
            else {
                if ( fname[0] == 1 )  strcpy(fname,"..");
                else{
                    //fname[ *filenamelength ] = 0;
                    /*
                     * Move *filenamelength to t,
                     * Only to stop gcc warning for noobs :)
                     */
                    int t = *filenamelength;
                    fname[t] = 0;
                }
            }

            /** Rockridge Check **/ /*** Remove any trailing ;1 from ISO name ***/
            rr = strstr (fname, ";"); //if ( fname[ strlen(fname) - 2 ] == ';' )
            if (rr != NULL) *rr = 0;  //fname[ strlen(fname) - 2 ] = 0;*/

            /*strcpy(filelist[filecount].filename, fname);
              memcpy(&offset32, &readbuffer[diroffset + EXTENT], 4);
              memcpy(&cdfiles[filecount].length, &readbuffer[diroffset + FILE_LENGTH], 4);
              memcpy(&cdfiles[filecount].flags, &readbuffer[diroffset + FILE_FLAGS], 1);
              cdfiles[filecount].offset = (u64)offset32;*/
            strcpy(filelist[filecount].filename, fname);
            memcpy(&filelist[filecount].offset, &readbuffer[diroffset + EXTENT], 4);
            memcpy(&filelist[filecount].length, &readbuffer[diroffset + FILE_LENGTH], 4);
            memcpy(&filelist[filecount].flags, &readbuffer[diroffset + FILE_FLAGS], 1);

            filelist[filecount].offset <<= 11;
            filelist[filecount].flags = filelist[filecount].flags & 2;

            /*** Prepare for next entry ***/
            diroffset += readbuffer[diroffset];

            return 1;
        } 
    }
    return 0;
}

/****************************************************************************
 * ParseDirectory
 *
 * Parse the isodirectory, returning the number of files found
 ****************************************************************************/

int parsedir()
{
    int pdlength;
    int pdoffset;
    int rdoffset;
    int len = 0;
    int filecount = 0;

    pdoffset = rdoffset = rootdir;
    pdlength = rootdirlength;
    filecount = 0;

    /*** Clear any existing values ***/
    memset(&filelist, 0, sizeof(FILEENTRIES) * MAXFILES);

    /*** Get as many files as possible ***/			
    while ( len < pdlength )
    {
        if (dvd_read (&readbuffer, 2048, pdoffset) == 0) return 0;
        //dvd_read(&readbuffer, 2048, pdoffset);
        diroffset = 0;

        while ( getfiles( filecount ) )
        {
            if ( filecount < MAXFILES )
                filecount++;
        }

        len += 2048;
        pdoffset = rdoffset + len;
    }

    return filecount;
}

/***************************************************************************
 * Update SDCARD curent directory name 
 ***************************************************************************/ 
int updateSDdirname()
{
    int size=0;
    char *test;
    char temp[1024];
    //  char tmpCompare[1024];

    /* current directory doesn't change */
    if (strcmp(filelist[selection].filename,".") == 0) return 0; 

    /* go up to parent directory */
    else if (strcmp(filelist[selection].filename,"..") == 0) 
    {
        /* determine last subdirectory namelength */
        sprintf(temp,"%s",rootSDdir);
        test= strtok(temp,"\\");
        while (test != NULL)
        { 
            size = strlen(test);
            test = strtok(NULL,"\\");
        }

        /* remove last subdirectory name */
        size = strlen(rootSDdir) - size - 1;
        rootSDdir[size] = 0;

        /* handles root name */
        //sprintf(tmpCompare, "dev%d:",choosenSDSlot);
        if (strcmp(rootSDdir, sdslot ? "dev1:":"dev0:") == 0)sprintf(rootSDdir,"dev%d:\\fceu\\..", sdslot); 

        return 1;
    }
    else
    {
        /* test new directory namelength */
        if ((strlen(rootSDdir)+1+strlen(filelist[selection].filename)) < SDCARD_MAX_PATH_LEN) 
        {
            /* handles root name */
            //sprintf(tmpCompare, "dev%d:\\fceu\\..",choosenSDSlot);
            //if (strcmp(rootSDdir, tmpCompare) == 0) sprintf(rootSDdir,"dev%d:",choosenSDSlot);
            if (strcmp(rootSDdir, sdslot ? "dev1:\\fceu\\.." : "dev0:\\fceu\\..") == 0) sprintf(rootSDdir,"dev%d:",sdslot);

            /* update current directory name */
            sprintf(rootSDdir, "%s\\%s",rootSDdir, filelist[selection].filename);
            return 1;
        }
        else
        {
            WaitPrompt ("Dirname is too long !"); 
            return -1;
        }
    } 
}

/***************************************************************************
 * Browse SDCARD subdirectories 
 ***************************************************************************/ 

int parseSDdirectory() 
{
    int entries = 0;
    int nbfiles = 0;
    int numstored = 0;
    DIR *sddir = NULL;

    /* initialize selection */
    selection = offset = 0;

    /* Get a list of files from the actual root directory */ 
    entries = SDCARD_ReadDir (rootSDdir, &sddir);
    //entries = SDCARD_ReadDir (sdcardpath, &sddir);

    if (entries <= 0) entries = 0;	
    if (entries>MAXFILES) entries = MAXFILES;

    while (entries)
    {
        if (strcmp((const char*)sddir[nbfiles].fname, ".") != 0) { // Skip "." directory
            memset (&filelist[numstored], 0, sizeof (FILEENTRIES));
            strncpy(filelist[numstored].filename,(const char*)sddir[nbfiles].fname,MAXJOLIET);
            filelist[numstored].filename[MAXJOLIET-1] = 0;
            filelist[numstored].length = sddir[nbfiles].fsize;
            filelist[numstored].flags = (char)(sddir[nbfiles].fattr & SDCARD_ATTR_DIR);
            numstored++;
        }
        nbfiles++;
        entries--;
    }

    free(sddir);

    return nbfiles;
}
/****************************************************************************
 * ShowFiles
 *
 * Support function for FileSelector
 ****************************************************************************/

void ShowFiles( int offset, int selection )
{
    int i,j;
    char text[45];

    ClearScreen();

    j = 0;
    for ( i = offset; i < ( offset + PAGESIZE ) && ( i < maxfiles ); i++ )
    {
        if ( filelist[i].flags ) {
            strcpy(text,"[");
            strncat(text, filelist[i].filename,43);
            strcat(text,"]");
        } else
            strncpy(text, filelist[i].filename, 45);

        text[45]=0;

        /*if ( j == ( selection - offset ) )
          writex( CentreTextPosition(text), ( j * font_height ) + 117,
          GetTextWidth(text), font_height, text, blit_lookup_inv );
          else
          writex( CentreTextPosition(text), ( j * font_height ) + 117,
          GetTextWidth(text), font_height, text, blit_lookup );*/

        writex( CentreTextPosition(text), ( j * font_height ) + 130, GetTextWidth(text), font_height, text, j == ( selection - offset ) );

        j++;		
    }

    SetScreen();

}

/****************************************************************************
 * FileSelector
 *
 * Let user select another ROM to load
 ****************************************************************************/
bool inSz = false;
//#define PADCAL 70
extern int PADCAL;

void FileSelector()
{
    short p;
    short q = 0;
    signed char a;
    int haverom = 0;
    int redraw = 1;

    while ( haverom == 0 )
    {
        if ( redraw ) ShowFiles( offset, selection );

        redraw = 0;
        p = PAD_ButtonsDown(0);
        a = PAD_StickY(0);

        if (p & PAD_BUTTON_B) return;

        if ( ( p & PAD_BUTTON_DOWN ) || ( a < -PADCAL ) ){
            selection++;
            if (selection == maxfiles) selection = offset = 0;
            if ((selection - offset) >= PAGESIZE) offset += PAGESIZE;
            redraw = 1;
        } // End of down
        if ( ( p & PAD_BUTTON_UP ) || ( a > PADCAL ) )
        {
            selection--;
            if ( selection < 0 ){
                selection = maxfiles - 1;
                offset = selection - PAGESIZE + 1;
            }
            if (selection < offset) offset -= PAGESIZE;
            if ( offset < 0 ) offset = 0;
            redraw = 1;
        } // End of Up

        if ( (p & PAD_BUTTON_LEFT) || (p & PAD_TRIGGER_L) )
        {
            /*** Go back a page ***/
            selection -= PAGESIZE;
            if ( selection < 0 ) {
                selection = maxfiles - 1;
                offset = selection-PAGESIZE + 1;
            }
            if ( selection < offset ) offset -= PAGESIZE;
            if ( offset < 0 ) offset = 0;			
            redraw = 1;
        }

        if (( p & PAD_BUTTON_RIGHT ) || (p & PAD_TRIGGER_R))
        {
            /*** Go forward a page ***/
            selection += PAGESIZE;
            if ( selection > maxfiles - 1 ) selection = offset = 0;
            if ( ( selection - offset ) >= PAGESIZE ) offset += PAGESIZE;
            redraw = 1;
        }

        if ( p & PAD_BUTTON_A ) {
            if ( filelist[selection].flags )	/*** This is directory ***/
            {
                if (UseSDCARD)
                {
                    //if ( filelist[selection].filename[0] == 0x2e) { 
                    /* update current directory and set new entry list if directory has changed */
                    int status = updateSDdirname();
                    if (status == 1)
                    {							
                        maxfiles = parseSDdirectory();
                        if (!maxfiles)
                        {
                            WaitPrompt ("Error reading directory !");
                            haverom   = 1; // quit SD menu
                            haveSDdir = 0; // reset everything at next access
                        }
                    }
                    else if (status == -1)
                    {
                        haverom   = 1; // quit SD menu
                        haveSDdir = 0; // reset everything at next access
                    }
                } else {
                    rootdir = filelist[selection].offset;
                    rootdirlength = filelist[selection].length;
                    offset = selection = 0;
                    maxfiles = parsedir();
                }
                }
                else if(selection == 0 && inSz == true)
                {
                    rootdir = filelist[1].offset;
                    rootdirlength = filelist[1].length;
                    offset = 0;
                    maxfiles = parsedir();
                    inSz = false;
                    SzClose();
                }
                else if(inSz == false && SzDvdIsArchive(filelist[selection].offset) == SZ_OK)
                {
                    // parse the 7zip file
                    SzParse();
                    if(SzRes == SZ_OK)
                    {
                        inSz = true;
                        offset = selection = 0;
                    }
                    else
                    {
                        SzDisplayError(SzRes);
                    }
                }
                else if(inSz == true)
                {
                    // extract the selected ROM from the 7zip file to the buffer
                    if(SzExtractROM(filelist[selection].offset, nesromptr) == true)
                    {
                        haverom = 1;
                        inSz = false;

                        // go one directory up
                        rootdir = filelist[1].offset;
                        rootdirlength = filelist[1].length;
                        offset = selection = 0;
                        maxfiles = parsedir();
                    }
                }
                else
                {
                    rootdir = filelist[selection].offset;
                    rootdirlength = filelist[selection].length;
                    // Now load the DVD file to it's offset 
                    LoadDVDFile(nesromptr);	
                    haverom = 1;
                }
                redraw = 1;
            }
            /*if ( p & PAD_BUTTON_B ) {
              if ( cdfiles[1].filename[0] == 0x2e && cdfiles[1].filename[1] == 0x2e && cdfiles[1].flags ) {
              if (UseSDCARD) {
              char fname[SDCARD_MAX_PATH_LEN];
              int pathpos = strlen(cdfiles[1].sdcardpath);
              while (pathpos > 6) {
              if (cdfiles[1].sdcardpath[pathpos] == '\\') break;
              pathpos--;
              }
              if (pathpos < 7) {pathpos = 16; cdfiles[1].sdcardpath[pathpos-3] = '\\'; cdfiles[1].sdcardpath[pathpos-2] = '.'; cdfiles[1].sdcardpath[pathpos-1] = '.'; } // handle root case 
              strncpy(fname, cdfiles[1].sdcardpath, pathpos);
              fname[pathpos] = 0;
              offset = selection = 0;
              maxfiles = parse_sdcard_dir(fname);
              }
              else if(inSz == true)
              {
              rootdir = cdfiles[1].offset;
              rootdirlength = cdfiles[1].length;
              offset = selection = 0;
              maxfiles = parsedir();
              inSz = false;
              SzClose();
              }
              else {
              rootdir = cdfiles[1].offset;
              rootdirlength = cdfiles[1].length;
              offset = selection = 0;
              maxfiles = parsedir();
              }
              redraw = 1;
              }
              }*/
        }
    }

    /****************************************************************************
     * LoadDVDFile
     ****************************************************************************/
    int LoadDVDFile( unsigned char *buffer )
    {
        u64 offset;
        int blocks;
        int i;
        u64 discoffset;

        /*** SDCard Addition ***/
        if (UseSDCARD) GetSDInfo ();
        if (rootdirlength == 0)	return 0;

        /*** How many 2k blocks to read ***/
        blocks = rootdirlength / 2048;
        offset = 0;
        discoffset = rootdir;

        ShowAction("Loading ... Wait");
        if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, 2048);
        else dvd_read(&readbuffer, 2048, discoffset);

        if ( isZipFile() == false ) 
        {
            if (UseSDCARD) SDCARD_SeekFile (filehandle, 0, SDCARD_SEEK_SET);
            for ( i = 0; i < blocks; i++ ) 
            {
                if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, 2048);
                else dvd_read(&readbuffer, 2048, discoffset);
                memcpy(&buffer[offset], &readbuffer, 2048);
                offset += 2048;
                discoffset += 2048;
            }

            /*** And final cleanup ***/
            if( rootdirlength % 2048 ) 
            {
                i = rootdirlength % 2048;
                if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, i);
                else dvd_read(&readbuffer, 2048, discoffset);
                memcpy(&buffer[offset], &readbuffer, i);
            }
        } else {
            return unzipDVDFile( buffer, discoffset, rootdirlength);
        }
        if (UseSDCARD) SDCARD_CloseFile (filehandle);

        return rootdirlength;
    }

    /****************************************************************************
     * OpenDVD
     *
     * This function performs the swap task for softmodders.
     * For Viper/Qoob users, sector 0 is read, and if it contains all nulls
     * an ISO disc is assumed.
     ****************************************************************************/
    static int havedir = 0;

    int OpenDVD()
    {

        int i, j;
        int driveid=-99;

        haveSDdir = 0;

        /*** Get Drive Type ***/
        dvd_inquiry();
        driveid = (int)inquiry[2];

        /*** Make sure it's one I now about ***/
        if ( ( driveid != 4 ) && ( driveid != 6 ) && ( driveid != 8 ) ) {
            isWii = true;
        }

        memset(&readbuffer, 0x80, 2048);
        dvd_read(&readbuffer, 2048,0);

        for ( i = j = 0; i < 2048; i++ )
            j += readbuffer[i];

        if ( j ) {
            if ( IsXenoGCImage( (char *)&readbuffer ) )
                j = 0;
        }

        /*** Was there any data in sector 0 ? ***/
        // do not do all this stuff here if we are running on a Wii
        // because the modchip will take care of this.
        havedir = offset = selection = 0;
        if (isWii == false && j ) {
            /*** Yes - so start swap sequence ***/
            ShowAction("Stopping DVD ... Wait");
            dvd_motor_off();
            WaitPrompt("Insert an ISO 9660 DVD");
            ShowAction("Resetting DVD ... Wait");
            dvd_reset();

            /*** Now the fun begins
              This is essentially the cactus implementation from gc-linux
              sequence of events. There may well be a better way to do it
              from inside libogc, but no documentation is available. ***/

            /*** Reset the DVD Drive, to enable the firmware update ***/
            /*** Reset
              Unlock
              SendCode
              Enable Extension
              Unlock
              Motor On
              SetStatus
              ReadID ***/

            ShowAction("Sending Drive Code ... Wait");	
            dvd_unlock();
            SendDriveCode(driveid);
            dvd_extension();
            dvd_unlock();
            ShowAction("Mounting DVD ... Wait");
            dvd_motor_on_extra();
            dvd_setstatus();
            dvd_read_id((void *)0x80000000);
        }

        /*** At this point I should have an unlocked DVD ... so let's do the ISO ***/
        if ( havedir != 1  ) {
            if ( IsPVD() )
            {
                /*** Have a valid PVD, so start reading directory entries ***/
                maxfiles = parsedir();	
                if ( maxfiles ) {
                    offset = selection = 0;
                    FileSelector();
                    havedir = 1;
                }
            } else {
                return 0;
            }
        } else 
            FileSelector();

        return 1;		
    }

    int OpenSD () 
    {
        UseSDCARD = 1;
        char msg[128];

        if (choosenSDSlot != sdslot) haveSDdir = 0;

        if (haveSDdir == 0)
        {
            /* don't mess with DVD entries */
            havedir = 0;

            /* Reset SDCARD root directory */
            sprintf(rootSDdir,"dev%d:\\fceu\\roms",choosenSDSlot);
            sdslot = choosenSDSlot;

            /* Parse initial root directory and get entries list */
            ShowAction("Reading Directory ...");
            if ((maxfiles = parseSDdirectory ()))
            {
                sprintf (msg, "Found %d entries", maxfiles);
                WaitPrompt (msg);
                /* Select an entry */
                FileSelector ();

                /* memorize last entries list, actual root directory and selection for next access */
                haveSDdir = 1;
            }
            else
            {
                /* no entries found */
                sprintf (msg, "Error reading dev%d:\\fceu\\roms", choosenSDSlot);
                WaitPrompt (msg);
                return 0;
            }
        }
        /* Retrieve previous entries list and made a new selection */
        else  FileSelector ();

        return 1;
    }

    /****************************************************************************
     * SDCard Get Info
     ****************************************************************************/ 
    void GetSDInfo () {
        char fname[SDCARD_MAX_PATH_LEN];
        rootdirlength = 0;

        /* Check filename length */
        if ((strlen(rootSDdir)+1+strlen(filelist[selection].filename)) < SDCARD_MAX_PATH_LEN)
            sprintf(fname, "%s\\%s",rootSDdir,filelist[selection].filename); 

        else
        {
            WaitPrompt ("Maximum Filename Length reached !"); 
            haveSDdir = 0; // reset everything before next access
        }

        filehandle = SDCARD_OpenFile (fname, "rb");
        if (filehandle == NULL)
        {
            WaitPrompt ("Unable to open file!");
            return;
        }
        rootdirlength = SDCARD_GetFileSize (filehandle);
    }
