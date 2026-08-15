# Active context

Last updated: 2026-08-15

## Build preference

- The user's active development configuration is Win32 Debug. Use Debug for
  routine builds and verification; do not build Release unless explicitly
  requested or investigating release-specific behavior.
- Do not run x64 builds during the current XTET reverse-engineering phase. Keep
  new source architecture-neutral, but verify only Win32 Debug unless the user
  explicitly requests another architecture or a later portability milestone.
- The compatibility loader window uses a fixed-size overlapped style with a
  caption, system menu, and minimize button, but no sizing frame or maximize
  button. Its 640-by-480 client area is not user-resizable.
- The tracked XTET compatibility loader source lives at
  `xtet/loader/main.cpp`; root `src/` is reserved for the future main game.
- The loader presents a completed minigame result as decimal `Score: N`; the
  result dialog and diagnostic trace no longer include hexadecimal output.

## Version control

- The repository was renamed from `gagboy` to `freegag`. Use `freegag` for the
  repository/project identity; preserve `GAGBoy` where it is an original script
  object or other recovered game identifier.
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
- For newly generated C++, the user prefers `snake_case` local variables and
  free functions, camel case structures/classes and methods, and concise C-style
  casts. Apply these preferences to new or naturally touched code; do not rename
  recovered ABI identifiers or unrelated existing code solely for consistency.
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

Reconstruct `XTETDLL.DLL` as source under `xtet/`, preserving exact original
Win32 x86 compatibility while allowing matching recompiled Windows hosts and
DLLs to build for other processor architectures.

## XTET reconstructed DLL activation and verification

- The reconstructed DLL is active and is copied beside `xtet_loader.exe`; the
  loader no longer uses the original DLL. Ordinals 1--3 retain their recovered
  x86 `__fastcall` ABI and decorated exports.
- Initialization now loads the embedded XTETSFS archive, decodes the scene and
  assets, creates the six recovered audio groups, starts the 2,402-entry loop
  queue, initializes the portable gameplay runtime, renders the initial frame,
  and enables the cooperative worker.
- The live adapter implements keyboard control, scene-derived mouse controls,
  pause/resume, restart, level selection, exit, result acknowledgement, private
  host messages, audio commands, level effects, win/over presentation, and
  cooperative cleanup.
- `InitializeGameAssets` confirms the gameplay layer has 117 children, dynamic
  figurine slots are 3--103, effect placeholders are 104--110, `over.bmp` is
  child 111, and `win.bmp` is child 112. The gameplay layer's screen origin is
  `(243,77)`.
- The internal board layer's palette-index `0x13`/zero writes are collision-mask
  state, not host-visible framebuffer pixels. Live presentation therefore
  recomposes the static scene and all registered figurines for movement/removal,
  then emits the recovered dirty-cell and sprite callbacks. This removed the
  offset sprites, trails, and brown mask stripe observed in early live captures.
- The loader sound lock is recursive and marks buffers stopped before reset so
  synchronous WinMM callbacks cannot deadlock teardown.
- Win32 Debug builds cleanly; CTest and `check-format` pass. A live loader smoke
  test remained responsive during worker activity and injected input, then
  closed the game and result dialog with process exit code 0. `dumpbin` confirms
  the three expected fastcall exports. No x64 build was run.
- Remaining validation is comparative parity against the original through
  longer user-driven sessions (visual timing, exact mouse pressed feedback, and
  audible output). No known implementation gate remains.

## Live compositor corrections

- User testing exposed falling-sprite remnants. Full-scene recomposition was
  already restoring the framebuffer memory, but the removal phase published
  only occupied 17-by-17 board cells. It now also publishes the complete old
  sprite rectangle, including transparent artwork outside occupied cells.
- User testing also exposed black rectangles behind matched-pair animations.
  Ghidra confirms `RenderRliFrame` decodes into temporary TSprBmp maps. Encoded
  zeroes overwrite those temporary maps, but TSprBmp composition subsequently
  treats index zero as transparent. Direct RLI-to-framebuffer composition now
  reproduces that second color-key stage.
- Match setup publishes both removed sprites' complete bounds after scene
  recomposition before revealing temporary animation layers. Win32 Debug build,
  CTest, `format`, and `check-format` pass after these corrections.
- A second live test still showed remnants near the top during downward motion.
  The removal callback receives a value-copy snapshot of the old geometry after
  the registered figurine has already been mutated. Pointer-based exclusion
  therefore failed and recomposed the registered object during the removal
  phase. The adapter now resolves that registry entry from its committed
  previous geometry and excludes the actual live object. Win32 Debug build and
  all automated checks pass; this correction awaits user runtime confirmation.
- A subsequent live test isolated a remnant at match-animation startup. A
  blocked directional match leaves the source figurine at its hypothetical
  match coordinates while the last presented sprite remains at its committed
  `previous_*` coordinates. Match setup now publishes complete sprite bounds at
  both positions for both participants after background recomposition and before
  revealing RLI layers. Win32 Debug build and all automated checks pass.
- Live testing then exposed right-side flicker during the blink and temporary
  disappearance of settled figurines during a level change. Each RLI reveal now
  publishes the union of both participants' 108-by-108 temporary canvases, so a
  plan transition cannot leave pixels from the preceding plan visible on the
  other participant's side. Level-face changes now update effect state, fully
  recompose the gameplay scene, score, and registry figurines, then publish the
  face region; they no longer place an opaque face over already drawn pieces.
  Win32 Debug build, CTest, `format`, and `check-format` pass.
- A final minimal falling-tick flicker came from presenting the removal and add
  phases as two synchronous host copies. The adapter now retains old board and
  sprite bounds, recomposes the completed add phase, and publishes one union
  rectangle covering old and new artwork. Portable movement still emits its two
  logical mutation phases; only the DLL presentation boundary batches them.
  Win32 Debug build and all automated checks pass.
- Investigation of rare match-animation flicker recovered the original atomic
  scene-update mechanism. `BeginSceneUpdate` clears accumulated dirty bounds on
  the outermost entry; `EndSceneUpdate` flushes their union only when the nested
  update depth returns to zero. `MoveFallingFigurine` brackets the initial
  seven-layer `AnimateMatchedPair`, and `BlinkMatchedPair` separately brackets
  each expanded/normal rebuild plus the final seven-layer hide. The portable
  blink renderer now accumulates its internal seven reveals and publishes one
  completed state per scope: initial, four timed transitions, and final hide.
  Ghidra names/comments are synchronized. Win32 Debug build, CTest, `format`,
  and `check-format` pass.
- A complete Ghidra presentation-batching audit enumerated every caller of
  `BeginSceneUpdate`, `EndSceneUpdate`, the root dirty propagation helper,
  sprite visibility, position, and mirroring mutations. Reachable categories:
  new-game replacement (one atomic scope, then score); ordinary spawn/movement/
  rotation sprite update (one atomic scope); matched-pair initial construction,
  each blink state, and final hide (one scope each); completed-board-band
  highlight and removal (two intentional scopes); teardown (no visible flush);
  and direct single-object score/result/pause/level/button changes.
- The audit found one remaining active match discrepancy: ordinary pair removal
  was flushed before the initial animation state. The original brackets board
  invalidation, both ordinary sprite hides, and all seven initial effect layers
  together. `present_match_effect` now accumulates its removal bounds into the
  first atomic animation flush, eliminating the blank pre-effect frame.
- `ClearCompletedBoardBands` at `0x10020500` is confirmed but not yet represented
  by the portable gameplay implementation. It deliberately publishes two atomic
  states: an index-`0xe5` completed-band highlight, then index-`0x13` removal and
  figurine destruction. This is a broader known gameplay fidelity gap, not an
  active reconstructed dirty/flicker path.
- Ghidra now names `BeginSceneUpdate`, `EndSceneUpdate`, `StartNewGame`,
  `SetBoardOverlayVisible`, `ClearCompletedBoardBands`,
  `FindCompletedBoardBand`, `IsBoardRowFull`, and
  `BoardRowsShareFigurine`, with synchronized comments.
- The source DLL target, tests, formatting, and whitespace checks pass. A live
  `xtet_loader.exe` process currently locks `build/Debug/XTETDLL.DLL`, so the
  newly built artifact is `build/xtet/Debug/XTETDLL.dll`; the loader-directory
  copy remains the previously loaded build until that process exits.

## Mouse-control hit testing

