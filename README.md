# FCE Ultra GX
https://github.com/dborth/fceugx (Under GPL License)
 
FCE Ultra GX is a modified port of the FCE Ultra Nintendo Entertainment
system for x86 (Windows/Linux) PCs. With it you can play NES games on your 
Wii/GameCube.


## Nightly builds

|Download nightly builds from continuous integration: 	| [![Build Status][Build]][Actions] 
|-------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|

[Actions]: https://github.com/dborth/fceugx/actions
[Build]: https://github.com/dborth/fceugx/workflows/FCE%20Ultra%20GX%20Build/badge.svg


## Features

* Wiimote, Nunchuk, Classic, Wii U Pro, and Gamecube controller support
* Wii U GamePad support (requires homebrew injection into Wii U VC title)
* iNES, FDS, VS, UNIF, and NSF ROM support
* 1-4 Player Support
* Zapper support
* Auto Load/Save Game States and RAM
* Custom controller configurations
* SD, USB, DVD, SMB, Zip, and 7z support
* Custom controller configurations
* 16:9 widescreen support
* Original/filtered/unfiltered video modes
* Turbo Mode - up to 2x the normal speed
* Cheat support (.CHT files and Game Genie)
* Famicom 3D System support
* IPS/UPS automatic patching support
* NES Compatibility Based on FCEUX 2.2.3+ (git 21c0971)
* Open Source!


## UPDATE HISTORY

[3.4.7 - June 29, 2020]

* Compiled with latest devkitPPC/libogc
* Translation updates
* Added Wii U vWii Channel, widescreen patch, and now reports console/CPU speed
* Added additional exit combo to match the other emulators (L+R+START)
* Other minor fixes

[3.4.6 - March 4, 2020]

* Fixed 3rd party controllers (again)
* Fixed GameCube version issues with SD2SP2 

[3.4.5 - February 17, 2020]

* Fixed box art not working on GameCube
* Fixed some 3rd party controllers with invalid calibration data
* Fixed file browser issues
* Fixed issue changing Auto Save option

[3.4.4 - February 9, 2020]

* Added back start+A+B+Z trigger to go back to emulator
* Updated spanish translation
* Added support for serial port 2 (SP2 / SD2SP2) on Gamecube
* Compiled with latest libraries

[3.4.3 - April 13, 2019]

* Updated spanish translation (thanks Psycho RFG)
* Fixed preview image not displaying on GameCube
* Fixed crash when used as wiiflow plugin
* Fixed crash on launch when using network shares
* Fixed issues with on-screen keyboard
* Updated Korean translation

[3.4.2 - January 25, 2019]

* Fixed GameCube controllers not working
* Added ability to load external fonts and activated Japanese/Korean 
  translations. Simply put the ko.ttf or jp.ttf in the app directory
* Added ability to customize background music. Simply put a bg_music.ogg
  in the app directory
* Added ability to change preview image source with + button (thanks Zalo!)

[3.4.1 - January 4, 2019]

* Improved WiiFlow integration
* Fixed controllers with no analog sticks
* Added Wii U GamePad support (thanks Fix94!)

[3.4.0 - August 23, 2018]

* Updated to the latest FCEUX core
* Updated color palettes (thanks Tanooki16!)
* Allow loader to pass two arguments instead of three (libertyernie)
* Added PocketNES interoperability (load ROMs and read/write SRAM)
* Fixed audio pop when returning to a game from the menu
* Added option to not append " Auto" on saves
* Added soft and sharp video filtering options
* Removed update check completely
* Compilation fixes for DevkitPPC

[3.3.9 - December 10, 2016]

* Hide saving dialog that pops up briefly when returning from a game
* don't ignore buttons when zapper is enabled. prevented "Gotcha! The Sport!" from working (thanks liuhb86!)

[3.3.8 - May 14, 2016]

* Removed some unused and redundant palettes (thanks to Burnt Lasagna), new naming convention is:
    Accurate Colors = Unsaturated-V5 Palette By FirebrandX
    Vivid Colors = YUV-V3 Palette By FirebrandX
    Wii VC Colors	= Wii's Virtual Console Palette By SuperrSonic
    3DS VC Colors	= 3DS's Virtual Console Palette By SuperrSonic
	FCEUGX Colors	= Real Palette by AspiringSquire
* Added a new "Delete" button in the Game Options, to erase Save States and SRAM files.
* NES Zapper support fixed (thanks to Burnt Lasagna)

[3.3.7 -  Apr 18, 2016]

* Added both Firebrandx NES color palettes (thanks to SuperrSonic and Asho).
* Added Nestopia's RGB palette (thanks to SuperrSonic and ShadowOne333).
* Added a new window when selecting a color palette (in order to avoid cycling the color palettes one by one).
* Reverted FDS file in order to fix Disk System support (thanks to Burnt Lasagna) (Support was broken on ver 3.3.5 MOD).
* Added option to disable / enable the Virtual Memory messages on the settings menu.
* Removed the "Reset" and "Power On" messages when loading and reseting a game (Messages were added on ver 3.3.5 MOD).

[3.3.6 -  Apr 12, 2015]

