/*
 * ---------------------------------------------------------------------------
 *  English by dsbomb
 * ---------------------------------------------------------------------------
 */

// Some general menu strings
#define MENU_ON                 "ON"
#define MENU_OFF                "OFF"
#define MENU_EXIT               "Return to Previous"
#define MENU_PRESS_A            "Press A to Continue"

// Main menu
#define MENU_MAIN_PLAY          "Play Game"
#define MENU_MAIN_RESET         "Reset NES"
#define MENU_MAIN_LOAD          "Load New Game"
#define MENU_MAIN_SAVE          "Save Manager"
#define MENU_MAIN_INFO          "ROM Information"
#define MENU_MAIN_JOYPADS       "Configure Joypads"
#define MENU_MAIN_OPTIONS       "Video Options"
#ifdef HW_RVL
#define MENU_MAIN_RELOAD        "TP Reload"
#define MENU_MAIN_REBOOT        "Reboot Wii"
#else
#define MENU_MAIN_RELOAD        "PSO Reload"
#define MENU_MAIN_REBOOT        "Reboot Gamecube"
#endif
#define MENU_MAIN_CREDITS       "View Credits"
#define MENU_MAIN_TEXT1         MENU_CREDITS_TITLE
#define MENU_MAIN_TEXT2         "Press L + R anytime to return to this menu!"
#define MENU_MAIN_TEXT3         "Press START + B + X anytime for PSO/SD-reload"
#define MENU_MAIN_TEXT4         "FCE Ultra GC is a modified port of the FCE Ultra 0.98.12 Nintendo Entertainment system for x86 (Windows/Linux) PC's. In English you can play NES games on your GameCube using either a softmod and/or modchip from a DVD or via a networked connection to your PC."
#define MENU_MAIN_TEXT5         "Disclaimer - Use at your own RISK!"
#define MENU_MAIN_TEXT6         "Official Homepage: http://www.tehskeen.net"

// Media menu
#define MENU_MEDIA_TITLE        "Load a Game"
#define MENU_MEDIA_SDCARD       "Load from SDCard"
#define MENU_MEDIA_DVD          "Load from DVD"
#define MENU_MEDIA_STOPDVD      "Stop DVD Motor"
#define MENU_MEDIA_STOPPING     "Stopping DVD ... Wait"
#define MENU_MEDIA_STOPPED      "DVD Motor Stopped"
#define MENU_MEDIA_TEXT1        "What are You waiting for? Load some games!"
#define MENU_MEDIA_TEXT2        "Still here?"
#define MENU_MEDIA_TEXT3        "How can You wait this long?! The games are waiting for You!!"

// Save menu
#define MENU_SAVE_TITLE         "Save State Manager"
#define MENU_SAVE_SAVE          "Save State"
#define MENU_SAVE_LOAD          "Load State"
#define MENU_SAVE_DEVICE        "Device"
#define MENU_SAVE_TEXT1         "From where do you wish to load/save your game?"
#define MENU_SAVE_TEXT2         "Hard time making up your mind?"

// Rom Information
#define MENU_INFO_TITLE         "ROM Information"
#define MENU_INFO_ROM           "ROM Size"
#define MENU_INFO_VROM          "VROM Size"
#define MENU_INFO_CRC           "iNES CRC"
#define MENU_INFO_MAPPER        "Mapper"
#define MENU_INFO_MIRROR        "Mirroring"

// Config Joypad menu
#define MENU_CONFIG_TITLE       "Controller Configuration"
#define MENU_CONFIG_A           "NES Button A"
#define MENU_CONFIG_B           "NES Button B"
#define MENU_CONFIG_START       "NES Button START"
#define MENU_CONFIG_SELECT      "NES Button SELECT"
#define MENU_CONFIG_TURBO_A     "NES Button TURBO A"
#define MENU_CONFIG_TURBO_B     "NES Button TURBO B"
#define MENU_CONFIG_FOUR_SCORE  "Four Score"
#define MENU_CONFIG_CLIP        "Analog Clip"
#define MENU_CONFIG_SPEED       "Turbo Speed"
#define MENU_CONFIG_TEXT1       "Configure Your Gamepad"
#define MENU_CONFIG_TEXT2       "Up, up, down, down, left, right, left, right, B, A, start"

// Emulator Options menu
#define MENU_VIDEO_TITLE        "Video Options"
#define MENU_VIDEO_SCALER       "Screen Scaler"
#define MENU_VIDEO_PALETTE      "Palette"
#define MENU_VIDEO_SPRITE       "8 Sprite Limit"
#define MENU_VIDEO_TIMING       "Timing"
#define MENU_VIDEO_SAVE         "Save Settings"
#define MENU_VIDEO_LOAD         "Load Settings"
#define MENU_VIDEO_DEFAULT      "Default"
#define MENU_VIDEO_TEXT1        "Wow, these colors and shapes sure are beautiful, brings back the memories."
#define MENU_VIDEO_TEXT2        "Be sure not to mess these settings up, You don't want to ruin the experience! :D"

// Credits menu
#ifdef HW_RVL
#define MENU_CREDITS_TITLE      "FCE Ultra Wii Edition v1.0.10beta1"
#else
#define MENU_CREDITS_TITLE      "FCE Ultra GC Edition v1.0.10beta1"
#endif
#define MENU_CREDITS_BY         "by"
#define MENU_CREDITS_GCPORT     "Gamecube Port"
#define MENU_CREDITS_ORIG       "Original FCE"
#define MENU_CREDITS_FCEU       "FCE Ultra"
#define MENU_CREDITS_DVD        "DVD Codes courtesy of"
#define MENU_CREDITS_MISC       "Misc addons"
#define MENU_CREDITS_EXTRAS     "Extra features"
#define MENU_CREDITS_THANK      "Thank you to"
#define MENU_CREDITS_WII        "for bringing it to the Wii"

