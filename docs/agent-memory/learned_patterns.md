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

## 2026-08-15 — C++ naming and casts

- For generated C++, use `snake_case` for local variables and free functions,
  camel case for structures/classes and methods, and C-style casts where a cast
  is needed. Preserve recovered ABI names and avoid unrelated naming-only churn.

## 2026-08-15 — Portable reconstruction boundary

- Preserve the original x86 `__fastcall` ABI and fixed structure offsets only
  in the x86 compatibility adapter. Other Windows architectures use their native
  ABI and pointer widths with a matching recompiled host; they are not binary
  compatible with the original 32-bit executable.
- Keep proprietary resource extraction separate from normal compilation. A
  deterministic prepared resource directory lets the source build re-embed user-
  supplied evidence without committing the original asset payload.

## 2026-08-15 — XTET SFS container

- XTET mounts one named `XTETSFS` `RT_RCDATA` resource. Its SFS header is 0x100
  bytes, uses signature `SFS\0` and version 200, and validates an additive byte
  checksum after clearing the stored checksum field.
- The header points to a directory of 32-byte entries and an allocation map with
  one 32-bit entry per 32 KiB of logical archive space. Validate all derived
  counts, offsets, and spans before interpreting either region.
- SFS path lookup uppercases the relative path, computes a pair of table-driven
  32-bit hashes, and binary-searches the sorted directory. Both lookup tables are
  linear under XOR and can be represented exactly by eight basis values each.
- The allocation map has one terminal offset beyond the logical block count.
  Stored blocks are either 32 KiB raw data or shorter compressed records: marker
  1 selects the 4 KiB-window LZSS decoder, while the other marker routes the
  following type-0/type-8 stream through raw-copy or deflate decoding.
- SFS directory file positions are virtual offsets, not offsets into the resource.
  Reads map every 32 KiB virtual block through adjacent allocation-map offsets,
  decode that block, then apply the file's intra-block offset and logical length.

## 2026-08-15 — XTET object scripts

- XTET's serialized object declarations are text assets inside SFS. They use
  brace-delimited registered class names, semicolon line comments, optional DOS
  control-Z terminators, and recursive `INCLUDE` records.
- Resolve includes as part of declaration loading and reject active-stack cycles.
  Validate every declared binary asset through the archive before advancing DLL
  initialization, while keeping legacy in-memory class layouts unreconstructed
  until their consumers require and confirm them.

## 2026-08-15 — XTET binary assets

- Every bitmap referenced by the recovered XTET scripts is an uncompressed
  8-bit indexed BMP. Decode stored DWORD-aligned rows into a top-down linear
  framebuffer and retain the per-file 256-entry BGRA palette separately.
- XTET wave objects retain a standard 16-byte PCM format descriptor followed by
  borrowed/owned sample bytes from the RIFF `data` chunk. Preserve this descriptor
  layout independently of Windows header packing so it remains portable.
- An SFS type-8 stream that produces a complete 32 KiB logical block is usable
  even when the generic legacy inflater reports failure while trying to consume
  a terminator beyond its bounded output. Accept this only for exactly full
  blocks; never accept partial output after an inflater error.

## 2026-08-15 — XTET scene declarations

- An `INCLUDE` used as a serialized `item` contributes its included script's
  single top-level object at that exact child index. Preserve this hierarchy;
  flattening included children would change the runtime sprite indices.
- `EMPTY` declarations are reserved child slots, not whitespace or placeholders
  that can be discarded. Preserve them explicitly because later gameplay code
  addresses sprite children by stable numeric index.
- Names following a serialized object's closing brace are ObjLink bindings.
  Bindings are one-to-many (`loop`, `act`, and `pal_none` repeat), so expose
  lookup as a collection rather than a unique-name map.
- Keep absent serialized fields distinct from explicit zero/false fields until
  constructor defaults are recovered. Optional declaration values prevent a
  plausible default from silently becoming claimed original behavior.

## 2026-08-15 — XTET initial rendering

- Serialized `SHOW` and `VIEW` are independent sprite properties. Constructor
  defaults are zero, so initial visibility follows explicit `SHOW` values and
  must not be inferred from `VIEW`.
