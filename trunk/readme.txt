¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
                                 - FCE Ultra GX -
                                  Version 3.0.7
                         http://code.google.com/p/fceugc   
                               (Under GPL License)
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤

FCE Ultra GX is a modified port of the FCE Ultra Nintendo Entertainment
system for x86 (Windows/Linux) PC's. With it you can play NES games on your 
Wii/GameCube.

-=[ Features ]=-

* Wiimote, Nunchuk, Classic, and Gamecube controller support
* iNES, FDS, VS, UNIF, and NSF ROM support
* 1-4 Player Support
* Zapper support
* Auto Load/Save Game States and RAM
* Custom controller configurations
* SD, USB, DVD (requires DVDx), SMB, GC Memory Card, Zip, and 7z support
* Custom controller configurations
* 16:9 widescreen support
* Original/filtered/unfiltered video modes
* Turbo Mode - up to 2x the normal speed
* Cheat support (.CHT files)
* IPS/UPS/PPF automatic patching support
* NES Compatibility Based on FCEUX 2.1.0a
* Open Source!

×—–­—–­—–­—–­ –­—–­—–­—–­—–­—–­—–­—–­—–­—–­— ­—–­—–­—–­—–­—–­—–­—–­—-­—–­-–•¬
|0O×øo·                         UPDATE HISTORY                        ·oø×O0|
`¨•¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨ ¨¨¨¨¨¨¨¨¨¨¨¨¨'

[3.0.7]

* Core upgraded to FCEUX 2.1.0a - improved game compatibility
* Cheat support (.CHT files)
* IPS/UPS/PPF automatic patching support
* Fixed "No game saves found." message when there are actually saves.
* Fixed shift key on keyboard
* Text scrolling works again
* Change default prompt window selection to "Cancel" button

[3.0.6 - July 9, 2009]

* Faster SMB/USB browsing
* Last browsed folder is now remembered
* Fixed controller mapping reset button
* Fixed no sound on GameCube version
* Directory names are no longer altered
* Preferences now only saved on exit
* Fixed on-screen keyboard glitches
* RAM auto-saved on power-off from within a game
* Prevent 7z lockups, better 7z error messages

[3.0.5 - June 30, 2009]

* Fixed auto-update
* Increased file browser listing to 10 entries, decreased font size
* Added text scrolling on file browser
* Added reset button for controller mappings
* Settings are now loaded from USB when loading the app from USB on HBC
* Fixed original mode lockup bug
* Fixed menu crashes caused by ogg player bugs
* Fixed memory card saving verification bug
* Fixed game savebrowser bugs
* Miscellaneous code cleanup/corrections

[3.0.4 - May 30, 2009]

* Fixed SD/USB corruption bug
* SMB works again
* GUI bugs fixed, GUI behavioral improvements

[3.0.3 - May 26, 2009]

* Improved stability
* Fixed broken SDHC from HBC 1.0.2 update
* Fixed issues with returning to menu from in-game
* Add option to disable rumble
* Auto-determines if HBC is present - returns to Wii menu otherwise
* Miscellaneous bugfixes

[3.0.2 - April 30, 2009]

* Improved scrollbar
* Multiple state saves now working
* Built with more stable libogc/libfat
* Fixed rumble bug in filebrowser
* Fixed PAL sound stuttering
* Added confirmation prompts
* Fixed settings saving glitches

[3.0.1 - April 22, 2009]

* GameCube controller home trigger fixed
* USB support fixed
* More stable SMB support
* Corrections/improvements to game saving/loading
* Video mode corrections
* Settings are now saved when exiting game menu settings area
* 8 sprite limit and Zapper crosshair can now be turned off from the menu
* New video mode selection in menu (forcing a video mode is not recommended)

[3.0.0 - April 13, 2009]

* New GX-based menu, with a completely redesigned layout. Has Wiimote IR 
  support, sounds, graphics, animation effects, and more
* Thanks to the3seashells for designing some top-notch artwork, to 
  Peter de Man for composing the music, and a special thanks to shagkur for 
  fixing libogc bugs that would have otherwise prevented the release
* Onscreen keyboard for changing save/load folders and network settings
* Menu configuration options (configurable exit button, wiimote orientation,
  volumes)
* Configurable button mapping for zapper
* New save manager, allowing multiple saves and save browsing. Shows
  screenshots for Snapshot saves, and save dates/times
* SMB reconnection feature
* ISI issue fixed

[2.0.9 - January 27, 2009]

* Fixed a major memory corruption bug in FCE Ultra 0.98.12
* Faster SD/USB - new read-ahead cache
* Removed trigger of back to menu for Classic Controller right joystick
* Changed GameCube controller back to menu from A+Start to A+B+Z+Start
* Add option for horizontal-only video cropping
* Decreased minimum game size to 8 KB
* Fixed a bug with reading files < 2048 bytes
* Fixed some memory leaks, buffer overflows, etc
* Code cleanup, other general bugfixes

[2.0.8 - December 24, 2008]

* Fixed unstable SD card access
* Proper SD/USB hotswap (Wii only)
* Auto-update feature (Wii only)
* Rewritten SMB access - speed boost, NTLM now supported (Wii only)
* Improved file access code
* Resetting preferences now resets controls
* Overscan (cropping) setting now saved in preferences
* Rewritten RAM/state saving - old state saves are now invalid
* Minor bug fixes

[2.0.7 - November 19, 2008]

* Special thanks to eke-eke & KruLLo for contributions, bugfixes, and tips
* Video code rewritten - now has original, unfiltered, filtered modes
* Zoom option
* 16:9 widescreen support
* Full widescreen support
* SDHC support
* SD/USB hot-swapping
* A/B rapid-fire
* Turbo option
* Video cropping (overscan hiding) option (thanks yxkalle!)
* Palette changing fixed
* Fixed audio 'popping' issue
* Wii - Added console/remote power button support
* Wii - Added reset button support (resets game)
* Wii - Settings file is now named settings.xml and is stored in the same
  folder as the DOL (eg: apps/fceugx/settings.xml)
* GameCube - Added DVD motor off option

[2.0.6 - October 21, 2008]

* Right audio channel corruption fixed (thanks cyberdog!)
* Low pass audio filter turned off (muffles audio)
* Changed to alternate audio filter
* PAL Timing corrected
* Cheesy/2X video filters fixed
* Qoob Pro modchip support for GameCube (thanks emukidid!)

[2.0.5 - October 19, 2008]

* Sound bug fixed - thanks eke-eke!
* High quality sound enabled, lowpass filter enabled
* Video threading enabled
* Fixed timing error (incorrect opcode)

[2.0.4 - October 15, 2008]

* Wii DVD fixed
* FDS BIOS loading works now
* FDS disk switching now consistently works with one button press
* FDS saving implemented
* 7z support
* Faster SD/USB (readahead cache enabled)
* VS coin now mapped to 1 button for VS zapper games
* Changed GC controller mappings - Select - Z, Start - Start, 
  Home - Start+A, Special - L

[2.0.3 - October 1, 2008]

* Complete rewrite of loading code - FDS / UNIF / NSF support added!
* VS games work (coin insert submitted by pakitovic)
* Mapping of 'Special' commands - VS coin insert, FDS switch disk (default A)
* 480p and DVD now available for GameCube
* Improved stability - less crashes!

[2.0.2 - September 19, 2008]

* Fixed network freeze-up problem
* Zapper now mapped to A and B
* Fixed auto-save feature
* Performance slowdowns on Gamecube should be fixed
* Will now attempt to load old save states with CRC filename

[2.0.1 - September 6, 2008]

* Zapper support! Turn this on in the Controller Settings - most games 
  require you to have the Zapper on Port 2. Thanks go to aksommerville whose
  previous work on the Zapper helped, and michniewski's cursor code
* RAM game save support! Now you can save your games just like the NES did. 
  By default game saves are saved/loaded automatically. This can be changed
  in the Preferences menu
* Start/Select reversed mapping fixed for Wii controllers
* Small bug fixes / improvements / tweaks

[2.0.0 - September 1, 2008]

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
				
gamecube		Contains GameCube DOL file (not required for Wii)
				
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
This document doesn't cover how to do any of that.

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

-[ Emulator Options ]-

Palette - The colors used while viewing the game:
          Default . loopy . quor . chris . matt
          pasofami . crashman . mess . zaphod-cv
          zaphod-smb . vs-drmar . vs-cv . vs-smb

Timing - NTSC or PAL (Depends if you're running a PAL or NTSC game)

¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤

-=[ Credits ]=-

			Coding & menu design		Tantric
			Menu artwork				the3seashells
			Menu sound					Peter de Man
			Logo design					mvit

			¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
			FCE Ultra GX GameCube		SoftDev,
										askot & dsbomb

			FCE Ultra					Xodnizel
			Original FCE				BERO
			libogc/devkitPPC			shagkur & wintermute
			FreeTypeGX					Armin Tamzarian

			And many others who have contributed over the years!
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
 
                                  FCEUGX Web Site
                          http://code.google.com/p/fceugc
 
¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤°`°¤ø,¸,ø¤°`°¤ø,¸¸,ø¤
