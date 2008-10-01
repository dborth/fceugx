¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
                                 - FCE Ultra GX -
                                  Version 2.0.3   
                               (Under GPL License)
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
-=[ Explanation ]=-
 
FCE Ultra GX is a modified port of the FCE Ultra 0.98.12 Nintendo Entertainment
system for x86 (Windows/Linux) PC's. With it you can play NES games on your 
Wii/GameCube.  Version 2 is a complete rewrite based on code from the 
SNES9x GX and Genesis Plus GX projects.

-=[ Features ]=-

* Wiimote, Nunchuk, Classic, and Gamecube controller support
* 1-2 Player Support
* Zapper support
* RAM and State saving
* Custom controller configurations
* SD, USB, DVD, SMB, GC Memory Card, and Zip support
* NES Compatibility Based on v0.98.12
* Sound Filters
* Graphics Filters (GX Chipset, Cheesy and 2x)

×—–­—–­—–­—–­ –­—–­—–­—–­—–­—–­—–­—–­—–­—–­— ­—–­—–­—–­—–­—–­—–­—–­—-­—–­-–•¬
|0O×øo·                         UPDATE HISTORY                        ·oø×O0|
`¨•¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨'

[What's New 2.0.3 - October 1, 2008]
* Complete rewrite of loading code - FDS / UNIF / NSF support added!
* VS games work (coin insert submitted by pakitovic)
* Mapping of 'Special' commands - VS coin insert, FDS switch disk (default A)
* 480p and DVD now available for GameCube
* Improved stability - less crashes!

[What's New 2.0.2 - September 19, 2008]
* Fixed network freeze-up problem
* Zapper now mapped to A and B
* Fixed auto-save feature
* Performance slowdowns on Gamecube should be fixed
* Will now attempt to load old save states with CRC filename

[What's New 2.0.1 - September 6, 2008]
* Zapper support! Turn this on in the Controller Settings - most games 
  require you to have the Zapper on Port 2. Thanks go to aksommerville whose
  previous work on the Zapper helped, and michniewski's cursor code
* RAM game save support! Now you can save your games just like the NES did. 
  By default game saves are saved/loaded automatically. This can be changed
  in the Preferences menu
* Start/Select reversed mapping fixed for Wii controllers
* Small bug fixes / improvements / tweaks

[What's New 2.0.0 - September 1, 2008]

* Complete rewrite based on code from SNES9x GX and Genesis Plus GX
* Wiimote, Nunchuk, and Classic controller support
* Button mapping for all controller types
* Full support for SD, USB, DVD, GC Memory Card, and Zip files
* Game starts immediately after loading
* Load/save preference selector. ROMs, saves, and preferences are 
  saved/loaded according to these
* Preliminary Windows file share loading/saving (SMB) support on Wii:
  You can input your network settings into FCEUGX.xml, or edit 
  fceuconfig.cpp from the source code and compile.
* 'Auto' settings for save/load - attempts to automatically determine
  your load/save device(s) - SD, USB, Memory Card, DVD, SMB
* Preferences are loaded and saved in XML format. You can open
  FCEUGX.xml edit all settings, including some not available within
  the program
* One makefile to make all versions

×—–­—–­—–­—–­ –­—–­—–­—–­—–­—–­—–­—–­—–­—–­— ­—–­—–­—–­—–­—–­—–­—–­—-­—–­-–•¬
|0O×øo·                         SETUP & INSTALLATION                  ·oø×O0|
`¨•¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨'

Unzip the archive. You will find the following folders inside:

apps			Contains Homebrew Channel ready files
				(see Homebrew Channel instructions below)
				
executables		Contains Gamecube / Wii DOL files
				(for loading from other methods)
				
fceugx			Contains the directory structure required for storing
				roms and saves (see below)

------------------------------
Loading / Running the Emulator:
------------------------------

Wii - Via Homebrew Channel:
--------------------
The most popular method of running homebrew on the Wii is through the Homebrew
Channel. If you already have the channel installed, just copy over the apps 
folder included in the archive into the root of your SD card.

Remember to also create the fceugx directory structure required. See above.

If you haven't installed the Homebrew Channel yet, read about how to here:
http://hbc.hackmii.com/

Gamecube:
---------
You can load FCEUGX via sdload and an SD card in slot A, or by streaming 
it to your Gamecube, or by booting a bootable DVD with FCEUGX on it. 
This document doesn't cover how to do any of that. A good source for information 
on these topics is the tehskeen forums: http://www.tehskeen.com/forums/

----------------------------
ROMS, Preferences, and Saves:
----------------------------

By default, roms are loaded from "fceugx/roms/" and saves / preferences are 
stored in "fceugx/saves/".

Wii
----------
On the Wii, you can load roms from SD card (Front SD or SD Gecko), USB, DVD,
or SMB share. Note that if you are using the Homebrew Channel, to load from 
USB, DVD, or SMB you will first have to load FCEUGX from SD, and then set 
your load method preference. To load roms from a Windows network share (SMB) 
you will have to edit FCEUGX.xml on your SD card with your network settings, 
or edit fceuconfig.cpp from the source code and compile. If you edit and compile 
the source, you can use wiiload and the Homebrew Channel to load and play 
FCEUGX completely over the network, without needing an SD card.