- Keep opaque and transparent indexed rendering as separate recovered paths.
  A confirmed clipped row-copy blitter is sufficient for explicitly opaque
  initial nodes, but it is not evidence for color-key or transparency behavior.
- Compose initial output by traversing the recovered scene hierarchy and its
  object-local positions. This preserves nested offsets and visibility instead
  of hard-coding one final framebuffer while runtime objects remain incomplete.

## 2026-08-15 — XTET RLI animations

- RLI header word +6 is a maximum frame index, not a frame count. Preserve and
  decode `max_index + 1` records; adjacent offsets delimit non-final frames and
  declared file size closes the final record.
- Resident and streamed resource modes can share a portable parsed model, but
  ownership must follow the recovered call mode. XTET's four startup RLI files
  are explicitly loaded resident and retained in full.
- Keep unresolved record fields as raw bytes while exposing only confirmed
  fields such as the monotonic data offset. This permits strict bounds checking
  without prematurely assigning animation semantics.

## 2026-08-15 — XTET RLI frame payloads

- Validate every adjacent RLI offset, then use declared file size as the final
  frame's payload boundary. Do not infer a terminal-only record merely because
  the record array has one more element than the header word.
- XTET RLI compressed frames reuse the standard BMP RLE8 packet language and
  bottom-up scan direction. A portable decoder can normalize output top-down as
  long as inclusive frame bounds and bottom-up packet movement are preserved.
- RLI flag bits independently select raw pixels, RLE8 pixels, and a palette
  prefix. Parse these capabilities separately even when the current resources
  happen to use only RLE8, so confirmed format behavior is not narrowed to the
  observed asset subset.

## 2026-08-15 — Indexed sprite coverage

- Color-key transparency and RLE sparse coverage are not interchangeable.
  An RLE encoded run of zero explicitly writes zero into the decoded bitmap;
  only RLE control gaps preserve that bitmap's existing pixels. If the decoded
  bitmap is then owned by a transparent TSprBmp, its zero pixels preserve the
  final scene during the separate sprite-composition stage.
- Normalize sparse decoded frames with separate pixel and coverage arrays. This
  retains the original distinction while allowing safe clipped compositing into
  architecture-neutral framebuffers.
- Mirroring changes source traversal, not destination advancement. Clip in
  destination coordinates, then map each retained destination coordinate back
  through the horizontal and vertical source transforms.

## 2026-08-15 — Dirty bounds must cover restored sprite artwork

- Reconstructing the correct pixels in a shared framebuffer does not display
  them until the host receives matching dirty callbacks. Board occupancy cells
  are insufficient when transparent sprite artwork extends outside its logical
  shape; publish the complete old sprite bounds after removal recomposition.
- Do not use callback argument identity when a mutation API deliberately passes
  an old-state value copy. Resolve the registered object through stable identity
  or its still-committed previous geometry before excluding it from a scene
  recomposition.
- A failed movement that transitions into matching may intentionally retain
  hypothetical gameplay coordinates without having presented the sprite there.
  At the effect boundary, restore both the last committed display bounds and the
  hypothetical bounds; current logical coordinates alone do not describe the
  pixels presently visible on screen.
- If an effect transition reconstructs all temporary layers from a background
  snapshot, dirtying only the newly revealed participant can expose stale pixels
  from the previous plan. Publish the union of every canvas whose prior pixels
  were replaced by that reconstruction.
- An opaque background/level-face change must be followed by recomposition of
  persistent dynamic sprites before its dirty rectangle is presented. Keeping
  gameplay objects in state is insufficient when the framebuffer and visible
  window are updated independently.
- When a synchronous host callback copies framebuffer pixels directly to the
  window, publishing removal and insertion as separate frames creates visible
  motion flicker. Retain both logical callbacks internally, but batch the union
  of their dirty bounds after the completed frame has been recomposed.

## 2026-08-15 — Nested scene-update scopes

- Treat begin/end scene-update functions as an observable batching contract,
  not merely locking. XTET resets accumulated dirty bounds on outermost begin,
  permits multiple sprite mutations to expand them, and flushes one union only
  when the nesting depth returns to zero.
- A multi-layer animation reconstructed directly into the host framebuffer must
  not expose intermediate layer creation merely because each internal mutation
  has a dirty callback. Accumulate those callbacks and publish only the fully
  composed state at the recovered end-update boundary.
