# FreeGAG

<p align="center">
  <img width="845" height="1052" alt="FreeGAG cover" src="https://github.com/user-attachments/assets/001f101e-01dc-4b96-97dc-3c028e3255f8">
</p>

This is an open source engine implementation of ГЭГ: Отвязное приключение (GAG: The Impotent Mystery) and ГЭГ+: Гарри в отпуске (GAG+: Harry on Vacation)

FreeGAG is an open-source engine implementation for the point-and-click adventure games [ГЭГ: Отвязное приключение](https://en.wikipedia.org/wiki/GAG_The_Impotent_Mystery) (GAG: The Impotent Mystery) and ГЭГ+: Гарри в отпуске (GAG+: Harry on Vacation).

FreeGAG was created by reverse engineering the original game engine and reimplementing its behavior as portable, open-source code.

I started this project more than 12 years ago, although it spent much of that time on the back burner. After returning to it recently, I completed the port to SDL. FreeGAG now supports modern platforms, native 64-bit builds, high-resolution output, integer scaling, windowed and fullscreen modes, a true-color framebuffer, high-quality audio mixing, and native scripted save and load dialogs.

## Current status

FreeGAG has been tested on:

- Windows x64
- Linux x64
- Linux ARM64
- macOS Universal

Both supported games and all available language builds have been tested. The original game can be played from beginning to end, and the known engine bugs found during testing have been fixed.

Although the engine is functionally complete, development is ongoing. The source code continues to be cleaned up, and bugs are fixed as they are found. Bug reports and any other feedback are welcome through [GitHub Issues](https://github.com/superg/freegag/issues).

## Playing the games

Download the archive for your platform from [GitHub Releases](https://github.com/superg/freegag/releases) and extract it. The `freegag` and `xtet_resource_extractor` executables are in the extracted directory.

FreeGAG does not include the original game data. You can prepare data from your own game discs or use one of the complete prepared game-data packages.

### Option 1: use your own game discs

1. Copy all `.CDF` files from the game discs into the same directory as the FreeGAG executable.
2. Open a terminal in that directory.
3. Run `xtet_resource_extractor`, passing the absolute path to the original `XTETDLL.DLL` on your game disc:

```text
xtet_resource_extractor "<absolute path to XTETDLL.DLL>"
```

The command extracts the asset archive as `XTETDLL.SFS` into the current directory. Keep this file beside the FreeGAG executable and the `.CDF` files. FreeGAG does not use `XTETDLL.DLL` itself, so you do not need to copy the DLL from the disc.

For GAG: The Impotent Mystery:

```text
FreeGAG/
├── freegag.exe
├── xtet_resource_extractor.exe
├── GAG01.CDF
├── GAG02.CDF
└── XTETDLL.SFS
```

Copy any additional numbered `.CDF` archives included with your edition into this same directory.

For GAG+: Harry on Vacation:

```text
FreeGAG/
├── freegag.exe
├── xtet_resource_extractor.exe
├── GARY.CDF
└── XTETDLL.SFS
```

Files in the game disc's `GAG3/` directory are not needed because GAG+: Harry on Vacation does not use them.

### Option 2: download a complete prepared game-data package

1. Visit the [FreeGAG game-assets download page on MEGA](https://mega.nz/folder/TFMUFTia#qa5OQ0C4FybTY8_ZlwWv6w).
2. Download the complete game-data package for the language variant you want to play.
3. Extract its contents into the same directory as the FreeGAG executable.

### Start the game

Run the `freegag` executable.

FreeGAG detects the game from its primary archive: `GAG01.CDF` starts GAG: The Impotent Mystery, while `GARY.CDF` starts GAG+: Harry on Vacation. If both are present in the same directory, `GARY.CDF` takes precedence.

## Configuration

FreeGAG creates `freegag.ini` beside the executable and updates it when settings change. Close the game before editing the file so your changes are not overwritten on exit.

Section and setting names are case-sensitive. Boolean values may be written as `true` or `false` in any capitalization.

```ini
[Game]
Fullscreen=false
IntegerScaling=true
LowColorResources=false
Sound=true
Subtitles=false
```

| Setting | Default | Available in-game | Description |
| --- | --- | --- | --- |
| `Fullscreen` | `false` | Yes | Starts the game in fullscreen mode. |
| `IntegerScaling` | `true` | No | Uses whole-number scaling for crisp pixels. Set it to `false` for aspect-preserving fractional scaling with letterboxing. |
| `LowColorResources` | `false` | No | Selects the original low-color resource variants where available. The framebuffer remains true-color. |
| `Sound` | `true` | Yes | Enables sound and music. |
| `Subtitles` | `false` | Yes | Displays subtitles for spoken comments. |

The optional `[Window]` section contains the last window position as `Left`, `Top`, `Right`, and `Bottom` desktop coordinates. FreeGAG maintains these values automatically, so they normally do not need to be edited.

## GAGBoy

GAGBoy is a handheld minigame included as part of the main game. For convenience, it can also be launched directly by passing `--gagboy` to the FreeGAG executable.

## Acknowledgments

Special thanks to **Yaroslav Kemnits**, the original designer and art director of GAG, for sharing unique builds and a localization disc, and for his invaluable guidance throughout this project. And most of all, thank you for creating GAG!

## License

FreeGAG is licensed under the [GNU General Public License v3.0](LICENSE). This license covers the engine source code, not the original game data.