* Merged Emu_kidid's 3.3.5 mod version with Zopenko's 3.3.4 mod version.
* Added SuperrSonic's 3DS Virtual Console palette.
* Changed the savestate cursor box color (in order to match the emu's color design).

[3.3.5 MOD -  Apr 22, 2015]

* Merged in changes from FCEUX (up to r2951)
* Added tueidj's TLB VM (w/ ARAM storage) for ROM and other data storage
* Enabled menu audio
* Less out of memory crashes
* Free memory displayed on in game menu

[3.3.4 MOD -  Apr 12, 2015]

* Added Cebolleto's preview image support.
* Added FIX94's WiiUPro controller support.
* Added SuperrSonic's Wii Virtual Console Palette.
* Increase preview image size and reduce game list width.
* Added a background to the preview image.
* Added a Screenshot button (under the game settings options, the video scaling option must be set to default otherwise screenshot looks smaller and with black borders around it, also screenshot folder must already exist otherwise a folder error will popup).
* Added a "WiiuPro" button on the button mapping menu, the options is just for completeness, since the controller mappings are shared between the wiiupro and the classic controller.
* Fixed the inverted color button selection that was in some option Windows.
* On the cheat menu, increased the cheat name display size and added scrolling if the name is too long to display at once.
* Fixed cover image dimensions, now it displays screenshot and cover within the background border.
* Fixed screenshot option, it no longer creates an additional "dummy" file.

[3.3.4 - January 12, 2013]

* Updated core to latest FCEUX (r2818)

[3.3.3 - December 14, 2012]

* Updated core to latest FCEUX (r2793)

[3.3.2 - November 9, 2012]

* Fixed lag with GameCube controllers

[3.3.1 - July 7, 2012]

* Fixed PAL support

[3.3.0 - July 6, 2012]

* Support for newer Wiimotes
* Fixed screen flicker when going back to menu
* Improved controller behavior - allow two directions to be pressed simultaneously
* Updated core to latest FCEUX (r2522)
* Compiled with devkitPPC r26 and libogc 1.8.11

[3.2.9 - January 25, 2012]

* Fixed zapper support

[3.2.8 - January 23, 2012]

* Fixed bug with flipping disk sides for FDS

[3.2.7 - January 14, 2012]

* Updated core to latest FCEUX (r2383)
* More accurate pixel scaling (thanks eke-eke!)
* Other minor changes

[3.2.6 - May 15, 2011]

* Fixed audio skipping (thanks thiagoalvesdealmeida!)
* Added Turkish translation

[3.2.5 - March 23, 2011]

* Fixed browser regressions with stability and speed

[3.2.4 - March 19, 2011]

* Updated core to latest FCEUX
* Support for Famicom 3D System games (thanks Carl Kenner!)
* Improved USB and controller compatibility (recompiled with latest libogc)
* Enabled SMB on GameCube (thanks Extrems!)
* Added Catalan translation
* Translation updates

[3.2.3 - October 7, 2010]

* Sync with upstream SVN - fixes a few specific game issues
* Fixed "blank listing" issue for SMB
* Improved USB compatibility and speed
* Added Portuguese and Brazilian Portuguese translations
* Channel updated (improved USB compatibility)
* Other minor changes

[3.2.2 - August 14, 2010]

* IOS 202 support removed
* USB 2.0 support via IOS 58 added - requires that IOS58 be pre-installed
* DVD support via AHBPROT - requires latest HBC

[3.2.1 - July 22, 2010]

* Fixed broken auto-update

[3.2.0 - July 20, 2010]

* Reverted USB2 changes

[3.1.9 - July 14, 2010]

* Fixed 16:9 correction in Original mode
* Fixed PAL/NTSC timing switching issue
* Ability to use both USB ports (requires updated IOS 202 - WARNING: older 
  versions of IOS 202 are NO LONGER supported)
* Hide non-ROM files
* Other minor improvements

[3.1.8 - June 20, 2010]

* USB improvements
* GameCube improvements - audio, SD Gecko, show thumbnails for saves
* Other minor changes

[3.1.7 - May 19, 2010]

* DVD support fixed
* PAL/NTSC timing corrections
* Fixed some potential hangs when returning to menu
* Video/audio code changes
* Fixed scrolling text bug
* Other minor changes

[3.1.6 - April 9, 2010]

* Fix auto-save bug

[3.1.5 - April 9, 2010]

* Most 3rd party controllers should work now (you're welcome!)
* Translation updates (German and Dutch)
* Other minor changes

[3.1.4 - March 30, 2010]

* DVD / USB 2.0 support via IOS 202. DVDx support has been dropped. It is
  highly recommended to install IOS 202 via the included installer
* Multi-language support (only French translation is fully complete)
* Thank you to everyone who submitted translations
* SMB improvements/bug fixes
* Minor video & input performance optimizations
* Synced with official FCEUX (various game fixes)
* ROMs larger than 3 MB now load
* Now also searches in application path for gg.rom and disksys.rom

[3.1.3 - December 23, 2009]

* Fixed major file loading issue, more games load now
* File browser now scrolls down to the last game when returning to browser
* Auto update for those using USB now works
* Fixed scrollbar up/down buttons
* Fixed zapper
* Updates from FCEUX
* Minor optimizations

[3.1.2 - December 2, 2009]

* Fixed SMB (for real this time!)

[3.1.1 - November 30, 2009]

* Mapper fixes - several more games work now (Fire Emblem, 76-in-1, etc)
* Fixed SMB
* Added separate horizontal/vertical zoom options
* Improved scrolling timing - the more you scroll, the fast it goes
* Fixed reset button on Wii console - now you can reset multiple times
* Reduce memory fragmentation - fixes out of memory crashes
* Other minor code optimizations

[3.1.0 - October 7, 2009]

* New default palette - more accurate colors!
* Revamped filebrowser and file I/O
* New timing and frameskip code - allows PAL gamers to play NTSC games
* Fixed FDS/Game Genie errors
* Many, many other bug fixes

[3.0.9 - September 16, 2009]

* Text rendering corrections
* SMB improvements
* Updated to latest FCEUX SVN
* Built with latest libraries
* Video mode switching now works properly
* Other minor bugfixes and cleanup

[3.0.8 - July 31, 2009]

* Fixed menu crash
* Fixed turbo mode - reduced to frameskip of 1
* Fixed .CHT file support
* Added Game Genie support - required GG rom placed at /fceugx/gg.rom
* FDS BIOS location changed to /fceugx/disksys.rom
* DVD file limit of 2000 removed

[3.0.7 - July 24, 2009]

* Core upgraded to FCEUX 2.1.0a - improved game compatibility
* State issues fixed - old state files are now invalid!
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


## SETUP & INSTALLATION

Unzip the archive. You will find the following folders inside:

apps			Contains Homebrew Channel ready files
				(see Homebrew Channel instructions below)

fceugx			Contains the directory structure required for storing
				roms and saves (see below)


### Loading / Running the Emulator:

#### Wii - Via Homebrew Channel:
The most popular method of running homebrew on the Wii is through the Homebrew
Channel. If you already have the channel installed, just copy over the apps 
folder included in the archive into the root of your SD card.

Remember to also create the fceugx directory structure required. See above.

If you haven't installed the Homebrew Channel yet, read about how to here:
http://hbc.hackmii.com/

#### Gamecube:
You can load FCEUGX via sdload and an SD card in slot A, or by streaming 
it to your Gamecube, or by booting a bootable DVD with FCEUGX on it. 
This document doesn't cover how to do any of that.

#### ROMS, Preferences, and Saves:
By default, roms are loaded from "fceugx/roms/" and saves / preferences are 
stored in "fceugx/saves/".

#### Wii
On the Wii, you can load roms from SD card (Front SD or SD Gecko), USB, DVD,
or SMB share. Note that if you are using the Homebrew Channel, to load from 
USB, DVD, or SMB you will first have to load FCEUGX from SD, and then set 
your load method preference.

If you are planning to use your Network (LAN) to load and/or save games from 
you will need to enter in the SMB share settings you haveve setup on your 
computer via the Settings menu. You will need to enter in the SMB Share IP, 
Share Name, Share Username and Share Password.

#### Gamecube
You can load roms from DVD or SD card. If you create a bootable 
DVD of FCEUGX you can put roms on the same DVD. You may save preferences and
game data to SD or Memory Card.


#### Famicom Disk System (FDS)

FCE Ultra GX supports loading FDS games. The FDS BIOS is required - put it 
in your fceugx folder, and name it disksys.rom (should be 8 KB in size).
You can switch disks using the A button (by default). The mapped button
can be changed under Controller Configuration ('Special' button).
Compatibility is limited, so check that the game in question works on 
FCEUX for Windows before asking for help. 

#### 3D Game Support

Supported Famicom 3D System games: 
* Highway Star
* Famicom Grand Prix II
* 3D Hot Rally
* JJ (Tobidase Daisakusen Part 2)
* Cosmic Epsilon
* Attack Animal Gakuen

Supported anaglyph games:
* The 3-D Battles of World Runner (Tobidase Daisakusen)
* Rad Racer

#### Emulator Options

Palette - The colors used while viewing the game:
          Default . loopy . quor . chris . matt
          pasofami . crashman . mess . zaphod-cv
          zaphod-smb . vs-drmar . vs-cv . vs-smb

Timing - NTSC or PAL (Depends if you're running a PAL or NTSC game)


## CREDITS

			Coding & menu design		Tantric
			Menu artwork				the3seashells
			Menu sound					Peter de Man
			Logo design					mvit
			Additional updates/fixes	Zopenko, Burnt Lasagna, Askot
			Beta testing, bug reports	Sindrik, niuus
			
			FCE Ultra GX GameCube		SoftDev,
										askot & dsbomb

			FCE Ultra					Xodnizel
			Original FCE				BERO
			libogc/devkitPPC			shagkur & wintermute
			FreeTypeGX					Armin Tamzarian

			And many others who have contributed over the years!


## LINKS

                                  FCEUGX Web Site
                          https://github.com/dborth/fceugx
 