- Audit batching globally by enumerating callers of begin/end update, the root
  dirty propagator, and all sprite visibility/position/mirroring mutations.
  Looking only at the animation function misses an outer caller scope that may
  also include removal of the preceding ordinary sprites.
- Preserve separate update scopes when the original deliberately exposes two
  states. XTET's completed-board-band effect atomically flushes a highlight and
  then atomically flushes removal; merging them would erase an intended visual
  transition just as splitting one scope introduces flicker.

## 2026-08-15 — Scene-collection mouse hit testing

- Validate hit-test node shape against the extracted scene rather than assuming
  it mirrors another collection. XTET controls are direct TSprBmp children of
  the control collection, not wrapper nodes containing another sprite.
- Traverse scene collections from last child to first so visual topmost order
  also determines input ownership. For transparent indexed sprites, rectangle
  containment is only the first gate; palette index zero must reject the hit.

## 2026-08-15 — Portable runtime pointer tables

- When an original x86 subsystem allocates several fixed-count arrays of object
  pointers, preserve the table and slot cardinalities but use native pointer
  widths in the portable gameplay model. Only ABI-visible layouts remain fixed
  to 32-bit pointers.
- Multi-table allocation should publish state only after every table succeeds.
  The original can expose partial allocations before failure; the reconstructed
  model can improve internal failure atomicity when that state is not observable
  across the compatibility boundary.

## 2026-08-15 — Figurine orientation geometry

- Occupancy templates may preserve multiple nonzero cell values while treating
  all nonzero values identically for bounds. Do not reduce the stored template
  to booleans when later gameplay may distinguish those values.
- Derive rotation-center corrections from occupied bounds after applying the
  exact discrete 5x5 transform. Computing offsets from nominal grid centers
  loses the asymmetry corrections used when a piece changes orientation.
- Mirrored orientations form their own four-state cycle. Keep their transforms
  and transition offsets separate from ordinary rotations even when several
  symmetric shapes produce identical results.

## 2026-08-15 — Legacy worker lifecycle

- A worker thread may be created before resource initialization yet remain
  behaviorally inactive behind a separate run flag. Recover creation and
  activation as distinct lifecycle stages before deciding when a reconstructed
  DLL can safely report successful initialization.
- Do not reproduce `TerminateThread` merely for machine-level fidelity. When no
  observable ABI depends on forced termination, use cooperative cancellation
  and joining to preserve cleanup ordering without corrupting locks or C++ state.
- Level timing can be a discrete lookup rather than a formula. Preserve the
  recovered table exactly, including its fallback value, instead of fitting an
  approximate progression curve.

## 2026-08-15 — Falling-piece board entry

- A centered occupancy template can legitimately extend above the board during
  spawn. Preserve the original row-table lower-bound test: ignore occupied cells
  at negative rows, while still rejecting the bottom and horizontal bounds once
  the cell reaches a real row.
- When collision tables retain pointers to a currently moving object, placement
  validation must explicitly ignore that object's pointer and any confirmed
  paired runtime object. Treating every non-null slot as a collision breaks
  movement checks performed before old slots are cleared.
- Apply a constrained random-family choice before updating its running balance.
  At a limit, switching the choice and then applying the selected family's delta
  produces the original movement back toward the permitted range.

## 2026-08-15 — Figurine sprite variants

- A legacy piece renderer can encode rotation as a combination of a base bitmap
  variant selected by orientation parity and independent horizontal/vertical
  mirror flags. Preserve this compact selection model instead of generating new
  rotated pixels when the serialized asset collections already contain the
  parity variants.
- Keep logical sprite selection separate from legacy clone/ownership mechanics.
  A portable descriptor containing the asset family, frame index, mirror flags,
  and position captures confirmed observable behavior while avoiding premature
  reconstruction of intrusive reference-counted scene objects.

## 2026-08-15 — Serialized sprite-map resolution

- Resolve gameplay frames through the recovered serialized hierarchy rather
  than duplicating its frame-to-filename table in code. Validate the expected
  transparent sprite wrapper and its single bitmap map so malformed or changed
  asset input fails at a clear boundary.
