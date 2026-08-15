# Learned patterns

This file records verified, reusable architectural patterns. Append new patterns;
do not replace prior entries without correcting a demonstrated error.

## 2026-08-14 — DLL boundary and rendering

- The minigame boundary uses ordinal exports and an x86 `__fastcall` ABI rather
  than ordinary stack-only Win32 callbacks. Function-pointer calling conventions
  must therefore be declared explicitly in any compatible host.
- Result state is returned by synchronous private window messages, so pointed-to
  payloads must be copied inside the receiving window procedure before
  `SendMessage` returns.
- Host dimensions describe a clipping/render target, not a scaling request.
  Asset-native geometry must be supplied when recreating the host framebuffer.
- Supporting a 16-bit final display does not imply that the minigame renders
  16-bit pixels. The original architecture renders indexed 8-bit pixels first
  and performs palette-based conversion in the host graphics engine.
- Palette ownership is per loaded BMP: the bitmap object references a palette
  object at `+0x48`, and that palette object references its 256 RGBQUAD entries
  at `+0x10`. Palette frequency is not a reliable way to identify the display
  palette when backgrounds and sprites use different source palettes.
- `XTETDLL.DLL` packages game data in a named `XTETSFS` `RT_RCDATA` resource.
  Resource mounting and asset parsing are internal to the DLL; the standalone
  host does not need external copies of the background or sprite files.

## 2026-08-14 — Evidence standard

- When both sides of a proprietary interface and their resources are available,
  compatibility values must be recovered from those artifacts or measured at
  runtime. Plausible platform defaults and heuristic selection are diagnostic
  tools only, not valid reconstructed behavior.

## 2026-08-14 — Historical documentation

- A legacy filename extension is not evidence of a binary document format:
  `CommandsManual.DOC` is UTF-8 plain text. Detect the actual format before using
  document converters or OCR.
- Preserve discrepancies in historical technical documentation as explicit notes
  until code or runtime evidence resolves them; do not silently normalize syntax
  from a prose description.

## 2026-08-14 — CDF archive tooling

- The existing CDF extractor is self-contained apart from its historical
  Boost.Filesystem use; C++17 `std::filesystem` is a direct replacement for the
  path traversal and directory creation operations it uses.
- The `CDF97a` on-disk format uses packed structures: a 19-byte header and
  44-byte index entries. Compile-time size checks protect this layout across
  compilers.
- CDF block payloads begin with a 16-bit encoding type. Type `0` stores raw data
  and type `8` is decoded by the extractor's bundled inflate implementation.

## 2026-08-14 — Script-owned minigame environment

- `/GAME:<dll>:<object>::<field>` uses the suffix as a result destination, not
  as an initialization argument. GAG revisits the GAME command after module
  termination and assigns the synchronously captured result payload to that
  script state field.
- A minigame's palette context can be owned by the surrounding script plan rather
  than the DLL. Resolve conditional sublocations through to the active PRIMARY
  image at the moment `/GAME` executes; an earlier plan's primary is insufficient.
- `NOPAL` bypasses source-to-destination pixel-index remapping, but it does not
  prevent a PRIMARY image from installing its palette. These are independent
  behaviors in GAG's image activation path.
- Runtime verification confirms that the initial XTET launch palette is the
  palette embedded in `VE-GBNEW.BMP`, selected by the `NoSP` conditional PRIMARY
  image immediately before the GAME command.
- GAG image flag `NOPAL` is bit `0x04000000`. In the 8-bit blitter it bypasses
  source-to-destination palette-index remapping; therefore an image's embedded
  BMP palette is not necessarily the palette that should display its pixels.
- Game progression requires an XTET score greater than 65: the script stores the
  DLL result in `GAGBoy::Score`, adds `GAGBoy::Win` (`-65`), and tests whether the
  resulting field is positive.

## 2026-08-14 — Runtime sidecar resources

- When a required resource is defined as a DLL sibling, resolve the loaded
  module's actual path with its `HMODULE`; do not depend on the process working
  directory or assume where the DLL search path found it.
- Validate externally supplied bitmap structure and palette bounds before
  installing its color table. A filename match alone is not sufficient evidence
  that the required indexed resource was provided.

