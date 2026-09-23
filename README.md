# FCE Ultra GX

[github.com/dborth/fceugx](https://github.com/dborth/fceugx) — GPL licensed

FCE Ultra GX is a Nintendo Entertainment System / Famicom emulator for the **Nintendo GameCube**, **Wii**, and **Wii U**, built on [FCEUX](https://fceux.com) and the shared [`libgui`](https://github.com/dborth/libgui) UI/driver framework.

FCE Ultra GX is homebrew — it isn't signed by Nintendo, so your console needs to be set up to run unsigned code first. If you haven't done that yet, jump to **[Installation](#installation)** below; it links to a step-by-step guide for whichever console you have.

## Table of Contents

- [Nightly Builds](#nightly-builds)
- [Features](#features)
- [Installation](#installation)
  - [All Platforms: SD Card & Folder Layout](#all-platforms-sd-card--folder-layout)
  - [Wii](#wii)
  - [GameCube](#gamecube)
  - [Wii U](#wii-u)
- [Initial Setup](#initial-setup)
- [Configuration](#configuration)
  - [Button Mappings](#button-mappings)
  - [Video](#video)
  - [Emulation](#emulation)
  - [Saving & Loading](#saving--loading)
  - [Menu](#menu)
  - [Language & Custom Fonts](#language--custom-fonts)
  - [Artwork](#artwork)
  - [Network Shares (SMB)](#network-shares-smb)
- [File Browser](#file-browser)
- [Gameplay](#gameplay)
- [Cheats & Game Genie](#cheats--game-genie)
- [Turbo Mode & Rapid Fire](#turbo-mode--rapid-fire)
- [Famicom Disk System (FDS)](#famicom-disk-system-fds)
- [VS System](#vs-system)
- [Famicom 3D System & Anaglyph Games](#famicom-3d-system--anaglyph-games)
- [Importing and Exporting RAM Saves](#importing-and-exporting-ram-saves)
- [Credits](#credits)
- [Links](#links)

> 📜 Looking for version notes? They're in the **[CHANGELOG.md](CHANGELOG.md)**.

---

## Nightly Builds

Every push builds automatically. Grab the latest continuous-integration build:

| Platform                    | Status                             | Download                                    |
|-----------------------------|------------------------------------|---------------------------------------------|
| Wii / vWii                  | [![Build Status][Build]][Actions]  | [![Download][Download]][fceugx-wii]         |
| GameCube                    | [![Build Status][Build]][Actions]  | [![Download][Download]][fceugx-gamecube]    |
| Wii U                       | [![Build Status][Build]][Actions]  | [![Download][Download]][fceugx-wiiu]        |

[Actions]: https://github.com/dborth/fceugx/actions/workflows/build.yml
[Build]: https://github.com/dborth/fceugx/actions/workflows/build.yml/badge.svg
[Download]: https://img.shields.io/badge/Download-blue
[fceugx-wii]: https://github.com/dborth/fceugx/releases/download/Pre-release/FCEUltraGX.zip
[fceugx-gamecube]: https://github.com/dborth/fceugx/releases/download/Pre-release/FCEUltraGX-GameCube.zip
[fceugx-wiiu]: https://github.com/dborth/fceugx/releases/download/Pre-release/FCEUltraGX-WiiU.zip

> The Wii build also runs unmodified in **vWii** (the Wii U's built-in Wii-compatibility mode), including via Virtual Console-style injection. The Wii U build is a separate, **native** Wii U (Aroma) application — see [Wii U](#wii-u) below for how the three options compare.

---

## Features

- Based on [FCEUX](https://fceux.com) (compatible with FCEUX 2.6.6+ core behavior)
- Wiimote, Nunchuk, Wii Classic Controller, Wii U Pro Controller, and GameCube Controller support
- **Wii U GamePad** support — full touch + buttons on the **native Wii U build**; buttons/sticks and display (no touch) on **vWii via Virtual Console-style injection** — see [Wii U](#wii-u)
- Native Wii U version outputs up to **1080p**, with a **GX2 shader-based ScaleFX** upscaler built specifically for the Wii U's GPU
- **1–4 player support**, plus Zapper light gun support
- iNES (`.nes`), Famicom Disk System (`.fds`), VS System, UNIF, and NSF (NES Sound Format) ROM support
- **Famicom 3D System** and anaglyph-game support — see [Famicom 3D System & Anaglyph Games](#famicom-3d-system--anaglyph-games)
- Cheat code support (`.cht` files) plus **Game Genie** code support
- Cover art / screenshot / artwork preview support
- Auto load/save of save states and RAM saves (in-game battery saves)
- Fully customizable, per-controller button mappings, including independent **Rapid Fire** bindings for A/B
- SD, USB, DVD, and SMB network share support (modern SMB2/3 via `libsmb2` — see [Network Shares (SMB)](#network-shares-smb)), plus ZIP/7z archive loading and automatic IPS/UPS ROM patching
- Auto-detected PAL/NTSC/Dendy timing, 16:9 widescreen correction
- Selectable video output mode including authentic 240p on Wii / GameCube
- A dozen curated color palettes, plus the FCEUX default
- Upscaling filters (hq2x, Scale2x, 2xBR, DDT on GameCube/Wii; ScaleFX on Wii U) plus scanlines
- Configurable and remappable Turbo Mode (distinct from per-button Rapid Fire — see [Turbo Mode & Rapid Fire](#turbo-mode--rapid-fire))
- Adjustable screen zoom, screen position, and overscan cropping
- Open source

---

## Installation

### All Platforms: SD Card & Folder Layout

However you load FCE Ultra GX, it looks for its files in a `fceugx` folder at the root of your storage device. Format your SD card as **FAT32** — it's the most reliable, best-tested option across all three consoles, and the one this guide assumes throughout. USB drives, DVD, and SMB network shares are also supported (see [Saving & Loading](#saving--loading) and [Network](#network)), but SD is the simplest starting point and works identically on GameCube, Wii, and Wii U.

Once you've placed the loader files for your console (below), also create these folders and drop your content in:

```text
SD:/fceugx/
├── roms/          ← your NES/Famicom ROMs (.nes/.fds/.unf/.nsf, or zipped/.7z)
├── saves/         ← RAM saves and save states
├── cheats/        ← .cht cheat files (see Cheats & Game Genie)
├── screenshots/   ← in-game screenshots and/or screenshot preview images
├── covers/        ← cover art preview images
└── artwork/       ← artwork preview images
```

Only `roms/` needs anything in it to get started — the rest are created automatically the first time they're needed. You can point the emulator at different load/save folders later from [Saving & Loading](#saving--loading). Two optional add-on files also go directly in `fceugx/` (not in a subfolder): `disksys.rom` for Famicom Disk System games (see [Famicom Disk System (FDS)](#famicom-disk-system-fds)) and `gg.rom` for Game Genie codes (see [Cheats & Game Genie](#cheats--game-genie)).

### Wii

1. Follow the **[Wii Homebrew Guide](https://wii.hacks.guide/)** if you haven't already installed the Homebrew Channel. This is a one-time setup per console.
2. Download the Wii build (`FCEUltraGX.zip` above) and extract it to the root of your SD card. This adds two things:
   - `apps/fceugx/boot.dol` (plus its icon/meta files) — this is what the Homebrew Channel launches.
   - `fceugx/` — your ROMs and saves folder, per [above](#all-platforms-sd-card--folder-layout).
3. Insert the SD card, open the **Homebrew Channel**, and launch **FCE Ultra GX**.

Your SD card should look like this:

```text
SD:/
├── apps/
│   └── fceugx/
│       ├── boot.dol
│       ├── icon.png
│       └── meta.xml
└── fceugx/
    └── roms/
        └── ...
```

#### About the forwarder channel

A **forwarder** is a small channel installed to your Wii's actual System Menu that, when launched, simply hands off to a homebrew app on your SD card — it makes FCE Ultra GX show up as its own channel on the Wii Menu instead of something you dig for inside the Homebrew Channel every time.

A common point of confusion: **installing the forwarder does not copy the emulator onto the channel itself.** The forwarder is just a pointer — `boot.dol` still needs to exist at `apps/fceugx/boot.dol` on your SD card every time you launch it. If you update FCE Ultra GX later, you only need to replace that `boot.dol` (and the `fceugx` folder if instructed); you do **not** need to reinstall or recreate the forwarder channel itself. The forwarder only needs to be installed once, ever, per console.

To install the forwarder, use the **[official channel installer](https://github.com/dborth/fceugx/releases)**, on the releases page, which points to `apps/fceugx/boot.dol` on your SD card.

### GameCube

GameCube doesn't have anything like the Wii's Homebrew Channel sitting on the console itself — instead you boot a **loader**, a small piece of software that then launches your `.dol`. The de facto standard today is **[Swiss](https://github.com/emukidid/swiss-gc)**, a GameCube loader/multitool that can read `.dol` files straight off an SD card (via an SD Gecko or SD2SP2 adapter) and handles most other loading methods too. This README assumes Swiss.

Exactly how you get Swiss running (modchip, boot-disc exploit, Broadband Adapter, etc.) depends on your GameCube's hardware revision and what you already own — **[gc-forever.com](https://www.gc-forever.com/)** is the best community hub for GameCube homebrew and hardware guides matched to your exact setup; start there if you're not sure what applies to you.

Once Swiss is running, the **recommended setup is an SD Gecko (or SD2SP2) memory-card-slot adapter**, using the same FAT32 SD card approach as Wii/Wii U — by far the most reliable and lowest-latency option.

FCE Ultra GX also supports **GC Loader** and **DVD** (burned disc) loading, but be aware going in: both are noticeably rougher experiences than SD — GC Loader in particular has had more reported reliability issues in this port, and burned-disc loading is slow to start and inflexible to update. Use them only if SD Gecko/SD2SP2 genuinely isn't an option for your setup.

1. Set up Swiss (or another loader of your choice) for your GameCube — see [gc-forever.com](https://www.gc-forever.com/) for hardware-specific guides.
2. Download the GameCube build (`FCEUltraGX-GameCube.zip` above) and extract it to the root of your SD card.
3. Boot Swiss, launch `fceugx-gc.dol` from your SD Gecko/SD2SP2, and you should land in the same file browser as the other platforms.

```text
SD:/
├── fceugx-gc.dol
└── fceugx/
    └── roms/
        └── ...
```

Note that GameCube does **not** use the `apps/` folder convention that Wii/Wii U do — the `.dol` sits at the SD card root (or wherever your loader expects it), while your ROMs/saves still live in `fceugx/`, same as every other platform.

### Wii U

Wii U support comes in **three genuinely different forms** — pick the one that matches what you've set up on your console:

|                     | vWii (Homebrew Channel) | vWii (VC-style injection) | Native Wii U build |
|---------------------|--------------------------|-----------------------------|----------------------|
| What it is          | The regular **Wii** build, run inside vWii | The same Wii build, launched as its own injected Virtual-Console-style channel | A dedicated Wii U (Aroma) app, `.wuhb` |
| Requires            | Homebrew Channel *inside vWii* | Homebrew Channel *inside vWii*, plus a channel built with **TeconMoon's WiiVC Injector Mod** | **Aroma** (Wii U homebrew environment) |
| CPU                 | Standard vWii clock, 1 core | **Unlocked**, faster single-core clock | Full native Wii U, **4 cores** |
| GamePad             | Not usable | Usable as an **extra controller** (buttons/sticks; with display but no touch) | **Full support** — touch, buttons, second screen |
| Output              | vWii-level, up to 480p | Same as plain vWii | Native, up to **1080p** |
| Upscaling filters   | GX-based (hq2x, Scale2x, 2xBR, DDT) | Same as plain vWii | GX2 shader-based (ScaleFX, Sharp Bilinear) |
| GC/Wii-only settings | **Full access** — Output Mode switching (NTSC/PAL/240p/etc.), Hardware Softening, and other Wii-side [Video](#video) options | Same as plain vWii | Not available — these are Wii/GameCube-specific; the native build gets ScaleFX/1080p in their place instead |
| Which download      | `FCEUltraGX-Wii.zip` (Wii build) | `FCEUltraGX-Wii.zip` (Wii build) | `FCEUltraGX-WiiU.zip` |

If you're not sure which you want: the **native build** is the strongest experience on a console with Aroma installed — full GamePad, four cores, 1080p, and GPU-based upscaling. **VC injection** is the best you'll get out of vWii itself (unlocked CPU and a usable GamePad, at Wii-level output), and plain **Homebrew Channel vWii** is the simplest but weakest of the three.

#### Native Wii U (Aroma)

1. Follow the **[Wii U Homebrew Guide](https://wiiu.hacks.guide/)** to install **Aroma** if you haven't already. One-time setup per console.
2. Download the Wii U build (`FCEUltraGX-WiiU.zip` above) and copy `fceugx.wuhb` to `wiiu/apps/` on your SD card, alongside your other Aroma apps. Also add the `fceugx/` folder from the same download to the SD card root.
3. Insert the SD card and turn on your Wii U — with Aroma installed, **FCE Ultra GX shows up as its own icon directly on the Wii U Menu**, right alongside your other software. No separate app store or launcher step needed; just select it and go.

```text
SD:/
├── wiiu/
│   └── apps/
│       └── fceugx.wuhb
└── fceugx/
    └── roms/
        └── ...
```

Note the extra `wiiu/` nesting compared to Wii: the native Wii U app folder is kept separate from vWii's own `apps/` folder so the two can coexist on the same SD card without colliding.

> ⚠️ **Run the latest Aroma.** This port is only tested against, and only intended to work on, whatever the current Aroma release is at the time you're reading this. We can't promise it'll behave — or even boot — on an old Aroma build or an outdated Wii U system version. If something looks wrong, updating Aroma first is the right move before reporting it.

**Recommended companions, installed through the same Wii U Homebrew Guide:**

- **[Mocha](https://github.com/wiiu-env/MochaPayload)** — an Aroma component that gives Cafe OS access to USB storage (FAT32/exFAT/NTFS). Without it, USB drives simply won't show up as a load/save option on the native build; SD still works fine either way.
- **[Bloopair](https://github.com/GaryOderNichts/Bloopair)** — lets you pair non-Nintendo Bluetooth controllers (Switch Pro Controller, Joy-Con, DualShock/DualSense, Xbox controllers, and others) to your Wii U as if they were a Wii U Pro Controller. Handy if you don't have a GamePad or Pro Controller handy. Bloopair works at the system level within the native Wii U environment and doesn't apply inside vWii.

#### vWii (Wii Homebrew Channel, inside Wii U)

1. Follow the **[Wii Homebrew Guide](https://wii.hacks.guide/)** to install the Homebrew Channel in vWii — the process runs from inside the Wii U's Wii mode and is otherwise the same as on a standalone Wii.
2. Follow the [Wii instructions](#wii) above exactly, using the same SD card — the vWii build is the Wii build.
3. Boot into vWii on your Wii U (from the Wii U Menu) and launch it from the Homebrew Channel, same as on Wii.

This is the simplest Wii U path, but it's also the most limited one: standard vWii clock speed, and no GamePad. For GamePad support and a CPU unlock without going all the way to the native build, see VC-style injection below.

One thing plain (and injected) vWii keep that the native build doesn't: full access to the Wii/GameCube-side **[Video](#video)** settings — Output Mode switching (NTSC/PAL/240p/576p/etc.) and Hardware Softening — since those are tied to the GX video hardware vWii emulates. The native build trades that for GX2-based upscaling (ScaleFX) and native 1080p output instead. See the table above.

#### vWii via VC-style injection (GamePad + unlocked CPU)

Rather than launching FCE Ultra GX from the Homebrew Channel every time, you can package it as its **own injected channel** using **[TeconMoon's WiiVC Injector Mod](https://github.com/timefox/TeconMoon-s-WiiVC-Injector-Mod)**. This installs FCE Ultra GX as a Virtual-Console-style title in your vWii NAND rather than something launched through the Homebrew Channel, which is what unlocks the faster single-core CPU clock and lets you use the Wii U GamePad for display (without touch) and as an extra controller.

At a high level:

1. Install the Homebrew Channel in vWii first (see [vWii](#vwii-wii-homebrew-channel-inside-wii-u) above) — you'll still want it for updates and other homebrew.
2. Download and run **TeconMoon's WiiVC Injector Mod** on a PC, and choose **Wii Homebrew Injection (DOL)** as the injection type.
3. Point it at FCE Ultra GX's `boot.dol` (from the Wii build), and pick one of the available **GamePad Emulation** modes so the injector configures GamePad input for the resulting channel.
4. Build the injected package and install it to your Wii U's vWii NAND with the tool of your choice (the injector's own documentation covers this step, since it depends on your existing vWii setup).
5. Keep the `fceugx/` ROMs/saves folder on your SD card exactly as described [above](#all-platforms-sd-card--folder-layout) — the injected channel reads from the SD card the same way the Homebrew Channel version does.

Consult the injector's own documentation/thread for anything version-specific — like forwarder tooling, this is third-party software this README doesn't track closely.

---

## Initial Setup

The first time you run FCE Ultra GX, it writes a new `settings.xml` next to the app (in `apps/fceugx/` on Wii/GameCube, `wiiu/apps/` on Wii U) to store your configuration. On launch it auto-detects your storage device and drops you straight into the ROM browser — highlight a game and press **A** to load it with default settings, or head into **Settings** first to configure things to your liking.

## Configuration

Press **A** on the **Settings** box from the main menu to open the settings screen, which is split into Button Mappings, Video, Emulation, Saving & Loading, Menu, and Network. **Reset Settings** restores everything to defaults; **Go Back** returns to the ROM browser.

### Button Mappings

Configure the **NES Controller** and **Zapper** independently, each against whichever input devices you have connected (GameCube Controller, Wiimote, Nunchuk+Wiimote, Classic Controller, Wii U Pro Controller, Wii U GamePad). Pick a device to configure, then click each button in turn — FCE Ultra GX will prompt you to press the physical button you want assigned to it. Sensible defaults are already set for every supported input device, so you generally only need this screen if you want something different.

The NES Controller's mappable buttons include the usual **B / A / Select / Start / D-pad**, independent **B (Rapid) / A (Rapid)** bindings for per-button auto-fire (see [Turbo Mode & Rapid Fire](#turbo-mode--rapid-fire)), and a combined **Insert Coin / Switch Disk** button that does VS System coin-insert or FDS disk-switching depending on what's loaded. The Zapper's mappable inputs are **Fire** and **Insert Coin**.

The **Other Mappings** screen (reached from Button Mappings) also lets you configure:

| Option | What it does |
|---|---|
| **Turbo Mode** | On/off |
| **Turbo Mode Button** | Which button/combo triggers Turbo Mode while held (see [Turbo Mode & Rapid Fire](#turbo-mode--rapid-fire)) |
| **Menu Toggle** | Which button/combo brings up the in-game menu |

### Video

| Setting | Options |
|---|---|
| **Output Mode** *(GameCube/Wii only)* | Automatic (recommended), NTSC (480i), Progressive (480p), PAL (50Hz), PAL (60Hz), Progressive (576p), Original (240p) |
| **Aspect Ratio Correction** | None, 16:9 |
| **Cropping** | Off, Vertical, Horizontal, Both — hides overscan pixels the original hardware output but most TVs never showed |
| **Palette** | Default (FCEUX built-in), plus 11 curated presets: Digital Prime, Smooth, PVM Style D93, Composite Direct, Magnum, NES Classic, PC-10, Sony CXA, Wavebeam, PAL, Restored Wii VC, and Wii Virtual Console |
| **Bilinear Filtering** | On/Off — has no effect when Sharp Bilinear is selected as the Wii U upscaling filter, since that filter does its own filtering |
| **Hardware Softening** *(GameCube/Wii only)* | Off, Auto, Sharp, Soft |
| **Upscaling** | GameCube/Wii: None, hq2x, hq2x Soft, hq2x Bold, Scale2x, 2xBR, 2xBR-lv1, DDT · Wii U: None, ScaleFX, Sharp Bilinear |
| **Scanline Overlay** | On/Off |
| **Screen Zoom** | Adjust horizontal/vertical zoom with the left/right arrows; 100% is default |
| **Screen Position** | Nudge the output with the on-screen arrows if it isn't centered on your display |

### Emulation

| Option | Notes |
|---|---|
| **Game Timing** | Automatic (recommended), NTSC, PAL, or Dendy — sets the console timing FCEUX emulates; Automatic detects it from the loaded ROM |
| **Sprite Limit** | On by default, matching the real NES/Famicom's 8-sprite-per-scanline limit — the cause of the sprite flicker some games show on real hardware. Turning it off removes that limit, eliminating the flicker at the cost of hardware accuracy |
| **Zapper Crosshair** | On/Off — shows an on-screen crosshair for Zapper light-gun aiming |

### Saving & Loading

| Option | Options |
|---|---|
| **Load Device** | SD, USB, DVD, Network, Auto |
| **Save Device** | SD, USB, Network, Auto |
| **Load Folder** | Opens an on-screen keyboard to set a custom ROM folder |
| **Save Folder** | Opens an on-screen keyboard to set a custom save folder |
| **Cheats Folder** | Opens an on-screen keyboard to set a custom cheats folder |
| **Screenshots Folder** | Opens an on-screen keyboard to set a custom screenshots folder |
| **Covers Folder** | Opens an on-screen keyboard to set a custom covers folder |
| **Artwork Folder** | Opens an on-screen keyboard to set a custom artwork folder |
| **Auto Load** | Off, RAM, State |
| **Auto Save** | Off, RAM, State, Both |
| **Append Auto to .SAV Files** | On/Off — when on, auto-created RAM saves get an " Auto" suffix so they don't overwrite a save you made manually |

FCE Ultra GX has two kinds of saves: **RAM** saves, the in-game battery save (only applicable to games that support it), and **save states**, real-time snapshots that capture exactly where you are and let you resume later.

### Menu

| Option | Options |
|---|---|
| **Exit Action** | Return to Loader, Return to Wii Menu, Power Off |
| **Wiimote Orientation** | Vertical, Horizontal |
| **Music Volume** / **Sound Effects Volume** | |
| **Rumble** | Enabled/Disabled — on Wii U, the GamePad uses a shorter, reduced-amplitude pattern than the Wiimote |
| **Language** | See [Language & Custom Fonts](#language--custom-fonts) |
| **Preview Image** | See [Artwork](#artwork) |
| **Hide RAM Saving** | Hides the RAM save option in the in-game save menu, for games where you only ever use save states |

### Language & Custom Fonts

Set the menu language under **Settings → Menu → Language**. Supported: English, Japanese, German, French, Spanish, Italian, Dutch, Chinese (Simplified), Korean, Portuguese, Brazilian Portuguese, Catalan, Turkish, and Swedish.

The built-in font only covers Latin-script languages. For **Japanese, Korean, or Chinese**, you also need to supply a matching font file yourself — this repository ships them in the [`fonts/`](https://github.com/dborth/fceugx/tree/master/fonts) folder:

| Language | Font file |
|---|---|
| Japanese | `jp.ttf` |
| Korean | `ko.ttf` |
| Chinese (Simplified) | `zh.ttf` |

Copy the matching `.ttf` into your app folder — `apps/fceugx/` on Wii, `wiiu/apps/` on Wii U — alongside `boot.dol` / `fceugx.wuhb`, then select that language from the menu; it switches fonts automatically once both are in place.

> This is a **Wii and Wii U only** feature — the GameCube build can't load external fonts, so Japanese/Korean/Chinese text won't render correctly there.

### Artwork

Cover art, screenshots, or general artwork can be shown on the main menu when a game is highlighted. Pick which one to display under **Settings → Menu → Preview Image**. Each image lives in its matching folder (`fceugx/covers`, `fceugx/screenshots`, `fceugx/artwork`) and must be a PNG named exactly the same as the ROM (e.g. `Excitebike.png` for `Excitebike.nes`), no larger than 640×480. **256×240** — the NES's own native resolution — is the recommended size.

### Network Shares (SMB)

To load or save over your LAN, enter your SMB share settings under **Settings → Network**: **IP**, **Name** (the share name), **Username**, and **Password**.

> 🔑 If your SMB share doesn't have a password, **leave the Password field blank** — don't type anything into it. An empty password connects as guest.

Network Shares (SMB) uses `libsmb2` on all three platforms, with the SMB dialect auto-negotiated (SMB2/3) rather than hardcoded — meaning it talks to modern Windows/Samba shares out of the box. One share can be connected at a time.

---

## File Browser

The File Browser loads automatically on startup and lists the contents of your `fceugx/roms` folder (or wherever you've pointed Load Folder — see [Saving & Loading](#saving--loading)). Click a game — uncompressed, or zipped in a `.zip`/`.7z` archive — to load it, or click **Up One Level** to navigate.

## Gameplay

Press **Home** while playing to open the in-game menu (Save, Load, Reset, Controller, Cheats). Select **Main Menu** to return to the File Browser, or **Close** to resume play.

- **Save** offers options to create a new RAM save or a new save state; click either to create one, or click an existing save to overwrite it.
- **Load** loads a saved RAM save or save state.
- **Reset** resets the current game.
- **Controller** toggles which controller drives the game.
- **Cheats** toggles your loaded cheat codes (below).

## Cheats & Game Genie

Cheats load from `fceugx/cheats` in the `.cht` file format, and must be named exactly the same as the ROM they apply to (e.g. `Excitebike.nes` needs `Excitebike.cht`).

FCE Ultra GX also supports **Game Genie** codes, which need a Game Genie patch ROM to work: download a `gg.rom` file and place it directly in your `fceugx` folder (alongside `disksys.rom`, not in a subfolder). With that in place, a **Game Genie** option appears in the in-game menu where you can enter and toggle codes.

## Turbo Mode & Rapid Fire

FCE Ultra GX has two distinct speed-up features that are easy to mix up:

- **Turbo Mode** roughly doubles overall playback speed while active — game logic, scrolling, everything. It's remappable — set which button or combo triggers it under **Settings → Button Mappings → Other Mappings → Turbo Mode Button** — and can be disabled entirely from the same screen. Hold the assigned button to run at double speed; release it to return to normal playback.
- **Rapid Fire** (the classic NES-style "turbo" feature) only affects a single button — **B (Rapid)** and **A (Rapid)** are separate bindings under [Button Mappings](#button-mappings) that auto-fire that specific button for as long as it's held, at normal game speed. This is the equivalent of a third-party "turbo controller," useful for shooters and other rapid-tap games, and has nothing to do with overall playback speed.

## Famicom Disk System (FDS)

To play Famicom Disk System games (`.fds`), you need an FDS BIOS file. Download `disksys.rom` (8KB) and place it directly in your `fceugx` folder (not in `roms/`). Once it's there, `.fds` games load like any other ROM.

Use the **Insert Coin / Switch Disk** button (configurable under [Button Mappings](#button-mappings)) to switch disk sides on multi-disk FDS games while playing.

## VS System

VS System arcade games are supported directly — load them like any other ROM. The same **Insert Coin / Switch Disk** button used for FDS disk-switching also inserts a coin for VS System games that need one to continue.

## Famicom 3D System & Anaglyph Games

FCE Ultra GX includes special handling for games designed around the **Famicom 3D System** (a red/blue anaglyph 3D peripheral) and other anaglyph-rendered NES/Famicom games, rendering them with a red/blue 3D effect you can view with a pair of anaglyph glasses. This support is inherited from the underlying FCEUX core and applies automatically to games it recognizes — no separate toggle is needed.

## Importing and Exporting RAM Saves

FCE Ultra GX can load `.sav` RAM saves created by FCEUX (or compatible emulators) on other platforms (Mac/PC/Linux/etc.), and vice versa.

- **To import**, make sure the `.sav` file's name matches your ROM's filename (aside from the extension).
- **To export**, copy the save from your console's `fceugx/saves` folder over to the other platform — you may need to rename it to whatever that FCEUX build expects.

---

## Credits

| Role | Credit |
|---|---|
| Coding & menu design | Tantric |
| Menu artwork | the3seashells |
| Menu sound | Peter de Man |
| Logo design | mvit |
| Additional updates/fixes | Zopenko, Burnt Lasagna, Askot, bladeoner |
| FCE Ultra GX GameCube | SoftDev, askot & dsbomb |
| FCE Ultra | Xodnizel |
| Original FCE | BERO |
| libogc / devkitPPC | shagkur & WinterMute |

## Links

- [FCE Ultra GX Project Page](https://github.com/dborth/fceugx)
- [FCEUX](https://fceux.com) — the core FCE Ultra GX is built on
- [Wii Homebrew Guide](https://wii.hacks.guide/)
- [Wii U Homebrew Guide](https://wiiu.hacks.guide/)
- [gc-forever.com — GameCube homebrew/hardware hub](https://www.gc-forever.com/)
- [Swiss](https://github.com/emukidid/swiss-gc) — the recommended GameCube loader
- [TeconMoon's WiiVC Injector Mod](https://github.com/timefox/TeconMoon-s-WiiVC-Injector-Mod)
- [Mocha](https://github.com/wiiu-env/MochaPayload) — USB storage access for the native Wii U build
- [Bloopair](https://github.com/GaryOderNichts/Bloopair) — Bluetooth controller pairing for the native Wii U build
- [Change History (CHANGELOG.md)](CHANGELOG.md)