- Dirty callbacks should describe the clipped destination rectangle, including
  negative-position clipping during board entry. Preserve the original callback
  register/stack argument order separately from the portable rectangle model.

## 2026-08-15 — Transactional board movement

- Validate a tentative move while old self-pointers remain in the occupancy
  table, explicitly ignoring those pointers. Only after success should the old
  slots be cleared and the new oriented footprint inserted; this prevents a
  rejected move from temporarily corrupting board state.
- Rollback fields may refer to the last rendered/committed state rather than the
  values immediately before the current call. Preserve explicit previous fields
  when the original uses them, especially when rendering and board mutation are
  coordinated across helper calls.
- Mirrored and unmirrored orientation cycles can advance in opposite numeric
  directions. Recover the transition table explicitly; arithmetic increment
  across signed codes can accidentally create the invalid orientation zero.

## 2026-08-15 — Normalized pair-action lookup

- A pair compatibility table can store only one family's canonical frame. To
  query it, consistently order the two families, transform relative position
  and the second object's orientation into the first object's local rotation,
  and apply its signed mirror last.
- Preserve nonzero occupancy values when collision initially treats them alike.
  Match-path validation may give a particular value separate semantics, such as
  rejecting only value-2/value-2 crossings while allowing other contacts.
- Directional match searches can intentionally leave the moving object at a
  hypothetical destination on success but restore committed coordinates on
  failure. Make this state transition explicit in the result contract rather
  than presenting the search as a const lookup.

## 2026-08-15 — Matched animation plans

- Separate action interpretation from timed rendering. A portable plan can
  resolve resource, frame range, mirror flags, transformed anchors, and temporary
  layer order without inheriting the legacy scene-clone ownership model.
- Validate recovered animation tables against every real action record and the
  decoded resource frame counts. This cross-check can expose format mistakes
  that ordinary sequential decoding misses, especially whether a final metadata
  record is renderable.

## 2026-08-15 — Fixed-slot effect composition

- When an effect reveals temporary scene children in one order but the scene
  renderer draws by child index, direct chronological blits produce the wrong
  overlaps. Retain active logical layers and recompose them in fixed slot order
  for every reveal.
- Mirror sparse frame patches around their declared full animation canvas. A
  patch-local flip moves pixels to the wrong side whenever frame bounds do not
  cover the entire canvas.
- Snapshot the background after ordinary sprites are hidden. Restoring that
  snapshot before recomposition reproduces retained sparse layers without
  requiring the legacy reference-counted temporary sprite objects.

## 2026-08-15 — Non-blocking legacy cadence

- Represent recovered busy-wait timing as explicit delay requests in the
  portable core. This preserves exact cadence while allowing a Windows adapter
  or test harness to choose cooperative waiting, scheduling, or deterministic
  immediate execution.
- Keep score progression separate from visual destruction. When the original
  updates once per freed object, pair removal must invoke the transition twice
  rather than replacing it with a guessed pair-level bonus.

## 2026-08-15 — Active versus board ownership

- Treat the active falling pointer as a control designation, not exclusive
  ownership of the object. Clearing it after downward rejection can leave the
  same object alive and owned through board slots; clearing it after a match
  accompanies removal of both matched objects.
- Movement result interpretation depends on the caller. A rejected automatic
  down step means settled, while a rejected rotate/arrow key keeps control of the
  active piece. Do not collapse this into a single generic rejection side effect.
- Capture registry identities before erasing matched entries. Candidate pointers
  into a vector or table become invalid during removal even though the underlying
  gameplay objects may remain caller-owned until cleanup completes.

## 2026-08-15 — Mutable cascade scans

- When gravity scans an occupancy grid that it mutates, preserve the original
  traversal direction. A bottom-to-top scan lets a moved object fall into rows
  already visited instead of processing it repeatedly as a new outer-loop item.
- A match that removes multiple objects invalidates both occupancy and registry
  iteration state. Complete the synchronous match effect and pair removal, then
  restart the scan from its original boundary rather than retaining pointers or
  indices into the mutated collections.

## 2026-08-15 — Automatic tick ownership transitions

- Keep a worker tick's active pointer distinct from board ownership. A rejected
  automatic downward move clears the active designation but deliberately leaves
  the same object in occupancy and registry storage as a settled obstacle.