- Ghidra confirms `GetHostCursorPoint` uses `GetCursorPos` plus
  `ScreenToClient`; `HitTestSpriteCollection` scans children from last to first,
  converts the point through scene-local transforms, and invokes per-pixel map
  testing for flagged sprites.
- Extracted scene evidence identified the mouse failure: each of the eight
  controls is directly a `sprite_bitmap` with one bitmap child. The DLL hit test
  incorrectly required an extra nested sprite node, so it skipped every control.
- `hit_test_sprite_collection` now validates the actual serialized shape,
  traverses in reverse/topmost order, converts to sprite-local coordinates,
  bounds-checks the decoded bitmap, and rejects transparent index-zero pixels.
  Focused tests locate a nonzero mask pixel in every extracted control and
  verify that it resolves to indices 0--7.
- Ghidra names/comments now cover `GetHostCursorPoint`,
  `HitTestSpriteCollection`, `TransformPointToSpriteLocal`, and
  `HitTestSpriteMap`. Win32 Debug source DLL, CTest, formatting, and whitespace
  checks pass. After the prior loader process exited, `xtet_loader` rebuilt and
  copied the corrected DLL into `build/Debug` successfully.

## Selected-level restart presentation

- User testing found that restarting after selecting a higher level initially
  exposed the serialized level-one face; falling-piece recomposition then
  revealed patches of the selected face.
- Ghidra `StartNewGame` at `0x1001b7e0` confirms restart hides every level child
  and existing figurine inside one scene update, shows child
  `selected_level - 1`, ends that atomic update, and then calls `DrawScore`.
- `restart_game` now initializes progress with the selected level, recomposes and
  publishes that selected face through the normal gameplay compositor, then
  publishes the reset score separately. Win32 Debug build, CTest,
  `check-format`, and whitespace checks pass; runtime confirmation remains.

## Pause-button pressed presentation

- User testing confirmed mouse pause/resume changed gameplay state and audio but
  did not display the original pressed pause artwork.
- Ghidra `HandleGameMouseButton` and the recovered helper now named
  `SetGamePaused` at `0x10021670` confirm control child 3 is a normally hidden
  overlay: entering state 4 calls `SetSpriteShown(true, invalidate)`, while
  leaving state 4/5 calls `SetSpriteShown(false, invalidate)`. Ordinal-3 host
  commands 2 and 4 invoke the same helper.
- The DLL now blits and publishes control overlay 3 on pause, retains it while
  paused, and removes it by recomposing gameplay before publishing its exact
  bitmap bounds on resume. Mouse and host-command paths share this behavior.
  Ghidra names `SetGamePaused` and `SetSpriteShown` are synchronized. Win32
  Debug build, CTest, `check-format`, and whitespace checks pass; runtime
  confirmation remains.

## Spawn viewport clipping

- User testing found repeated Start resets could leave figurine pixels just
  above the gameplay rectangle, and newly spawned figurines could draw over the
  console's upper frame.
- Ghidra reconfirmed the complete placement chain: `CreateFallingFigurine`
  starts at centered column and row 2, searches upward only on collision,
  `CalculateFigurineScreenPosition` maps to `x = column * 17 - 45` and
  `y = row * 17 - 45`, and `CreateFigurineSprite` attaches the clone to a free
  dynamic child slot in the gameplay scene surface. The portable spawn position
  was already exact and was not changed.
- The discrepancy was at the adapter boundary: portable figurines were blitted
  into the entire 640-by-480 host framebuffer instead of being clipped by their
  255-by-340 gameplay parent at `(243,77)`. Figurine composition and reported
  sprite dirty bounds now intersect that gameplay viewport. A focused test
  verifies a row-2 spawn begins drawing at screen y=77, leaves the console pixels
  above untouched, and reports only the clipped region.
- Win32 Debug build, CTest, `check-format`, and whitespace checks pass; runtime
  confirmation of repeated Start and spawn appearance remains.

## XTET DLL synchronous match presentation

- The dormant DLL gameplay-key adapter now constructs the shared movement
  presentation callback over the host framebuffer and callback-table slot 0.
- A matched pair is synchronously hidden through the recovered board-mask
  removal phase, then rendered through the complete RLI blink sequence. Every
  board cell and animation frame is reported through the host dirty-region
  callback, and the four recovered 400 ms delays use `Sleep` at the Windows
  boundary rather than introducing timing into the portable renderer.
- Progress presentation is now propagated through direct matches, worker ticks,
  and cascade matches. It runs after each individual figurine-removal update, so
  a pair visibly advances through both intermediate scores instead of jumping by
  two after controller completion.
- Ghidra reconfirmed that `DrawScore` draws all four glyphs and then invalidates
  one combined rectangle at `(359,438)` with width `4 * (atlas_width / 4)` and
  height `atlas_height / 10`. The DLL adapter redraws and reports that clipped
  region for every score update.
- Initialization still deliberately reports failure, so these paths cannot yet
  become active in the loader. Recovered audio side effects are the next adapter
  required before activation.
- Win32 Debug build, CTest, `format`, and `check-format` pass. No x64 build was
  run.

## XTET portable audio coordination

- `AudioCoordinator` now resolves the six confirmed scene-link groups: eight
  `loop` waves, six randomized `act` waves, and the single `stop`, `level`,
  `over`, and `win` waves. Each group validates one shared PCM format and owns a
  host-created sound handle while retaining borrowed decoded sample storage.
- Loop construction reproduces `InitializeLoopingSoundQueue`: stop the handle,
  queue the first loop sample with replace, queue it once more without replace,
  then queue all eight samples for 300 passes, producing exactly 2,402 borrowed
  PCM descriptors.
- One-shot selection queues the selected group's sample with replacement.
  Loop start/stop and deterministic random-index injection are isolated behind
  host callbacks, and teardown destroys all six handles.
- DLL callback slots 1–5 now use explicit recovered `__fastcall` declarations:
  create, destroy, queue, stop, and start. Audio initialization creates the six
  group handles, stops the loop handle, builds all 2,402 descriptors, and starts
  it in the confirmed order. Failure and ordinal cleanup destroy the handles
  deterministically; the loader defers actual device submission until ordinal 1
  returns.
- Match handling queues a randomized `act` sample with replacement before hiding
  sprites or starting animation. A level transition queues `level`; terminal
  completion stops loop music and queues `win`, all before the score redraw as
  recovered from `UpdateScoreAfterFigurineRemoval`.
- `set_game_paused` captures the confirmed state/audio transition independently
  from unrecovered mouse hit testing: state 1 plus pause becomes state 4 after
  stopping the loop; state 4 or 5 plus unpause becomes state 1 after restarting
  it. Other state/polarity combinations are no-ops.
- Focused tests verify group discovery, handle counts, loop queue cardinality and
  replace flags, randomized action queuing, loop controls, teardown, and pause
  transitions. Spawn-failure `over` and the scene-specific pause-button hit test
  remain tied to the still-gated worker and mouse-controller activation.
- Win32 Debug build, CTest, `format`, and `check-format` pass. No x64 build was
  run.

## XTET cooperative worker scheduler

- Ghidra reconfirmed that `GameWorkerThread` waits using the current level
  interval even while disabled, checks the run flag after each wait, enters the
  gameplay lock, discards one `rand()` result, and then runs one tick. A spawn's
  constructor subsequently consumes three additional random values for family,
  shape, and orientation.
- `GameWorker` now provides the architecture-neutral lifecycle: creation starts
  a disabled thread, interval/tick/failure operations are injected, enabling is
  independent from creation, and cooperative stop joins the thread. This
  intentionally replaces the original `TerminateThread` cleanup.
- A focused test starts the disabled worker, enables it, observes a timed tick,
  stops it, and verifies it is no longer running.
- Original static data confirms the new-game selected/base/current level begins
  at 1. DLL attachment remains gated because the upper bound of the runtime
  dynamic scene-child array is not yet confirmed; the previously recovered
  `g_nLastFigurineSceneSlot` is an allocation cursor, not that upper bound.
- The worker tick's remaining DLL-side result operations are confirmed: ordinary
  settling queues `stop`; spawn failure stops loop music, queues `over`, and
  establishes the shared two-second result deadline.
- Win32 Debug build, CTest, `format`, and `check-format` pass. No x64 build was
  run.

## XTET falling-figurine state and placement

- `FallingFigurine` now models the confirmed portable portion of the original
  0x10-byte falling-object state: family, shape, signed orientation/position,
  and previous orientation/position. The original runtime sprite pointer remains
  outside this portable state until its construction is recovered.
