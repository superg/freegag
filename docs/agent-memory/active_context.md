# Active context

Last updated: 2026-08-14

## Build preference

- The user's active development configuration is Win32 Debug. Use Debug for
  routine builds and verification; do not build Release unless explicitly
  requested or investigating release-specific behavior.

## Version control

- The workspace root is an initialized Git repository with unborn branch
  `main`. No files have been staged and no commits exist yet.
- Git reports dubious ownership under the agent execution account because the
  workspace belongs to a different Windows SID. Verification used a command-local
  `safe.directory` override; global Git configuration was not changed.
- README work is intentionally out of scope at this stage. Leave the existing
  README untouched and do not add README content unless requested later.
- Root `.gitignore` excludes `.vscode/launch.json`, generated `build/`, game
  `data/`, `docs/commands_manual_en.md`, and the local `ghidra/` project.
- Root `.gitattributes` overrides per-user `core.autocrlf` settings: all detected
  text is stored and checked out as LF on Windows, Linux, and macOS. The current
  Windows account has `core.autocrlf=true`, which caused the prior warning before
  this policy existed.

## Formatting preference

- The root `.clang-format` is the source of truth for C++ formatting. Notable
  settings are four spaces with no tab characters, Allman braces, no spaces
  before opening parentheses, right-aligned pointer/reference markers, and a
  200-column limit.
- Root CMake now mirrors redumper's `format` and `check-format` targets. It finds
  `clang-format`, formats/checks C++ sources under `src/` and `tools/`, excludes
  generated `build/` files by construction, and warns when clang-format is not
  installed.
- CMake configuration uses clang-format 18.1.6. The source tree has now been
  formatted, and both `format` and `check-format` succeed.
- WinMM's `<mmsystem.h>` requires Windows base declarations first. A dedicated
  include category in `.clang-format` keeps `<windows.h>` ahead of other system
  headers; without it, case-sensitive alphabetical sorting broke the build.
- The fully formatted Win32 Debug build succeeds.

## Current objective

Build a standalone, minimal 32-bit Win32 host that loads `XTETDLL.DLL`, runs the
XTET minigame, displays it correctly, forwards input, and captures its result.

## Governing reverse-engineering directive

- Do not guess missing behavior or data. Recover it from `GAG.EXE`,
  `XTETDLL.DLL`, their embedded resources, or controlled runtime evidence.
- In particular, do not substitute plausible palettes, display modes, ABI fields,
  or callback behavior for the original implementation.
- Treat the current halftone palette only as an initialization/diagnostic artifact;
  it is not evidence of original behavior and must not be the final rendering path.

## Current implementation

- `src/main.cpp` creates a 640x480 top-down 8-bit DIB and a Win32 message loop.
- The DLL is loaded dynamically and exports are resolved by ordinals 1, 2, and 3.
- The loader supplies the six callback entries referenced directly by XTET:
  dirty-region invalidation plus five functional WinMM audio operations.
- XTET result descriptors sent through message `0x7ffc`/`lParam == 0x40` are
  copied synchronously. `lParam == 0` ends a session; `lParam == 1` indicates
  initialization failure.
- CMake rejects non-32-bit configurations and copies `data/orig/XTETDLL.DLL`
  beside the built executable.
- The latest Release build succeeds with MSVC Win32.

## Rendering state

- Geometry is verified at 640x480. A 320x240 host clips the artwork to its
  upper-left quarter; the DLL does not scale it.
- XTET always renders indexed 8-bit pixels into the supplied framebuffer. It does
  not read the host context's bits-per-pixel field. Original high-color operation
  is implemented by `GAG.EXE` converting indexed pixels through a palette lookup.
- `XTETDLL.DLL` embeds an `XTETSFS` `RT_RCDATA` resource. It mounts that SFS archive
  and loads the static background and sprites as separate 8-bit BMP assets.
- Each loaded BMP owns a 1,024-byte, 256-entry RGBQUAD palette block.
- The loader initially chose the most frequent palette, then the palette attached
  to an internal 640x480 XTET bitmap. Neither is the original host behavior.