- Inject object creation at the portable controller boundary, then validate that
  the returned identity was registered before publishing it as active. This
  preserves recovered spawn success/failure behavior without coupling the core
  to allocation, rendering, or legacy clone ownership.

## 2026-08-15 — Input-specific rejection semantics

- The same movement rejection can have different ownership effects at different
  callers. A keyboard rejection, including the terminal rejection of hard drop,
  retains active control, while an automatic-tick rejection settles the piece by
  clearing only the active designation.
- Drain queued input only after a synchronous match effect, pair removal, and all
  cascade settling have completed. Keep the platform message-range operation in
  the adapter while representing its exact ordering as a portable callback.

## 2026-08-15 — Board-change presentation phases

- Expose presentation hooks at the successful mutation boundaries: old identity
  and geometry before occupancy removal, then new geometry after insertion.
  Rejected transactional moves should emit neither phase because the original
  does not invalidate its board cells on that path.
- Keep board-cell invalidation distinct from sprite rendering. The original first
  invalidates old/new 17-by-17 occupancy cells and only afterward updates the
  sprite object, so combining both into an assumed single dirty rectangle loses
  observable ordering and overlap behavior.

## 2026-08-15 — Shared movement presentation hooks

- Propagate one movement-presentation contract through direct movement, worker
  ticks, keyboard repetition, and cascade gravity. Controller-specific hooks can
  otherwise omit intermediate hard-drop or cascade steps and produce framebuffer
  behavior that diverges despite correct final board state.
- Count presentation phases against successful movement results, not attempted
  commands. A repeated movement command emits two phases per successful step and
  none for the rejection that terminates the loop.

## 2026-08-15 — Occupancy-cell invalidation adapters

- Convert oriented occupancy into dirty regions in board row/column order when
  the original invalidation helper scans the runtime table rather than the sprite
  bounds. Preserve the grid cell size and clip only at the framebuffer boundary.
- Do not treat dirty-region generation as permission to clear framebuffer pixels.
  A transparent moving sprite may overlap other dynamic or static scene layers;
  restoration requires the recovered compositor order, not a guessed background
  color or a direct redraw of only the base bitmap.

## 2026-08-15 — Runtime-expanded scene slots

- A serialized scene description may not expose the final runtime child array:
  constructors or wrapper objects can add reserved null slots and bookkeeping
  children. Do not derive dynamic layer indices from a plausible serialized node
  unless its identity is confirmed end-to-end.
- Model confirmed first-free slot allocation with an explicit recovered upper
  bound when construction of that bound is still unresolved. Preserve a live
  object's slot across sprite replacement so frame/orientation changes do not
  silently change overlap order.

## 2026-08-15 — Board masks versus scene restoration

- Trace a dirty-cell helper through its final bitmap primitive before classifying
  it as invalidation-only or scene recomposition. A collision/board layer may be
  explicitly filled with palette indices while sprite rendering happens in a
  separate later call.
- Preserve phase-specific fill values. XTET writes index `0x13` over an object's
  old occupied cells and index 0 over its new cells before updating the sprite;
  replacing this with a generic background restore loses the recovered indexed
  board-mask semantics.

## 2026-08-15 — Composed movement presentation

- Keep the board-mask update and sprite draw as ordered substeps of one movement
  presentation callback. The add phase must zero the new occupied cells before
  drawing the positioned transparent sprite; drawing first would erase covered
  pixels during the mask update.
- Emit occupied-cell regions and sprite bounds separately when the original has
  distinct invalidation paths. A single union rectangle can reproduce final
  pixels but loses callback ordering and intermediate presentation evidence.

## 2026-08-15 — Spawn search and partial side effects

- A blocked spawn can search upward from its nominal row until geometry no longer
  intersects the board, then still reject the object because its center crossed
  a separate minimum-row threshold. Preserve both the placement search and the
  post-search threshold rather than treating any geometrically placeable result
  as success.
- Random family-balance state can change before later allocation or placement
  failure. Do not roll back confirmed random-selection side effects merely to
  make a portable spawn transaction internally atomic.
- Keep drawable allocation failure distinct from gameplay-object failure when
  the original returns the object even if its cloned sprite is null. An invalid
  presentation slot can be represented explicitly without discarding board state.