- Spawn selection reproduces the two families (five and ten shapes), the
  family-balance limit, shape modulo, initial `-3`/`-1` orientation, centered
  column, and row 2. At either balance limit, a random choice that would cross
  the limit is switched to the opposite family before updating the balance.
- Oriented 5x5 templates are now exposed through a checked geometry API.
  `can_place_figurine` maps their center to board coordinates with a `-2`
  adjustment, rejects right/left/bottom bounds and foreign occupants, permits
  the figurine and paired object already in slots, and preserves the original
  behavior of ignoring occupied cells above row zero.
- `RuntimeTables::set` provides bounded mutation for later placement and focused
  tests. Tests cover normal/forced selection, collision, self-occupancy, and
  above-board entry.
- Ghidra now names and documents `CreateFallingFigurine`,
  `CanPlaceFigurine`, `PlaceFigurineOnBoard`, `MoveFallingFigurine`, and
  `UpdateFigurineSprite`.
- Win32 Debug build, CTest, `format`, and `check-format` pass. No x64 build was
  run. The gameplay worker remains intentionally disabled; the immediate next
  slice is runtime sprite creation/frame selection, followed by safe initial
  placement and worker activation.

## XTET figurine sprite selection

- Falling figurines use the serialized bitmap collections, not the RLI files.
  Original family 1 selects the 10-child `man` collection; family 0 selects the
  20-child `woman` collection.
- Odd signed orientations select child `shape_index`. Even orientations select
  `shape_index + 5` for `man` or `shape_index + 10` for `woman`.
- Sprite flag `0x800` is horizontal mirroring and `0x1000` is vertical
  mirroring. Confirmed orientation mapping is: `1/2` neither, `3/4` both,
  `-4/-1` horizontal, and `-3/-2` vertical.
- Board-to-screen placement is exactly `x = column * 17 - 45` and
  `y = row * 17 - 45`. `FigurineSpriteSelection` now exposes the confirmed
  family, frame, mirror flags, and position without reproducing the legacy
  ownership-heavy sprite clone object.
- Ghidra now names/documents `CalculateFigurineScreenPosition`,
  `CreateFigurineSprite`, `ApplyFigurineOrientationMirroring`,
  `SetSpriteHorizontalMirror`, `SetSpriteVerticalMirror`, and
  `SetSpritePosition`. Win32 Debug build, CTest, `format`, and `check-format`
  pass; no x64 build was run.
- Next resolve the selected serialized TSprBmp child to its bitmap map and
  reproduce its framebuffer composition/dirty rectangle. RLI remains associated
  with later match animation rather than normal falling-piece drawing.

## XTET figurine bitmap rendering

- Every `man` and `woman` collection child is a transparent `TSprBmp` containing
  exactly one `TBmp` map. The confirmed paths are `m1_..m5_`, `rm1_..rm5_`,
  `w1_..w10_`, and `rw1_..rw10_` BMPs, matching the parity-based frame ranges.
- `render_figurine_sprite` resolves the family link and selected child through
  `SceneDescription`, validates that serialized structure, finds the decoded map,
  and composes it with index-zero transparency and the selected mirror flags.
  It does not hard-code the bitmap filename mapping.
- The returned dirty region is clipped to the framebuffer. Ghidra confirms the
  original root flush passes clipped left/top in ECX/EDX and width/height on the
  stack to host callback slot 0.
- Tests resolve woman shape 7 / orientation -3 to the independently selected
  `w8_.bmp`, compare the complete mirrored/clipped framebuffer, and verify its
  dirty bounds. Win32 Debug build, CTest, `format`, and `check-format` pass; no
  x64 build was run.
- Ghidra now names/documents `UpdateSpriteDirtyRegion` and
  `PropagateSpriteInvalidation`, and documents `FlushDirtyRects` callback
  argument recovery. Next implement board removal/insertion and a reversible
  move redraw using these rendering primitives; worker activation remains gated.

## XTET board mutation and basic movement

- `RuntimeTables::clearValue` reproduces the confirmed scan that removes every
  occurrence of an object pointer from all 20 board rows. Bounded
  `place_figurine_on_board` validates first, then inserts the object into every
  nonnegative occupied template cell.
- `FigurineMove` covers the original commands: rotate, up, right, down, and left.
  Signed orientations advance through two distinct cycles:
  `4->1->2->3->4` and `-4->-1->-2->-3->-4`.
- Rotation uses the previously recovered per-family/per-shape transition offset
  indexed by the new orientation. A move validates while the old self-pointers
  remain in the board, then clears and reinserts them. Failure restores current
  orientation/column/row from the committed previous fields, matching the
  original rather than merely undoing an arithmetic delta.
- Successful portable moves commit previous orientation/position after board
  replacement, corresponding to the end of the original sprite update. The
  original non-rotation collision branch may initiate a match; that branch is
  deliberately not claimed by this boolean basic-move API yet.
- Tests cover insertion, horizontal movement, exact rotation correction,
  replacement without stale cells, boundary rejection, rollback, and occupancy
  preservation. Ghidra now names/documents `RemoveObjectFromRuntimeTables` and
  expands movement/orientation comments. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- Next recover `FindMatchCandidate` and the paired-object rules so a blocked
  directional move can distinguish ordinary rejection from the original match
  result before wiring live input or the worker.

## XTET match compatibility and candidate search

- `acts.txt` records are normalized around the five-shape `man` family. Fields
  0..4 are man shape, woman shape, normalized woman orientation, normalized
  relative x plus 2, and normalized relative y plus 2. Remaining fields belong
  to the later animation behavior. Same-family figurines never match.
- `find_matching_action` reproduces the original orientation normalization:
  rotate the relative center and woman orientation into the man's local frame,
  then mirror the normalized x/orientation when the man's signed orientation is
  negative. It accepts either argument order while always normalizing man first.
- `find_match_candidate` scans hypothetical source centers through a three-cell
  segment in the blocked direction (rotation never matches), then scans each
  center's 5x5 board neighborhood in original row/column order. It requires an
  opposite-family registered board object, an action record, and valid placement
  while ignoring both pair pointers.
- The path guard walks intermediate centers from the committed previous position
  toward the hypothetical match. It rejects a path only when a source template
  cell with stored value 2 crosses an occupied object's oriented value-2 cell;
  values 1 and 2 therefore remain observably distinct beyond ordinary collision.
- On success, the source remains at the hypothetical match center and the result
  contains both candidate and action. On failure, column/row are restored from
  previous fields. Tests cover all 548 canonical action keys plus a real
  directional board search from a displaced source.
- Ghidra now names/documents `FindMatchingActionDefinition` and
  `CheckMatchPathCrossing`, with expanded `FindMatchCandidate` documentation.
  Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run. Next recover how action fields 5..9 drive `AnimateMatchedPair`, scoring,
  removal, and post-match spawning before integrating the match result into a
  live movement controller.

## XTET matched-pair animation planning

- Action field 5 is a bitmask: bit 0 selects the alternate four-frame man RLI
  sequence, bit 1 selects the alternate three-frame woman sequence, and bit 2
  selects alternate temporary-layer slots. Exact per-shape frame bases and the
  two slot-order tables are represented in `MatchAnimationPlan`.
- Action fields 6/7 and 8/9 are signed canonical animation-anchor vectors for
  man and woman. Both are transformed into screen orientation using the man's
  signed orientation, then added to each figurine's `coordinate*17-45` origin.
  The blink's expanded variant moves every component one pixel farther from zero.
- Man uses `m.rli` or `rm.rli` by orientation parity and renders four frames;
  woman uses `w.rli` or `rw.rli` and renders three. Both reuse the confirmed
  orientation mirror mapping.
- RLI evidence corrected an earlier model: header word +6 is the maximum frame
  index, so all `max_index+1` records are renderable. The final payload ends at
  declared file size. The decoder now includes all 40/40/51/51 frames and tests
  lock updated aggregate hashes. Every one of 548 action plans stays in bounds.
- Matched animation is rendered once, then blinked twice with 400 ms between
  expanded and normal variants. Destruction afterward hides each figurine,
  removes its board slots, frees it, and increments score once; a pair scores 2.
- Ghidra now names/documents `BlinkMatchedPair`, `DestroyFigurine`, and
  `UpdateScoreAfterFigurineRemoval`, and has corrected animation/RLI comments.
  Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run. Next compose planned RLI frames into the framebuffer and expose each
  synchronous dirty update without reproducing the original busy-wait timing.