- `IVIEW.CIN` plan `VGB0` initially loads `XTET01.BMP` as `PRIMARY`, but cartridge
  insertion replaces it with `VGB1`. On the first play, `SPrise::Pick_Up` is off,
  so `NoSP` selects `VE-GBNEW.BMP` as the active `PRIMARY` image.
- GAG code at `0x00428027` passes every active PRIMARY image's palette at image
  offset `+0x1c` to `ApplyDisplayPalette`, even when `NOPAL` is set. `NOPAL`
  bypasses pixel-index remapping; it does not suppress primary-palette adoption.
- The palette source at first XTET launch is therefore
  `data/Gag01/VE-GBNEW.BMP`: 256 RGBQUAD entries at file offset `0x36`, length
  `0x400`, palette SHA-256
  `97817BD571C343CAA6A9D76318BB3163EAB7C47FD70AB630C2CEC762FE995E47`.
- GAG's `NOPAL` flag is `0x04000000`. Static analysis confirms it skips the
  256-entry palette-index remap and selects raw-index behavior for 8-bit blits.
  The standalone loader must therefore use the `XTET01.BMP` palette rather than
  discovering a palette inside `XTETDLL.DLL`.
- `src/main.cpp` now resolves the loaded DLL's actual filesystem path and requires
  `VE-GBNEW.BMP` in that same directory. Missing, unreadable, malformed, non-8-bit,
  non-640x480, or incomplete-palette files produce a fatal message before the
  DLL initializer is called.
- The loader reads all 256 RGBQUAD entries directly from the verified BMP and
  installs them into the DIB color table. All generated/halftone initialization
  and XTET heap-palette discovery have been removed.
- CMake copies `data/Gag01/VE-GBNEW.BMP` beside `XTETDLL.DLL` and `xtet_loader.exe`.
  The copied Release asset is byte-identical to the extracted source file.
- Runtime visual verification confirms that the `VE-GBNEW.BMP` palette renders
  both the XTET cabinet/background and gameplay sprites correctly. The palette
  issue is resolved for the initial `NoSP` minigame launch state.
- Runtime visual verification showed that installing the `XTET01.BMP` RGBQUAD
  table directly is still incorrect: the cabinet is posterized and the playfield
  is largely black. This disproves the assumption that XTET's output indices map
  directly to the file's palette entries; GAG applies an additional index/palette
  transformation in its graphics/presentation path.
- No `XTET01.256`, nor any XTET-named `.256` file, exists on either extracted
  disc. `XTET01.BMP` itself is an uncompressed 640x480, 8-bit indexed BMP.

## Confirmed DLL ABI

- Ordinal 1 `GAME_DLL_INIT`: `ECX = GameHostContext*`,
  `EDX = 35-entry callback table`.
- Ordinal 2 `GAME_DLL_WND_PROC`: `ECX = HWND`, `EDX = message`, with `WPARAM` and
  `LPARAM` passed on the stack.
- Ordinal 3 `GAME_DLL_EXEC`: command in `ECX`; command `1` terminates.
- Host context is 0x40 bytes. Confirmed consumed fields are `HWND` at `+0x00`,
  width/height at `+0x20/+0x22`, and framebuffer pointer at `+0x2c`.

## Immediate next steps

1. Continue reverse-engineering the next XTET behavior selected by the user.

## Audio state

- XTET loads standard RIFF/WAVE data internally. Its wave object starts with a
  16-byte PCM format block and stores the raw PCM payload at object offset
  `+0x10`, with its byte length at `+0x0c`.
- Callback slot 1 receives a pointer to that 16-byte format block in `ECX` and
  returns a sound handle. The block contains PCM format tag, channel count,
  sample rate, average byte rate, block alignment, and bits per sample.
- Callback slot 2 destroys the handle. Slot 3 receives handle in `ECX`, raw PCM
  pointer in `EDX`, then byte count and replace flag on the stack. Slots 4 and 5
  receive the handle in `ECX` and restart/reset flags respectively.
- GAG's queue stores PCM descriptors without copying sample bytes. Queues remain
  attached to a handle across start/stop operations; replace clears the old
  queue, restart rewinds it, and stop-without-reset pauses it.