## 2026-08-15 — Stable portable runtime identities

- Use stable heap object addresses as portable identities when original board
  tables and active state both store object pointers. A vector of owning smart
  pointers preserves pointee addresses while allowing the owner collection to
  grow and compact.
- Remove ownership only after controller callbacks and registry mutation finish.
  Match and cascade code may retain candidate pointers during synchronous effects;
  pruning objects earlier would recreate the dangling-pointer hazard avoided at
  the registry boundary.
- Keep random values as explicit tick inputs until the platform adapter is wired.
  This makes spawn behavior deterministic in tests without changing the original
  number or ordering of eventual random-source calls.

## 2026-08-15 — One owner for tick and input paths

- Route automatic ticks and keyboard commands through the same owned active
  identity and registry. Separate controller wrappers can accidentally disagree
  about whether terminal hard-drop rejection has settled the object.
- Perform owned-object pruning after either controller returns, never inside the
  effect callback. Cascades can remove additional objects beyond the initial
  active pair, so cleanup must compare the complete final registry.

## 2026-08-15 — Exported message acceptance contracts

- Separate an exported window-procedure's state gate and return-value contract
  from the inner handlers. A DLL can return “accepted” for messages it ignores as
  long as the gameplay state is active, so deriving return values only from
  whether a callback ran is incorrect.
- Preserve mouse transition polarity at the outer boundary. XTET converts
  `WM_LBUTTONDOWN`/`WM_LBUTTONUP` into a boolean pressed/released argument before
  performing scene hit-testing in the inner handler.

## 2026-08-15 — Result-state input deadlines

- End-state key handling can be time-gated independently of key identity. Before
  XTET's result deadline, all state-2/3 keys—including Escape—are drained; at the
  deadline, any key acknowledges the result and triggers stop/result/termination.
- Preserve unsigned deadline comparison and equality behavior. The original uses
  `deadline <= timeGetTime()`, so equality belongs to the completion path.
- Reset gameplay state before invoking stop cleanup when the original stop helper
  begins by clearing that state. Callbacks that inspect state can otherwise see a
  transient ordering difference even if final state is correct.

## 2026-08-15 — Shared end-state deadlines

- Trace every write to a deadline global before assigning separate semantics to
  failure and success screens. XTET uses the same `timeGetTime()+2000` input gate
  for both spawn-failure state 2 and completion state 3.
- Preserve unsigned timer arithmetic at the storage boundary. Adding a delay in
  a wider signed type and narrowing later changes wraparound behavior near the
  32-bit millisecond rollover.
- Split portable gameplay cleanup from platform resource teardown. The core can
  release active/board/owned state deterministically, while the Windows adapter
  owns thread joining, GDI deletion, scene references, and audio shutdown.

## 2026-08-15 — Gated DLL-boundary integration

- It is safe to wire recovered outer ABI/message behavior before activating a
  subsystem, provided incomplete inner callbacks remain explicitly unbound and
  initialization cannot publish an active state. Do not substitute a diagnostic
  success callback for a required match effect.
- Preserve the original clock API at the Windows boundary when deadline behavior
  is observable. XTET uses `timeGetTime`; a broadly similar tick source is not
  evidence-equivalent merely because it also returns milliseconds.

## 2026-08-15 — Synchronous effect adapters

- Keep recovered blocking effect cadence at the platform boundary: the portable
  renderer emits delay requests, while the DLL adapter selects the Windows wait
  primitive and preserves synchronous callback ordering.
- Hide ordinary matched sprites through the same recovered board-mask removal
  phase used by movement before taking the animation background snapshot. If the
  snapshot is taken first, blink restoration resurrects the sprites after the
  effect completes.

## 2026-08-15 — Per-object progress presentation

- When pair destruction calls a score transition once per object, surface the
  presentation callback inside that destruction primitive. Redrawing only after
  the pair controller returns skips an observable intermediate score and also
  misses identical updates produced by cascade paths.
- Recover dirty granularity from the drawing function's final invalidation call.
  A four-digit renderer may perform four blits but publish one combined region;
  framebuffer equivalence alone does not establish callback equivalence.

## 2026-08-15 — Borrowed PCM queue reconstruction

