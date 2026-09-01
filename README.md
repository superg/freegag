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

Download the archive for your platform from [GitHub Releases](https://github.com/superg/freegag/releases) and extract it. Windows and Linux packages include FreeGAG and `xtet_resource_extractor`; the macOS ZIP contains `FreeGAG.app` and the extractor CLI beside it.

FreeGAG does not include, download, or delete original game data. On first launch, open **Settings** and choose a prepared folder containing data you are entitled to use or import the data from your original discs. A valid configured folder starts automatically on later launches.

### Guided disc import

Choose **Import from discs…** under **Settings → Game assets**, then:

1. Choose a destination folder.
2. Select all `.CDF` files from the game discs.
3. Select the original `XTETDLL.DLL`.
4. Review the detected game and start the import.

FreeGAG copies source files asynchronously and extracts `XTETDLL.SFS`. It stages and validates the complete result before configuring it as the game asset folder. A different existing destination file is never overwritten, and cancellation removes only temporary files created by that import.

The standalone extractor remains available for manual preparation:

1. Copy all `.CDF` files from the game discs into a dedicated data folder.
2. Open a terminal in that folder.
3. Run `xtet_resource_extractor`, passing the absolute path to the original `XTETDLL.DLL` on your game disc:

```text
xtet_resource_extractor "<absolute path to XTETDLL.DLL>"
```

The command extracts the asset archive as `XTETDLL.SFS` into the current directory. Keep it beside the `.CDF` files. FreeGAG does not use `XTETDLL.DLL` itself.

For GAG: The Impotent Mystery:

```text
GAG-data/
├── GAG01.CDF
├── GAG02.CDF
└── XTETDLL.SFS
```

Copy any additional numbered `.CDF` archives included with your edition into this same directory.

For GAG+: Harry on Vacation:

```text
GAG-plus-data/
├── GARY.CDF
└── XTETDLL.SFS
```

Files in the game disc's `GAG3/` directory are not needed because GAG+: Harry on Vacation does not use them.

### Prepared folders

Choose **Settings → Game assets → Choose…** and select a data folder. FreeGAG remembers one external asset folder. It validates names case-insensitively, opens every CDF with FreeGAG's parser, and mounts `XTETDLL.SFS`; successful validation closes Settings and launches the game automatically. GAG requires `GAG01.CDF`, `GAG02.CDF`, and `XTETDLL.SFS`; GAG+ requires `GARY.CDF` and `XTETDLL.SFS`. Additional numbered CDF archives are accepted. A folder containing both GAG and GAG+ roots is rejected as ambiguous.

When no valid folder is configured, the compact main window says **Open Settings for configuration.** On Windows and Linux it also provides an **Open Settings** button; F10 and the configured controller shortcut work there on every platform. macOS additionally provides native **Settings…** and **Controls…** commands in the application menu.

## Configuration

The configured asset folder keeps its compatible `freegag.ini` and saves beside the game data. On macOS, choose **FreeGAG → Settings…** or **FreeGAG → Controls…** in the system menu bar; **Cmd+,** opens Settings. On Windows and Linux, use the **Open Settings** button before startup or F10 while playing. The F10 host binding opens the same separate preferences window on every platform. A running game pauses until that window closes. Choosing, importing, or revalidating game assets launches them automatically when no game is running; changing to another valid folder restarts an active game with that folder. Fullscreen, scaling, sound, and subtitles apply immediately; low-color resources require **Restart Game**.

Section and setting names are case-sensitive. Boolean values may be written as `true` or `false` in any capitalization.

```ini
[Game]
Fullscreen=false
IntegerScaling=false
LowColorResources=false
Sound=true
Subtitles=false
```

| Setting | Default | Available in-game | Description |
| --- | --- | --- | --- |
| `Fullscreen` | `false` | Yes | Starts the game in fullscreen mode. |
| `IntegerScaling` | `false` | Yes | Uses aspect-preserving fractional scaling to fill the nearest window edge. Set it to `true` for whole-number scaling with additional letterboxing. |
| `LowColorResources` | `false` | Yes, restart required | Selects the original low-color resource variants where available. The framebuffer remains true-color. |
| `Sound` | `true` | Yes | Enables sound and music. |
| `Subtitles` | `false` | Yes | Displays subtitles for spoken comments. |

The optional `[Window]` section contains the last window position as `Left`, `Top`, `Right`, and `Bottom` desktop coordinates. FreeGAG maintains these values automatically, so they normally do not need to be edited.

## GAGBoy

GAGBoy is a handheld minigame included as part of the main game. The `--gagboy` command-line option launches it directly.

## Input

The configuration screen and settings UI support full keyboard and controller navigation. Adventure and GAGBoy share one contextual game-control scheme, while application shortcuts remain separate. A duplicate inside either group replaces the previous assignment after warning the user.

| Group | Keyboard defaults | Controller defaults |
| --- | --- | --- |
| Game Controls | Arrows move the pointer; in GAGBoy, Left/Right/Down move and Up rotates. Z is primary/rotate, X is secondary/exit, Space is hard drop, and P is pause. Escape also exits GAGBoy. | Left stick/D-pad moves the pointer; in GAGBoy, Up rotates. South/A is primary/rotate, East/B is secondary/exit, West/X is hard drop, and Start is pause. |
| Application Shortcuts | F10 or Ctrl/Cmd+, settings; F9 stops the game | Guide/Home or L1+R1 settings; Back stops the game |

Pointer speed, analog dead zone, connected-controller status, key/button capture, contextual reset, and controller hot-plugging are available under **Controls**. Pointer bindings are suspended during save-name and other adventure text entry.

## Frontend preferences and command line

The configured asset root, input mappings, and UI preferences are stored in `frontend.json` under SDL's per-user preference directory for organization `superg` and application `FreeGAG`. Typical locations are `%APPDATA%\superg\FreeGAG` on Windows, `~/Library/Application Support/superg/FreeGAG` on macOS, and `$XDG_DATA_HOME/superg/FreeGAG` or `~/.local/share/superg/FreeGAG` on Linux. Writes use a temporary file and atomic replacement; malformed JSON produces a visible warning and safe defaults.

Normal startup launches the configured valid asset folder. Direct launch and automation can override it with:

```text
FreeGAG --data-dir "<game-data-folder>"
FreeGAG --data-dir "<game-data-folder>" --gagboy
```

`--gagboy` by itself uses a valid data root beside the executable or in the current directory; otherwise the configuration prompt opens and Settings displays the validation message.

## Acknowledgments

Special thanks to **Yaroslav Kemnits**, the original designer and art director of GAG, for sharing unique builds and a localization disc, and for his invaluable guidance throughout this project. And most of all, thank you for creating GAG!

## License

FreeGAG is licensed under the [GNU General Public License v3.0](LICENSE). This license covers the engine source code, not the original game data.