## XTET matched-pair framebuffer composition

- `blit_rli_frame_canvas` composites a sparse decoded RLI patch in its full
  animation canvas. Horizontal/vertical mirroring is applied around the 108x108
  canvas, not merely around the patch bounds, matching the original temporary
  sprite path.
- `render_match_animation_plan` snapshots the framebuffer after the ordinary
  pair sprites have been hidden, reveals four man frames followed by three woman
  frames, and emits one clipped dirty region for every reveal.
- Revealed frames are retained as seven logical temporary layers. Before each
  update the background is restored and active layers are recomposed by fixed
  scene-slot number, not reveal order. This preserves the confirmed action flag
  bit-2 overlap behavior: man slots `[1,2,5,3]`/`[1,2,5,4]`, then woman slots
  `[0,4,6]`/`[0,3,6]`.
- Composition preserves RLI coverage independently from pixel value, so covered
  zeroes overwrite while RLE gaps retain lower layers/background. Tests verify
  seven callbacks, clipped participant bounds, and observable final pixels.
- Ghidra comments now document both slot tables and fixed-slot scene ordering.
  Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run. Next add the expanded/normal blink sequence and portable destruction/
  score transition, with timing supplied by a controller rather than busy waits.

## XTET match blink and score progression

- `render_match_blink_sequence` snapshots the post-hide background, renders the
  normal seven-layer effect, requests 400 ms delays around expanded and normal
  variants for two cycles, then restores the background and emits seven ordered
  hide dirty regions. It never sleeps or busy-waits; timing is an injected
  callback for later worker/message-loop integration.
- The complete confirmed sequence produces five seven-layer reveal passes plus
  seven hides: 42 synchronous dirty callbacks and four 400 ms delay requests.
  Tests verify cadence, callback count, and final background restoration.
- `GameProgress` has no guessed constructor state. Given explicitly initialized
  state, `update_progress_after_figurine_removal` acts only in gameplay state 1,
  increments score once per figurine, calculates `base_level + score/30`, updates
  levels below 11, and changes gameplay state to 3 at level 11 while retaining
  level 10. A matched pair calls this transition twice and scores 2.
- Ghidra globals now identify gameplay state, base/current level, and removed-
  figurine score with confirmed types/comments. Win32 Debug build, CTest,
  `format`, and `check-format` pass; no x64 build was run.
- Next recover the destruction/controller ownership around the matched pair and
  spawning that follows it, then connect these portable pieces to input without
  yet enabling the autonomous fall worker.

## XTET active ownership and keyboard movement

- The original maintains one active falling pointer in addition to board-owned
  figurines. A worker tick creates/inserts a piece only when this pointer is null.
  Spawn failure changes gameplay state to 2. A successful downward move retains
  ownership; downward rejection clears only the active pointer and leaves the
  settled figurine registered in the board.
- `GameplayMoveResult` now distinguishes rejected, moved, and matched outcomes.
  `process_falling_move` performs normal transactional movement first and invokes
  the recovered candidate search only after a rejected non-rotation move.
- Confirmed keyboard mapping is Space=hard drop, Left=left, Up=rotate,
  Right=right, Down=down. Hard drop repeats down while the result is moved.
  Ordinary rejected keyboard moves retain the active pointer; a keyboard match
  clears it, settles cascades, and drains pending keyboard messages.
- `remove_matched_pair` captures both registry values before erasing entries,
  removes both objects from every board slot, and applies two per-figurine score
  transitions. Portable ownership/deallocation remains with the caller rather
  than exposing dangling legacy pointers.
- After a match, the original scans bottom-to-top and left-to-right, repeatedly
  moving each encountered object down. A newly produced match restarts the scan
  from the bottom. This cascade settling is the next implementation slice.
- Ghidra now names/documents `HandleGameplayKey`, `SettleBoardAfterMatch`,
  `ClearAllFigurines`, and `g_pActiveFallingFigurine`. Win32 Debug build, CTest,
  `format`, and `check-format` pass; no x64 build was run. Worker activation
  remains gated.

## XTET post-match cascade settling

- `settle_board_after_match` now reproduces the confirmed mutable-board scan:
  rows are visited from 19 to 0 and columns from left to right, and every
  encountered registered figurine is repeatedly moved down until movement is
  rejected or produces another match.
- A cascade match invokes an injected synchronous effect callback before both
  figurines are removed. Removal clears their board slots and registry entries,
  applies the two per-figurine score transitions, and restarts the complete scan
  from the bottom.
- The portable result records successful downward moves and cascade matches.
  Invalid board-to-registry state or a failed effect callback aborts cleanly;
  ownership and rendering remain outside this board controller.
- A focused test verifies an unmatched board-owned figurine falls to its last
  legal row, remains registered, does not invoke the effect callback, and does
  not change score. Win32 Debug build, CTest, `format`, and `check-format` pass;
  no x64 build was run.
- Next recover and implement the worker tick/spawn transition around automatic
  falling, while keeping worker activation gated until the full initialization
  path can safely succeed.

## XTET automatic gameplay tick

- `update_game_tick` now represents the confirmed gameplay-state-1 worker body
  without creating a thread or selecting an unverified scheduling policy.
- With no active figurine, one injected spawn operation is attempted. A valid
  spawned board/registry object becomes active; spawn failure sets gameplay
  state 2. The controller rejects a callback result that is not represented in
  the board registry instead of publishing inconsistent state.
- With an active figurine, one automatic downward movement is processed. A move
  retains the active designation, ordinary rejection clears only that
  designation and leaves the settled board object registered, and a match runs
  the synchronous effect, removes the pair, clears active state, and settles all
  cascades.
- Tests cover successful spawn, one-step movement, repeated ticks through
  settling, retained board ownership, and the state-2 spawn-failure transition.
  Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run. The DLL worker remains deliberately gated.
- Next recover the exact keyboard-controller orchestration around hard drop,
  match effects, cascade settling, and pending-key draining so worker and input
  can share this portable state safely before DLL activation.

## XTET keyboard gameplay controller

- Ghidra reconfirmed `HandleGameplayKey`: Space repeats downward movement only
  while the result is 1/moved; Left, Up, Right, and Down issue left, rotate,
  right, and down respectively. An absent active object or an unrelated key is
  ignored.
- `handle_gameplay_input` now preserves the caller-specific ownership behavior.
  A successful ordinary key move retains active state. Rejection, including the
  rejection that terminates a hard drop, also retains active state; the next
  automatic tick is responsible for marking that piece settled.
- Only result 2/matched runs the injected synchronous effect and pair removal,
  clears the active designation, settles cascades, and finally requests pending
  keyboard-message draining. The Win32 adapter will implement the confirmed
  `PeekMessage` range `0x100..0x108`; the portable core only exposes the ordered
  drain boundary.
- Tests verify a hard drop performs multiple downward moves, stops on rejection,
  retains the active piece, does not drain keys, and is settled by the following
  automatic tick. Win32 Debug build, CTest, `format`, and `check-format` pass;
  no x64 build was run.
- Next integrate the recovered figurine redraw/dirty-region sequence with
  movement so portable state changes can be presented correctly before wiring
  the controller into the DLL boundary.

## XTET movement presentation phases

- Ghidra reconfirmed the successful `MoveFallingFigurine` order: invalidate the
  old occupied board cells with phase 0/flags `0x13`, remove old slots, insert
  new slots, invalidate the new cells with phase 1/flags 0, update the runtime
  sprite, then commit orientation/column/row into the previous fields.
- The helper at `0x1001e830` scans the 20-by-width occupancy table for the exact
  object and invalidates each matching 17-by-17 board cell. It is now named
  `InvalidateFigurineBoardCells` in Ghidra.
- `try_move_falling_figurine` exposes an optional portable board-change callback.
  It emits the committed old figurine with `adding=false` immediately before
  clearing slots and the new figurine with `adding=true` immediately after
  insertion. Rejected moves emit neither phase. The later rendering adapter can
  use these boundaries without embedding scene or Win32 dependencies in board
  mutation.
- Tests lock the remove/add callback order and old/new columns for a successful
  move. Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build
  was run.
- Next propagate the presentation boundary through `process_falling_move`, the
  tick controller, and keyboard controller, then connect it to a renderer that
  can restore the old sprite area and draw the new sprite in scene order.

## XTET controller-wide presentation propagation