- XTET's loop setup queues the same raw sample sequence repeatedly (300 passes),
  so the callback is a persistent PCM stream queue, not a whole-WAV playback API.
- `src/main.cpp` implements each sound handle with an independent WinMM
  `waveOut` stream using the exact format XTET passes. It preserves descriptors
  and supports replacement, restart/rewind, and pause/resume. CMake links
  `winmm`.
- The first implementation submitted all 300 loop repetitions to `waveOutWrite`
  immediately. Runtime testing produced a short opening fragment followed by
  silence, while finite pause/game-over sounds played normally.
- GAG's `InitializeWaveOutMixer` at `0x00401330` proves the original engine uses
  exactly two output WAVEHDR buffers. The loader now likewise keeps at most two
  buffers in flight and replenishes them from the persistent descriptor queue on
  `MM_WOM_DONE`. Reset headers are retained to prevent delayed completion
  messages from aliasing newly allocated headers.
- MSVC Win32 Debug compiles successfully. Interactive verification of the
  bounded-streaming correction failed: the opening audio still stops after
  approximately one second, while pause/game-over audio remains normal.
- The Debug loader now emits targeted `XTET audio ...` messages for PCM formats,
  logical queue depth, each early/milestone device submission and completion,
  resets, and every WinMM failure result. The next runtime trace must establish
  whether completion messages arrive and whether the third logical descriptor is
  submitted; no further audio behavioral change should be made before that
  evidence is collected.
- The captured trace proves XTET creates one 11025 Hz, stereo, 16-bit loop handle
  and queues 2,402 borrowed PCM descriptors. The first two descriptors are
  300,608 bytes each; no WOM_DONE occurs during the reported startup fragment.
- End-to-end call sequence plus runtime comparison with the original game proves
  the loader's last two callback entries were reversed. XTET invokes slot 4
  before building the 2,402-entry loop queue and on pause entry; it invokes slot
  5 after initialization and on unpause. Original behavior establishes slot 4 as
  STOP and slot 5 as START.
- The prior conclusion that `WaveLoopObjLink` was pause-only audio was incorrect.
  The original game starts this music in normal gameplay, stops it while paused,
  and resumes it on unpause. The loader now binds callback 4 to `sound_stop` and
  callback 5 to `sound_start`.
- The GAG project's currently documented slot-4/slot-5 function interpretation
  appears opposite to the verified end-to-end behavior and remains an explicit
  reverse-engineering discrepancy to reconcile; do not use it to flip the loader
  back without new evidence.
- The loader defers device submissions until ordinal 1 returns. Runtime
  verification confirms this removed the one-second initialization artifact.
  Win32 Debug builds successfully.
- Runtime verification confirms the complete audio path now behaves like the
  original game: music starts in normal gameplay, stops on pause, resumes on
  unpause, and game-over/SFX playback works. The audio task is resolved.
- GAG's StartSound/StopSound flags reset per-handle scheduling metadata only;
  they do not rewind or discard PCM descriptor nodes. The loader and Ghidra
  comments have been corrected accordingly.
- Ghidra now names the five XTET callback globals, `LoadWavePcmData`,
  `InitializeLoopingSoundQueue`, and `QueueRandomSoundSample`; GAG's
  `CreateSoundHandle` prototype, `InitializeWaveOutMixer`,
  `HandleWaveOutCallback`, XTET's `SetLoopMusicPlaying`, and all relevant callback comments document the
  recovered register/stack ABI and two-buffer streaming architecture. Both
  programs were saved.

## Matched-figurine effect investigation

- The DLL does not omit the effect. `FindMatchCandidate` (`0x1001fc80`) locates a
  compatible nearby figurine. The movement-update path at `0x1001e8b0` then queues
  one sample from the list at `0x1003c758`, calls `AnimateMatchedPair`
  (`0x1001ea30`) synchronously, and only afterward removes/updates both objects.
- `AnimateMatchedPair` writes multiple intermediate states into the shared indexed
  framebuffer and reports them through callback slot 0 before returning.