## 2026-08-14 — Legacy asset naming

- Filename extension is not a pixel-depth guarantee. In this data set,
  `XTET01.BMP` is an uncompressed 8-bit indexed image, and there is no
  `XTET01.256` companion on either disc; inspect the actual bitmap header and
  script reference before selecting a mode-specific asset.

## 2026-08-14 — Minigame audio boundary

- The GAG minigame audio ABI separates format discovery from sample queueing:
  handle creation receives a 16-byte PCM format block, while later queue calls
  receive only a raw PCM pointer and byte count.
- Queued audio data is borrowed rather than copied. A minigame host must keep
  queued pointers valid only because the DLL's decoded wave objects outlive their
  sound handles; a general reimplementation must document or enforce equivalent
  ownership.
- The host audio queue is persistent. Replace, restart, pause, and reset are
  distinct operations, and looping can be expressed by enqueueing the same PCM
  descriptor many times rather than by a loop flag.
- GAG's waveOut backend uses a bounded double buffer. A compatible host must
  retain the DLL's long logical descriptor queue but feed it incrementally as
  the two device buffers complete; submitting hundreds of loop descriptors to
  the device at once does not reproduce the original architecture.
- GAG opens waveOut with `CALLBACK_FUNCTION`; that callback does no mixing. It
  forwards WOM_OPEN/WOM_CLOSE/WOM_DONE to a dedicated sound-thread window, and
  the window-message path schedules the next mix buffer. Keep device callbacks
  lightweight and perform queue advancement in a serialized message context.
- Audio start/stop flags in this ABI reset scheduling timestamps, not PCM queue
  nodes. Only queue-with-replace clears the persistent descriptor list and byte
  offsets; do not infer rewind semantics from a nonzero start/stop flag.
- XTET's loop-music call sequence identifies callback roles end-to-end: slot 4
  is invoked before queue construction and on pause entry, while slot 5 is
  invoked after initialization and on unpause. Runtime comparison with the
  original establishes these as STOP and START respectively. Validate callback
  ordering through caller behavior and the original runtime, not isolated
  function naming on only one side of the ABI.
- Runtime verification confirms that the correct XTET host ordering is slot 4 =
  STOP and slot 5 = START/RESUME; with initialization playback deferred and a
  bounded two-buffer stream, music, pause/resume, SFX, and game-over audio all
  reproduce correctly.

## 2026-08-14 — Development configuration

- This repository's routine development and runtime-testing configuration is
  Win32 Debug. Release builds are optional unless release-specific behavior is
  under investigation.
- The Git repository uses `main` as its initial branch.
- Cross-platform line endings are repository-controlled through
  `.gitattributes`: all detected text uses LF. This takes precedence over
  contributors' `core.autocrlf` settings.

## 2026-08-14 — C++ formatting

- Apply the repository-root `.clang-format` to C++ rather than inferring style
  from nearby legacy code. The project uses four spaces (never tabs), Allman
  braces, and no space before opening parentheses in calls, declarations, or
  control statements.
- CMake exposes redumper-style `format` (in-place) and `check-format` (dry-run
  with errors) targets. Source globs are limited to `src/` and `tools/` so an
  in-tree build cannot accidentally include generated compiler-identification
  sources.
- Windows SDK multimedia headers are order-dependent: `<windows.h>` must precede
  `<mmsystem.h>`. Encode this as a higher-priority clang-format include category
  so automatic sorting remains build-safe and `check-format` stays useful.

## 2026-08-14 — Synchronous legacy presentation and one-shot audio

- A legacy dirty-rectangle callback can be a synchronous presentation boundary,
  not a request for a later window repaint. When a DLL renders an entire animation
  inside one host callback stack, implementing that boundary with `InvalidateRect`
  alone coalesces all intermediate frames before `WM_PAINT` can run.
- Derive audio activation semantics from the original mixer as well as explicit
  start/stop calls. XTET one-shots are started by queueing their descriptor; only
  loop music uses the separate control callbacks. A replacement host must not gate
  every queued sample on an explicit Start operation.
- Runtime verification confirms that immediate dirty-rectangle presentation and
  default-active one-shot handles reproduce XTET's match animation and sound while
  preserving the explicitly stopped/started music loop.