- The optional `FigurineBoardChangeCallback` now flows through
  `process_falling_move`, post-match cascade settling, automatic gameplay ticks,
  and keyboard input handling. All recovered callers therefore expose the same
  old-remove/new-add phases for every successful movement.
- Hard drop emits one ordered remove/add pair for each successful downward step
  and emits no phase for its terminal rejection. Automatic falling and cascade
  gravity use the same hook, so the eventual renderer does not need separate
  controller-specific movement logic.
- Tests verify the hard-drop callback count is exactly twice its successful move
  count and that every pair is remove then add. Win32 Debug build, CTest,
  `format`, and `check-format` pass; no x64 build was run.
- Next implement the portable presentation adapter that converts each figurine
  phase into the confirmed 17-by-17 occupied-cell dirty regions and updates the
  normal figurine sprite without guessing unconfirmed scene-layer behavior.

## XTET occupied-cell presentation adapter

- `collect_figurine_board_regions` transforms the confirmed oriented 5-by-5
  template into one clipped framebuffer region per nonzero cell. Regions use the
  original board mapping `x = column * 17`, `y = row * 17`, width/height 17, and
  retain row-major scan order; above-board cells and fully clipped cells emit no
  region.
- `make_figurine_board_change_callback` adapts controller remove/add phases to
  synchronous region callbacks while preserving the phase. This is the portable
  counterpart of Ghidra's `InvalidateFigurineBoardCells` scan.
- Pixel restoration is intentionally not claimed yet. The adapter exposes exact
  invalidation regions but does not erase an old transparent sprite until the
  original scene compositor's overlap/redraw behavior is sufficiently recovered.
- Tests verify nonempty clipped regions, 17-pixel grid alignment, row-major
  ordering, dimensions, and complete remove-then-add phase propagation. Win32
  Debug build, CTest, `format`, and `check-format` pass; no x64 build was run.
- Next recover the root scene redraw invoked by these cell invalidations so old
  transparent sprite pixels can be restored from lower scene layers before the
  updated figurine sprite is drawn.

## XTET dynamic figurine scene slots

- `CreateFigurineSprite` scans runtime scene child slots starting at index 3 and
  ending at `child_count - 14`, selecting the first null slot. A parity-changing
  sprite replacement recovers and reuses the existing child's index rather than
  moving the figurine to a new layer.
- Ghidra global `0x100515a4`, initialized to that `child_count - 14` boundary,
  is now named `g_nLastFigurineSceneSlot`.
- The runtime-expanded child container is not directly represented by the
  current flattened serialized `SceneDescription`; a test-only diagnostic
  confirmed that deriving the range from a plausible script node was invalid,
  and that hypothesis was removed.
- `FigurineBoardEntry` now records an architecture-sized scene slot, and
  `find_free_figurine_scene_slot` reproduces first-free scanning over the
  explicitly supplied confirmed range. Tests cover first-slot allocation and
  skipping an occupied slot. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- The runtime container's final upper bound still must be supplied explicitly;
  its board-mask update path is recovered separately below.

## XTET board-mask framebuffer updates

- The `InvalidateFigurineBoardCells` path resolves each occupied table slot to a
  17-by-17 rectangle and calls the generic indexed bitmap rectangle fill. Phase
  0 fills old occupied cells with palette index `0x13`; phase 1 fills new
  occupied cells with index 0. `UpdateFigurineSprite` follows these fills.
- `fill_figurine_board_regions` now performs this exact clipped indexed fill, and
  `make_figurine_framebuffer_change_callback` combines it with the recovered
  remove/add controller phases and synchronous region notification.
- This corrects the earlier unresolved interpretation: the operation is a board
  mask update, not an inferred redraw of arbitrary lower scene layers. Normal
  figurine sprite drawing remains a distinct subsequent operation.
- Ghidra now names `FillIndexedBitmapRectangle` at `0x1000f2f0`,
  `AttachSceneChildAtSlot` at `0x10015790`, and `FindSceneChildIndex` at
  `0x100141d0`.
- Tests verify the exact number of pixels filled with `0x13`, their replacement
  with zero in the add phase, and region callback propagation. Win32 Debug
  build, CTest, `format`, and `check-format` pass; no x64 build was run.
- Next compose the post-fill `UpdateFigurineSprite` behavior into a portable
  presentation callback: preserve the dynamic slot, choose/replace the bitmap
  on orientation-parity changes, apply mirrors/position, and emit sprite dirty
  bounds after the board mask phases.

## XTET normal movement sprite presentation

- `render_figurine_board_change` now combines the recovered successful-move
  presentation order. Both phases update the indexed board mask and synchronously
  emit each occupied-cell region. The add phase then selects the correct
  man/woman bitmap variant, applies the recovered orientation mirrors and
  `column*17-45`/`row*17-45` position, draws the transparent sprite, and emits
  its clipped sprite dirty region.
- `make_figurine_presentation_callback` exposes this combined operation through
  the controller-wide `FigurineBoardChangeCallback`. Scene and bitmap resources
  are borrowed and must outlive the returned callback.
- Dynamic scene-slot identity remains stored separately in `FigurineBoardEntry`;
  replacing a parity-dependent bitmap does not allocate a new logical slot in
  the portable model.
- Tests compare the combined remove/add framebuffer against independent mask
  fill plus sprite composition, verify two complete board-region phase sets,
  and require exactly one add-phase sprite region. The final Win32 Debug build
  is warning-clean; CTest, `format`, and `check-format` pass. No x64 build was
  run.
- Next connect this callback to a higher-level spawn operation that allocates a
  stable scene slot, inserts the new figurine into occupancy/registry state,
  performs its initial mask/sprite presentation, and can be injected directly
  into `update_game_tick`.

## XTET atomic falling-figurine spawn

- Ghidra reconfirmed `CreateFallingFigurine`: after recovered random family,
  balance, shape, orientation, centered column, and row-2 initialization, it
  repeatedly decrements row until placement succeeds. If the resulting row is
  below 1, creation fails without board insertion; the already-updated family
  balance is retained.
- On successful live creation, the original places occupancy first, applies the
  add-phase board mask, creates/updates the sprite, then commits previous
  orientation/column/row. Allocation failure or blocked entry returns null to
  the worker, which later changes gameplay state to 2.
- `spawn_falling_figurine` now reproduces this portable sequence using caller-
  owned figurine storage and identity. It rejects duplicate identities, inserts
  the registry entry, assigns the first free scene slot when available, invokes
  the shared add-phase presentation callback, and commits previous fields.
  Exhausted scene slots retain an invalid slot, matching the original's ability
  to return a gameplay object even when sprite cloning returns null.
- Tests cover successful row-2 insertion, scene slot 3, registry/board identity,
  add-only presentation, committed previous state, and failure against a fully
  occupied board while retaining the family-balance change. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.
- Next bind this spawn helper and combined presentation callback into a portable
  runtime-state object so `update_game_tick` no longer needs ad-hoc test lambdas,
  then prepare the same object for ordinal-2 keyboard dispatch.

## XTET portable gameplay runtime owner

- `GameplayRuntime` now owns the architecture-neutral runtime tables, stable
  heap-allocated figurine objects, board registry, active identity, family
  balance, explicit last scene slot, and caller-supplied initialized
  `GameProgress`.
- `GameplayRuntime::updateTick` injects `spawn_falling_figurine` into the
  recovered tick controller using caller-provided random values. This keeps
  random sequencing deterministic and leaves the eventual Win32 `rand` calls at
  the adapter boundary.
- Board entries use the stable `FallingFigurine*` as their object identity.
  After match/cascade removal, the runtime prunes owned figurines no longer
  present in the registry, reproducing destruction without exposing dangling
  pointers to later ticks.
- Tests initialize a runtime, verify the first tick spawns/activates one entry
  with an add presentation phase, and verify the next tick moves the same active
  object with one remove/add pair. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- Next add `GameplayRuntime::handleInput` over the recovered keyboard controller,
  including hard drop, match/cascade ownership cleanup, and injected pending-key
  draining. Then the runtime will be ready for ordinal-2 message wiring.

## XTET owned runtime input dispatch

- `GameplayRuntime::handleInput` now routes the owned board, registry, active
  identity, and progress through the recovered keyboard controller. Match and
  cascade removal are followed by the same safe owned-object pruning used after
  automatic ticks.
- Presentation and pending-key draining remain injected boundaries. This lets
  ordinal 2 later provide the Win32 `PeekMessage` operation while deterministic
  tests observe the exact ordering without a window queue.