- Model legacy streaming callbacks as descriptor queues over stable decoded PCM
  storage, not whole-file playback calls. The original host may retain sample
  pointers long after the queue callback returns, so asset ownership must outlive
  every sound handle.
- Preserve apparently redundant queue operations when cardinality is observable.
  XTET queues the first loop sample twice before 300 full eight-sample passes,
  yielding 2,402 descriptors rather than simplifying the sequence to 2,400.

## 2026-08-15 — Audio event placement

- Place one-shot queue operations at the recovered event boundary, not after a
  composite controller returns. Match audio precedes synchronous animation, and
  level/result sounds precede the score redraw even though all operations occur
  within one input dispatch.
- Keep each host callback's register ABI explicit at the DLL boundary. Wrapping
  typed callback pointers in portable `std::function` adapters is safe only after
  the x86 `__fastcall` signature has been preserved by the cast target.

## 2026-08-15 — Pause state and audio control

- Separate pause state transitions from UI hit testing. The recovered state
  machine can be tested as state/polarity plus loop-control callbacks even while
  scene-child hit indices and pressed artwork remain unresolved.
- Commit the new gameplay state only after the corresponding host audio control
  succeeds. This prevents a portable adapter from publishing paused/unpaused
  state when the required observable loop transition failed.

## 2026-08-15 — Cooperative reconstruction of a gated worker

- Preserve worker creation and activation as separate operations when the
  original thread starts before resources but remains behind a run flag. This
  permits equivalent lifecycle ordering without allowing ticks to observe
  partially initialized state.
- Replace forced thread termination with a stop signal and join, while retaining
  the original injected wait interval, exception boundary, and gameplay-lock
  placement. Machine-level teardown mechanics are not behavioral compatibility.
- Account for random calls at their original layers. A worker may discard one
  random result before a spawned object's constructor consumes its own sequence;
  collapsing these calls changes every later deterministic selection.

## 2026-08-15 — Collision masks are not necessarily presentation surfaces

- A recovered indexed fill can target an internal collision/occupancy layer even
  when it shares dimensions and coordinates with visible gameplay. Confirm the
  destination object before copying palette indices into the host framebuffer.
- When the original scene compositor is ownership-heavy, a portable adapter can
  restore movement regions by deterministically recomposing static scene layers
  plus live registry objects in slot order, while retaining the recovered dirty
  callback granularity.

## 2026-08-15 — WinMM teardown callback reentrancy

- `waveOutReset` can synchronously deliver completion callbacks while teardown
  still owns its sound-state lock. Use a reentrant lock or release the lock
  around reset, and publish the stopped state before invoking the API.
- A loader that shows a modal result window after closing gameplay has not hung.
  Runtime smoke automation must close both top-level windows before evaluating
  process termination.

## 2026-08-15 — Restart composition must use runtime selection

- Do not reuse a serialized initial-scene renderer for a restart when selectable
  scene children have mutable runtime visibility. Recompose from the newly reset
  runtime state and publish the selected child in the same atomic replacement
  phase that removes prior dynamic objects.
- Preserve separately invalidated UI updates after the scene replacement. XTET's
  `StartNewGame` commits the selected level layer first and calls `DrawScore`
  afterward, so one convenient full-frame refresh would lose observable callback
  ordering.

## 2026-08-15 — Button artwork as hidden overlays

- A serialized button sprite can be a normally hidden pressed-state overlay,
  with the unpressed artwork already baked into the static background. Confirm
  visibility mutations before assuming the sprite itself is the normal button.
- Persistent controls such as pause keep the overlay shown across the state,
  whereas ordinary actions show it only around synchronous handling. Removing
  a transparent overlay requires restoring the composed scene beneath it before
  invalidating the overlay's bounds.

## 2026-08-15 — Preserve parent-surface clipping

- Child coordinates can intentionally place sprite artwork outside a parent
  scene surface. Rendering those children directly into the root framebuffer
  loses the parent's clipping contract and can overwrite surrounding frame
  artwork even when the recovered coordinates are exact.
- Clip both pixel writes and published dirty bounds to the owning surface. A
  reset that invalidates only the parent rectangle cannot repair pixels an
  incorrectly unclipped child previously wrote outside it.