Gamecube
------------
You can load roms from DVD or SD card. If you create a bootable 
DVD of FCEUGX you can put roms on the same DVD. You may save preferences and
game data to SD or Memory Card.

-=[ Famicom Disk System (FDS) ]=-

FCE Ultra GX supports loading FDS games. The FDS BIOS is required - put it 
in your roms folder, and name it disksys.rom (should be 8 KB in size).
You can switch disks using the A button (by default). The mapped button
can be changed under Controller Configuration ('Special' button).
Compatibility is limited, so check that the game in question works on 
FCE Ultra 0.98.12 for Windows before asking for help. 

-=[ Supported Mappers ]=-
 
Mappers are the way the Nintendo handles switching from ROM/VROM so the more
that are supported the more games will work.
 
000 . 112 . 113 . 114 . 117 .  15 . 151 . 16  . 17  .  18 . 180 . 182 . 184 . 187
189 . 193 . 200 . 201 . 202 . 203 . 208 . 21  . 22  . 225 . 226 . 227 . 228 . 229  
 23 . 230 . 231 . 232 . 234 . 235 . 240 . 241 . 242 . 244 . 246 . 248 .  24 .  26
 25 . 255 . 32  . 33  . 40  . 41  . 42  . 43  . 46  . 50  . 51  . 57  .  58 .  59
  6 .  60 . 61  . 62  . 65  . 67  . 68  . 69  . 71  . 72  . 73  . 75  .  76 .  77
 79 .   8 . 80  . 82  . 83  . 85  . 86  . 88  . 89  . 91  . 92  . 95  .  97 .  99
 
-[ Emulator Options ]-
 
Screen Scaler - How to scale the screen: GX, Cheesy or 2x
 
Palette - The colors used while viewing the game:
          Default . loopy . quor . chris . matt
          pasofami . crashman . mess . zaphod-cv
          zaphod-smb . vs-drmar . vs-cv . vs-smb
 
Timing - NTSC or PAL (Depends if you're running a PAL or NTSC game)

×—–­—–­—–­—–­ –­—–­—–­—–­—–­—–­—–­—–­—–­—–­— ­—–­—–­—–­—–­—–­—–­—–­—-­—–­-–•¬
|0O×øo·                    UPDATE HISTORY (1.0.x)                     ·oø×O0|
`¨•¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨'

What's new [20080331]

+[Askot]
- Fixed/changed SDCARD slot selection for searching roms, at 
  start you will be prompted for this option.
- Code cleanup.

+[dsbomb]
- Added Wii mode support.
- Give a "Bad cartridge" error instead of locking up.
- Joystick fixes due to libogc r14's changed stick values
- Rearranged menu to make more sense, and consistent with Snes9x
- Add "Reboot" menu option
- Removed "." directory from SD card listing, it's pointless
- Expand DVD reading to DVD9 size (once DVDs are working again)
- Added option to go back a menu by pressing B.

What's new? Askot [20080326]
- Added saving state in SD Card (State files will be saved in root of SDCARD).
  *Note: I can't make it work to save in root:\fceu\saves, so help needed.
- Added SDCARD slot selection for searching roms, meaning, you can search roms 
  from SDCARD SLOT A & SLOT B (Beta, meaning, buggy, but works).
- For standarization, you must create folders root:\fceu\roms to read NES 
  roms files from SDCARD.
- Added C-Left to call Menu.
- Reading files from SD Card it's faster, now they're called from cache
  after first reading.
- Menu in saving STATE file changed to choose SLOT, DEVICE, Save STATE, 
  Load STATE, Return to previous. 
- Added option PSO/SD Reload to menu, still works (START+B+X)
- Modified controls when going into the rom selection menu (DVD or 
  SDCARD):
  + Use B to quit selection list.
  + Use L/R triggrers or Pad Left/Right to go down/up one full page.
- Some menu rearrangment and a little of sourcecode cleanup: 
  + Everytime you pressed B button on any option, playgame started, not anymore
  until you select Play Game option.

What's new? asako [20070831]
- modify mmc3 code for Chinese pirated rom
- add some Chinese pirated rom mappers

What's new? _svpe_ [20070607]
- Wii support (PAL50 fix, 1.35GiB restriction removed)
- 7zip ROM loading support from the DVD (not from a SD card!!)
- dvd_read fix (the read data was always copied to readbuffer instead of *dst)
- slower file selection when using the D-Pad (I didn't like the selection to go 
as fast as in the latest release)
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
-=[ Credits ]=-

GameCube/Wii Port v2.x            Tantric
GameCube/Wii Port v1.0.9          askot & dsbomb
GameCube Port v1.0.8              SoftDev
FCE Ultra                         Xodnizel
Original FCE                      BERO
libogc                            Shagkur & wintermute
Testing                           tehskeen users

And many others who have contributed over the years!
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
                                  FCEUGX Web Site
                          http://code.google.com/p/fceugc
                              
                              TehSkeen Support Forums
                              http://www.tehskeen.net
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