- Tests hard-drop the runtime's active figurine, verify multiple remove/add
  presentation pairs, terminal rejection with active identity retained, and no
  key drain without a match. The following automatic tick clears only active
  state, emits no movement phases, and leaves the settled entry board-owned.
- Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run.
- Next recover the ordinal-2 message/state dispatch around `WM_KEYDOWN`, pause,
  focus/activation, and result states, then wire the confirmed subset to
  `GameplayRuntime` while initialization remains gated.

## XTET ordinal-2 outer message dispatch

- Ghidra reconfirmed ordinal 2 at `0x10020dc0`: gameplay states 1, 2, 3, and 4
  enter the game critical section, dispatch a small message subset, and return
  0. All other gameplay states return 1 without locking or dispatching.
- The accepted message subset is exactly `WM_DESTROY` (`0x0002`), `WM_KEYDOWN`
  (`0x0100`), `WM_LBUTTONDOWN` (`0x0201`), and `WM_LBUTTONUP` (`0x0202`). An
  accepted-state message outside this set is ignored but still returns 0. Mouse
  dispatch receives true for button-down and false for button-up.
- `dispatch_game_window_message` captures this architecture-neutral outer
  contract with injected destroy, key, and mouse callbacks. The DLL's existing
  mutex will remain the Win32 synchronization boundary when it is connected.
- Ghidra now names `HandleGameKeyDown` at `0x10020ed0`,
  `HandleGameMouseButton` at `0x10021070`, and `StopGameplay` at `0x100219d0`.
- Tests cover inactive-state rejection, all four active states, exact callback
  arguments, accepted ignored messages, and return values. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.
- Next implement the inner `HandleGameKeyDown` state machine: state-1 Escape and
  gameplay keys, state-2/3 deadline gating and queued-key draining, result
  posting, and state reset. Mouse hit-testing remains separate.

## XTET ordinal-2 key state machine

- `HandleGameKeyDown` behavior is now represented by `handle_game_key_down`.
  In gameplay state 1, Escape stops gameplay, posts the current removed-figurine
  score descriptor, and posts private message `0x7ffc` with termination lparam
  0; other keys are forwarded to the gameplay keyboard controller.
- State 4 also honors Escape with the same exit sequence but otherwise ignores
  keys. States 2 and 3 ignore key identity: before the unsigned millisecond
  deadline they drain queued keyboard messages `0x100..0x108`; at or after the
  deadline they set state 0, stop gameplay, post the score, and post termination.
- The state reset occurs before the injected stop callback, matching
  `StopGameplay` setting the global state to 0 before cleanup.
- Ghidra global `0x1003c798` is now named `g_dwResultInputDeadline`.
- Tests lock normal state-1 forwarding, Escape event order and score, early
  state-2 draining even for Escape, deadline equality in state 3, and ignored
  non-Escape state-4 keys. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- Next recover `StopGameplay` cleanup and the exact result-deadline setup in the
  state-2 spawn-failure and state-3 completion paths, then bind these key
  callbacks to the DLL's private-message helpers.

## XTET result transition and portable stop cleanup

- The only writes to `g_dwResultInputDeadline` are confirmed in the state-2
  spawn-failure path and the state-3 level-completion path. Both stop loop music,
  queue their result sound, call `timeGetTime`, and store the unsigned value plus
  exactly 2000 milliseconds.
- `GameplayRuntime::updateTick` now accepts the adapter's current millisecond
  value and records `calculate_result_input_deadline(current_time)` whenever a
  tick transitions from state 1 to state 2 or 3. Unsigned wraparound is retained.
- `GameplayRuntime::stop` covers portable cleanup: state becomes 0, active state
  clears, owned figurines and registry entries are released, and runtime pointer
  tables are deinitialized. The DLL adapter must separately stop/join the worker
  and release GDI, scene, resource, and audio objects in recovered order.
- Ghidra now names `DestroyGameResources` at `0x10020a70` and
  `UpdateGameTick` at `0x1001e281`.
- Tests force a fully blocked runtime spawn, verify state 2 and the wrapped
  two-second deadline, then verify portable stop cleanup. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.
- Next bind the recovered ordinal-2 outer/key dispatchers to `dll.cpp` using the
  shared private result-message helpers and `PeekMessage` drain callback, while
  keeping gameplay initialization false until worker/audio setup is complete.

## XTET gated ordinal-2 DLL binding

- `dll.cpp` now owns a `GameplayRuntime` and routes ordinal 2 through the
  recovered accepted-state/message dispatcher whenever initialization is true.
  The existing DLL mutex supplies the critical-section boundary.
- The key path uses `timeGetTime`, the runtime result deadline and score, and the
  recovered stop/result/termination callbacks. Result posting reuses the exact
  descriptor helper; termination posts private message `0x7ffc` with wparam 0
  and lparam 0.
- Keyboard draining exactly loops `PeekMessageA` for the host window and range
  `0x100..0x108` with `PM_REMOVE | PM_NOYIELD` (numeric value 3). `WM_DESTROY`
  performs portable runtime stop cleanup without posting a result.
- Normal state-1 gameplay-key forwarding is explicitly left unbound until the
  synchronous match-effect callback can be passed to
  `GameplayRuntime::handleInput`. Initialization remains deliberately false, so
  this incomplete inner path cannot execute or be described as playable.
- The reconstructed DLL now links WinMM for the original `timeGetTime` clock.
  Win32 Debug build, CTest, `format`, and `check-format` pass; no x64 build was
  run.
- Next construct the DLL match-effect callback from board-mask hiding, RLI blink
  animation, dirty callbacks, delays, score rendering, and recovered sound
  queueing; then bind ordinary gameplay keys safely.

## Reconstructed XTET source

- The root build now produces a reconstructed `XTETDLL.DLL` from `xtet/` and
  copies it beside `xtet_loader`; it no longer copies the original DLL as the
  runtime target.
- `xtet/api.h` is shared by the loader and DLL. It preserves the original x86
  `__fastcall` exports, 0x40-byte host context, result descriptor, 35 callbacks,
  and ordinals 1 through 3. Non-x86 builds use the platform-native ABI and
  pointer widths and are intended for matching recompiled hosts only.
- Win32 MSVC Debug builds succeed. Win32 `dumpbin` verification shows an
  x86 DLL with named exports at ordinals 1, 2, and 3 backed by the expected
  decorated `__fastcall` functions.
- The current DLL is an evidence-labeled foundation slice, not a playable
  reconstruction. It validates and mounts the embedded resource header, then
  reports the original initialization-failure result/message protocol because
  SFS file access and gameplay have not yet been recovered.
- Root configuration requires `XTET_ASSET_DIR` when the reconstructed DLL is
  enabled. Configure with `XTET_BUILD_RECONSTRUCTED_DLL=OFF` to bootstrap the
  extractor before prepared resources exist.

## XTET resource extraction and SFS

- `tools/xtet_resource_extractor` parses PE32 and PE32+ resource trees without
  loading the DLL. It requires exactly one named `XTETSFS` `RT_RCDATA` resource
  and atomically writes `XTETSFS.bin`, deterministic JSON metadata, and an RC
  script into a new output directory.
- The original payload is 2,867,586 bytes with SHA-256
  `e53a8a846b842469a3dd6c567eca7fa519762b4b59dc402a48075399a8edb26b`.
  Extraction from the reconstructed DLL round-trips to the same hash.
- The embedded SFS header is 0x100 bytes, begins with `SFS\0`, requires version
  200, and uses an additive checksum over the header with the checksum field
  cleared. Confirmed fields include a 32-byte-entry directory offset/count, a
  32-KiB-block allocation-map offset, data offset, logical size, and maximum
  path length. `SfsArchive` validates these fields without packed/unaligned
  reads or pointer-width assumptions.
- Ghidra globals at `0x10052568` through `0x10052588` now carry synchronized
  names, types, and comments for the confirmed copied SFS header fields.
## SFS file access and action definitions

- The SFS directory is already stored as sorted 32-byte entries; it is not
  decrypted during mount. Each key is a pair of 32-bit hashes of the uppercased
  relative pathname, followed by attributes, virtual offset, logical size, and
  runtime/status fields.
- The two 256-entry hash tables are linear XOR tables. The reconstructed reader
  stores their eight basis values each and reproduces the original dual hash
  without copying 2 KiB of generated table constants.