- Loader callback slot 0 currently calls only `InvalidateRect`. Since the matched
  effect runs synchronously on the message thread, `WM_PAINT` cannot run between
  those calls. All intermediate invalid regions coalesce and the loader displays
  only the final post-removal framebuffer.
- GAG callback slot 0 is `InvalidateGameFramebufferRect` (`0x00427830`). It enters
  the original graphics surface-update transaction, submits the changed bounds,
  and releases that transaction; it is not merely a deferred Win32 invalidation.
- Match audio uses the sound list at `0x1003c758`. XTET calls only queue-with-replace
  for this one-shot and does not call either audio-control callback for its handle.
  GAG's mixer consumes queued descriptors independently of the control flag. The
  loader instead submits from `sound_queue` only when `SoundHandle::playing` is
  true, so the newly queued match sample is retained but never sent to waveOut.
- The loader now presents callback-slot-0 rectangles immediately with `GetDC` and
  `BitBlt`, while retaining `WM_PAINT` for ordinary repaint/exposure handling.
- New sound handles now begin active, reproducing GAG's queue-started one-shots.
  XTET explicitly stops the loop handle before constructing its queue, so music
  initialization and later pause/resume retain their recovered behavior.
- Ghidra now names `FindMatchCandidate`, `AnimateMatchedPair`,
  `LoadActionDefinitions`, and labels `g_pMatchSoundList`, with comments on the
  exact match/effect and original framebuffer-update paths. Both programs were
  saved.
- Formatting and `check-format` pass, and the complete MSVC Win32 Debug build
  succeeds.
- Runtime verification confirms the complete matched-figurine effect now works:
  the intermediate animation is visible, its one-shot sound plays, the figurines
  disappear afterward, and the previously working loader behavior remains intact.

## XTET script launch trace

- Both discs contain byte-identical `IVIEW.CIN` and relevant XTET view assets.
- `VGAGBOY` selects `VGB0` while `Catridg::Use` is off and `VGB1` after cartridge
  insertion. In `VGB1`, the initial `NoSP` state selects `VE-GBNEW.BMP` as the
  primary palette-owning image immediately before `/GAME` executes.
- `VGB1` launches
  `/GAME:xtetdll.dll:GAGBoy::Score`. GAG opcode analysis confirms that
  `GAGBoy::Score` is not an initialization argument: it is the script state field
  that receives the result captured from the DLL after termination.
- `NEWGAME.CFG` initializes `GAGBoy::Score` to `0` and `GAGBoy::Win` to `-65`.
  After the minigame, the script adds `Win` to `Score` and tests whether the
  result remains positive, establishing a required raw score of at least 66.
- The GAME handler loads the named DLL and calls its ordinal-1 initializer with
  only the already documented host context and callback table. Palette state is
  owned by GAG's graphics environment, not passed through the DLL ABI.
- Ghidra now contains names/comments for the script opcode parser, image-flag
  parser/serializer, palette-index remapper, optional-remap blitter, state-field
  resolver, GAME command executor, and the captured result globals.

## CDF extractor

- The historical extractor was copied from
  `D:\projects\freegag\trunk\src\tools\cdf_extractor` into
  `tools/cdf_extractor`; the source location was only read, not modified.
- Its Boost.Filesystem dependency was replaced with C++17 `std::filesystem`.
  The original bundled inflate implementation remains local to the tool.
- The root build includes a separate `cdf_extractor` console target. Its legacy
  `inflate.c` file is compiled as C++ because it uses C++ headers and constructs.
- MSVC Win32 Release builds cleanly. A one-file pack/extract round trip produced
  byte-identical output.
- Usage is `cdf_extractor x <cdf-file> <directory>` for extraction and
  `cdf_extractor p <cdf-file> <directory>` for packing.

## Script-system documentation

- `docs/CommandsManual.DOC` is plain UTF-8 Russian text despite its `.DOC`
  extension. It is a technical description supplied by the original developer.
- `docs/CommandsManual.en.md` is an English companion translation. Script tokens
  and examples are preserved verbatim; the `/MOVI` source inconsistency is called
  out rather than resolved without parser or runtime evidence.