- The allocation map contains stored-resource offsets for each 32 KiB virtual
  block plus a terminal offset. Blocks may be raw, marker-1 LZSS with a 4 KiB
  window initialized at `0xfee`, or marker-2 data using the existing type-0/raw
  and type-8/deflate stream format.
- `SfsArchive` now validates directory sorting, allocation-map monotonicity and
  bounds, binary-searches paths, and reads files across raw or compressed blocks.
  The shared legacy inflater gained explicit input/output bounds reporting.
- `acts.txt` is directory entry 92: hash pair `fb32894c:a8cf14d6`, virtual offset
  `0x302678`, and length `0x3171` (12,657 bytes). It is successfully recovered
  from a deflate-compressed block.
- `ActionDefinition` reproduces the ten one-byte fields loaded by the original.
  Parsing the recovered file yields exactly 548 accepted records after applying
  the confirmed bounds on the first three fields.
- CTest covers mount/checksum rejection, case-normalized lookup, missing paths,
  compressed `acts.txt` reads, its confirmed directory metadata, and record count.
  Win32 Debug builds, CTest, and `check-format` pass. Per the current user
  direction, x64 builds are not run during this phase.
- The reconstructed DLL now completes resource mount, `acts.txt` read, and action
  parsing before intentionally returning initialization failure. Immediate next
  step is binary BMP/WAV decoding and construction, followed by initial indexed
  bitmap and sprite state.
- Ghidra now names and documents the dual hash, mounted-entry lookup, virtual
  open/read/seek/close, SFS block loader/backing-store reader, and LZSS decoder.

## XTET serialized asset declarations

- Initialization calls `LoadSerializedObjectLinks` (`0x100209e0`) three times
  with `base_scr.txt`, `man.txt`, and `woman.txt` in ECX. It constructs the
  legacy text deserializer, repeatedly creates registered top-level objects for
  their ObjLink side effects, releases them, and returns success.
- The recovered scripts are brace-delimited text with semicolon line comments,
  DOS control-Z terminators, and recursive `INCLUDE` directives. The root screen
  includes `base_tet.txt` and `keys.txt`; further includes yield seven distinct
  scripts in total.
- `AssetManifest` tokenizes these scripts directly from `SfsArchive`, rejects
  malformed braces, include cycles, unreadable includes, and missing referenced
  assets, and records the declarations without inventing legacy runtime layouts.
- The confirmed manifest contains 63 bitmap `LOAD` references and 18 `TWave`
  references. These include the 640x480 `xtet.bmp`, `digit.bmp`, control artwork,
  man/woman frames, and loop/action/result audio.
- DLL initialization now resolves and validates the complete declaration graph
  after parsing `acts.txt`, then deliberately reports initialization failure at
  the still-unrecovered binary BMP/WAV construction boundary.
- Ghidra names, prototypes, and documents `LoadSerializedObjectLinks`. The latest
  Win32 Debug build, CTest, `format`, and `check-format` all pass.

## XTET bitmap and wave decoding

- `IndexedBitmap` decodes the exact BMP variant used by every declared XTET
  bitmap: a bounded BITMAPINFOHEADER-or-later file, one plane, 8-bit indexed,
  uncompressed pixels, up to 256 BGRA palette entries, DWORD-aligned stored rows,
  and normalized top-down output pixels. All 63 bitmap declarations decode; the
  main `xtet.bmp` is confirmed as 640x480.
- `WavePcm` follows the recovered original loader: bounded RIFF/WAVE chunk
  traversal, a `fmt ` chunk of at least 16 bytes, PCM format tag 1, and the first
  following `data` chunk. Its architecture-neutral `PcmFormat` is exactly the
  16-byte descriptor passed across the host callback boundary. All 18 declared
  wave assets decode with nonempty, block-aligned sample data.
- Full 32 KiB type-8 SFS blocks can fill the legacy inflater output before it
  consumes a clean stream terminator. This is accepted only when the complete
  logical block was produced; partial blocks still require clean inflater
  completion. This fixes cross-block reads such as `act1.wav` (54,714 bytes).
- DLL initialization clears prior decoded state, resolves the declaration graph,
  and owns each unique decoded bitmap/wave by path. It still deliberately fails
  at the next unrecovered boundary: construction of the serialized sprite
  hierarchy and its named ObjLinks.
- Ghidra's `LoadWavePcmData` now has the confirmed thiscall/path prototype and a
  comment documenting RIFF chunks, object fields, and the 16-byte PCM layout.
  Win32 Debug build, CTest (including malformed BMP/WAVE rejection), `format`,
  and `check-format` pass. No x64 build was run.

## XTET scene hierarchy

- `SceneDescription` reconstructs the complete serialized declaration hierarchy
  from `base_scr.txt`, `man.txt`, and `woman.txt`, recursively splicing each
  single-root `INCLUDE` while rejecting cycles and malformed structure.
- Confirmed node types are `TSprites`, `TSprBmp`, `TBmp`, `TWave`, and `EMPTY`.
  Confirmed properties are `SIZE`, `MAP_SIZE`, `TO`, `SHOW`, `VIEW`, `TRANSP`,
  `FILL`, `item`, `map`, `LOAD`, and `CREATE`. Values whose constructor defaults
  are not yet recovered remain optional rather than receiving guessed defaults.
- The exact recovered graph has 22 top-level objects and 269 total nodes: 7
  sprite containers, 69 sprite-bitmap nodes, 71 bitmap maps, 18 waves, and 104
  empty child slots. The empty slots are observable index reservations and are
  retained rather than compacted away.
- Post-object link tokens are retained and queryable. Confirmed multiplicities
  include 63 `pal_none` bitmap bindings, 8 `loop` waves, 6 `act` waves, and one
  each of `home_scr`, `map_digit`, `man`, and `woman` (plus the singleton result
  wave bindings).
- Structural tests verify the 640x480 home declaration, three expanded home
  children, digit links, 10 man frames, 20 woman frames, all graph counts, and
  named-link multiplicities. DLL initialization now owns this scene description
  and deliberately stops at runtime sprite-object/framebuffer construction.
- Ghidra now names `SerializeTSpritesProperties` and labels the TSprites, TWave,
  and TSprBmp class-registration thunks. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.

## XTET initial indexed rendering

- Constructor tracing confirms that sprite flags default to zero and that
  serialized `SHOW` and `VIEW` are distinct bits. Initial scene traversal must
  therefore render only nodes with an explicit true `SHOW`; `VIEW` alone does
  not make a sprite visible.
- The confirmed initial opaque composition is `xtet.bmp` at `(0, 0)` followed
  by `f01.bmp` at `(243, 77)`. It follows the visible chain `home_scr` ->
  `base_tet` -> `base_fon`; the key sprites and man/woman roots are initially
  hidden.
- `IndexedFramebuffer`, `blit_opaque`, and `render_initial_scene` implement the
  architecture-neutral clipped 8-bit row-copy path. Visible transparent paths
  are rejected until the original transparency routine is recovered.
- DLL initialization renders the confirmed initial composition into the host
  framebuffer and reports the full host rectangle through callback slot 0. It
  still deliberately reports initialization failure immediately afterward at
  the unrecovered runtime-object/gameplay boundary.
- CTest compares the rendered 640x480 frame with an independently composed
  `xtet.bmp` plus clipped `f01.bmp`, and covers negative-coordinate clipping and
  invalid destinations. The full Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- Ghidra now names and documents `InitializeXtetGame` (`0x1001d260`) and
  `BlitOpaqueIndexedBitmap` (`0x1000fbd0`). Initialization next creates
  `m.rli`, `rm.rli`, `w.rli`, and `rw.rli`; recovering that animation-resource
  boundary and the transparent blitter is the immediate next slice.

## XTET RLI animation resources

- `CreateRliAnimation` (`0x10002b30`) allocates a 0x48-byte legacy object and
  calls `LoadRliAnimation` (`0x10001610`). The four initialization calls load
  `m.rli`, `rm.rli`, `w.rli`, and `rw.rli` with resident mode 1.
- The RLI header begins with signature `0x054e`, a 32-bit declared byte size,
  and a 16-bit frame-count field. A fixed prefix ends at offset `0x53a`, followed
  by `frame_count + 1` records of 20 bytes. Record field `+4` is a monotonic data
  offset used by the original to calculate adjacent frame storage sizes.
- Confirmed object properties copied by the loader are signed 32-bit width and
  height at file offsets `0x16` and `0x1a`, and retained flag bit `0x1000` from
  the word at offset `8`. Palette construction uses file data at offset `0x3a`.
- All four animations are 108x108 with flag `0x1000`. Resident byte sizes are
  40,048 (`m`), 38,598 (`rm`), 57,538 (`w`), and 56,818 (`rw`); their renderable
  frame counts are 40, 40, 51, and 51 respectively.
- `RliAnimation` now validates and owns each complete resident file plus its
  bounded records and confirmed data offsets. DLL initialization loads these
  four resources before the initial render, then deliberately fails at RLI
  frame decoding/runtime gameplay construction.
- Tests cover all confirmed sizes/counts/dimensions/flags plus malformed
  signature and oversized declared-length rejection. Ghidra names/comments are
  synchronized. Win32 Debug build, CTest, `format`, and `check-format` pass; no
  x64 build was run.

## XTET RLI frame decoding

- Each 20-byte frame record contains flags at `+0`, payload offset at
  `+4`, palette start/count at `+8/+10`, and signed inclusive
  left/top/right/bottom bounds at `+12/+14/+16/+18`. The following record's
  payload offset terminates a non-final frame; declared file size terminates the
  final frame.
- `RenderRliFrame` selects DWORD-aligned bottom-up raw pixels for flag bit 1,
  BMP RLE8 for bit 2, and an optional palette-entry prefix for bit 4.
  `DecodeRliRle8BottomUp` implements encoded runs, EOL, EOB, delta, and
  word-aligned absolute runs.
- The portable decoder now normalizes frame patches to top-down pixel arrays and
  validates rectangles, payload spans, palette spans, raw row pitch, every RLE
  packet, destination coordinates, and explicit EOB termination.
- All 182 frames decode: `m/rm` have 40 each and `w/rw` have 51 each.
  Every supplied frame uses RLE8 flag 2; none uses raw flag 1 or palette flag 4.
  Aggregate FNV-1a pixel hashes are locked in tests for all four resources.
- Ghidra now names/documents `GetRliFrameData`, `RenderRliFrame`,
  `CopyRawRliFrameBottomUp`, and `DecodeRliRle8BottomUp`. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.
- Next recover how decoded RLI patches are placed/composited by gameplay objects,
  plus the separate transparent TSprBmp rendering path.

## XTET indexed compositing

- `BlitTransparentIndexedBitmap` (`0x10010480`) confirms that TSprBmp
  transparency is an index-zero color key: source zero preserves the destination
  and every nonzero index overwrites it. The routine independently supports
  horizontal and vertical mirroring.
- `blit_transparent` implements that behavior with architecture-neutral clipping
  and mirroring. Scene rendering now accepts explicitly transparent TSprBmp nodes
  instead of rejecting them.
- RLI RLE8 skipping is different from index-zero transparency. Encoded zero runs
  write zero, while EOL/delta gaps preserve existing destination pixels. Decoded
  RLI frames therefore retain a separate per-pixel coverage mask.
- `blit_rli_frame` places the recovered inclusive frame rectangle relative to an
  object origin, clips it to the destination, and writes every covered pixel,
  including covered zero indices. Tests distinguish covered zero from uncovered
  pixels and cover transparent clipping plus both mirror axes.
- Ghidra now names/documents `BlitTransparentIndexedBitmap`. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.
- Next trace figurine runtime-object construction, animation frame selection, and
  initial gameplay placement so initialization can advance toward a live loop.

## XTET runtime pointer tables

- Immediately after the four RLI objects, `InitializeXtetGame` calls
  `InitializeRuntimePointerTables` with `15` in ECX. It first clears prior state,
  then allocates 20 independent arrays of 15 32-bit pointers and zeroes every
  slot. The original global range is `0x1003c830..0x1003c87c` with the shared
  capacity stored separately.
- `RuntimeTables` reproduces this logically with 20 arrays of architecture-sized
  pointers, preserving native pointer widths outside the x86 ABI boundary.
  Initialization is atomic on allocation failure and explicit clearing removes
  all slots and resets the capacity.
- DLL initialization now creates the confirmed 20-by-15 tables after resident
  RLI loading and before the initial render. Tests verify table count, capacity,
  zero initialization, and clearing.
- The following original call, `InitializeFigurineGeometryTables`, derives
  orientation geometry for five entries from one family and ten from another.
  Its source records contain 5x5 occupancy templates and its output contains
  eight coordinate pairs per entry; exact template/transform reconstruction is
  the next boundary.
- Ghidra now names/documents `InitializeRuntimePointerTables`,
  `ClearRuntimePointerTables`, `AllocateRuntimePointerTables`, and
  `InitializeFigurineGeometryTables`. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.

## XTET figurine geometry

- The DLL contains two figurine-template families at `0x1003c880` and
  `0x1003c938`, with 5 and 10 records respectively. Each 36-byte record begins
  with a confirmed 25-byte 5x5 occupancy template. Cell values 1 and 2 are both
  occupied for bounds calculations but remain distinct when transformed.
- The first five occupancy templates are shared between the two families; the
  second family adds five templates. The source bytes are now captured as
  evidence-backed constants in `figurine_geometry.cpp`.
- Eight orientation codes implement four rotations (`1..4`) and four mirrored
  rotations (`-1..-4`). Each derived signed coordinate pair is the integer
  difference between successive occupied-bounds centers. The cycles are
  `4->1->2->3->4` and `-4->-1->-2->-3->-4`.
- `FigurineGeometryTables` builds 5 and 10 arrays of eight signed coordinate
  pairs and is now initialized after the 20-by-15 runtime pointer tables. Tests
  lock every expected derived pair for both families.
- Ghidra now names/documents `BuildFigurineOrientationOffsets`,
  `OrientFigurineTemplate`, `AdvanceFigurineOrientation`, and
  `MeasureFigurineOccupiedBounds`. Win32 Debug build, CTest, `format`, and
  `check-format` pass; no x64 build was run.
- The next original steps draw the initial score, seed the random generator, and
  activate the worker/timing loop. Recover lifecycle ordering and worker message
  behavior before allowing initialization to remain successful.

## XTET score rendering and worker lifecycle

- `DrawScore` caps the value at 9999 and renders exactly four digits from
  `digit.bmp`. The atlas is divided by integer arithmetic into four columns and
  ten rows; XTET uses the first column, selects a row by digit, and draws at
  `(359, 438)` while advancing by one-quarter atlas width.
- `render_score` now reproduces this into the indexed framebuffer. DLL initial
  rendering includes score zero. Tests independently compose the four zero
  glyphs, verify the 9999 cap, and reject an invalid atlas.
- `InitializeWorkerSynchronization` creates an auto-reset event, initializes the
  critical section, and starts `GameWorkerThread` before resource loading. The
  worker waits using the level interval but remains gated until initialization
  sets the run flag at the very end.
- Worker intervals for levels 1..10 are exactly 300, 200, 150, 135, 125, 117,
  110, 105, 100, and 90 ms; other values use 500 ms. `get_game_tick_interval`
  and tests preserve this table.
- Once activated, the worker locks the game critical section, calls `rand`, and
  immediately dispatches `UpdateGameTick`, which enters unrecovered falling-
  figurine spawn/movement logic. Initialization must therefore remain gated
  until those initial runtime objects and state transitions are reconstructed.
- The original cleanup uses `TerminateThread` while holding the critical section,
  then closes event/thread handles. The portable implementation must eventually
  use cooperative stop plus join; the unsafe original mechanism is documented,
  not copied.
- Ghidra now defines/names `GameWorkerThread` and names/documents
  `InitializeWorkerSynchronization`, `DestroyWorkerSynchronization`,
  `GetGameTickInterval`, `UpdateGameTick`, and `DrawScore`. Win32 Debug build,
  CTest, `format`, and `check-format` pass; no x64 build was run.

## Governing reverse-engineering directive

- Do not guess missing behavior or data. Recover it from `GAG.EXE`,
  `XTETDLL.DLL`, their embedded resources, or controlled runtime evidence.
- In particular, do not substitute plausible palettes, display modes, ABI fields,
  or callback behavior for the original implementation.
- Treat the current halftone palette only as an initialization/diagnostic artifact;
  it is not evidence of original behavior and must not be the final rendering path.

## Current implementation

- `xtet/loader/main.cpp` creates a 640x480 top-down 8-bit DIB and a Win32 message loop.
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
- `xtet/loader/main.cpp` now resolves the loaded DLL's actual filesystem path and requires
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
- `xtet/loader/main.cpp` implements each sound handle with an independent WinMM
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
