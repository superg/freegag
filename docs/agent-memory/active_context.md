# Active context

Last updated: 2026-08-16

## GAG main executable reconstruction

- Re-accepted the five-function runtime-visual lifecycle cluster against raw x86: `ParseRuntimeVisualObject` (`0x00408DD0`), `CreateOrUpdateRuntimeVisualObject` (`0x00409060`), `RemoveRuntimeVisualObject` (`0x004091B0`), `SerializeRuntimeVisualObjects` (`0x00409210`), and `DestroyRuntimeVisualObjects` (`0x004092E0`). Existing coverage already proved equal-file dirty clearing, PRIMARY exclusivity, palette accumulation/inversion, POS updates, exact serialization, unlink/free results, and complete destruction. New tests prove missing-name no-op, 0x164-byte zero-allocation failure without list mutation, tail insertion and self identity, changed-file current-to-previous scene transfer, and subsequent no-FILE clearing without overwriting an already saved previous scene. Raw control flow confirms that the image-flag result and second POS integer remain the loop-control value, including `0xffffffff` termination after mutation. Ghidra names/prototypes/comments are synchronized and saved. Full Win32 Debug build, CTest (2/2; 0.22 seconds), `check-format`, and tab checks pass. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- Accepted the typed-value/serialization cluster: `ParseScriptTypedValue` (`0x00408AA0`), `AppendNaturalMouseImageFlag` (`0x00408B20`), `SerializeScriptObjectStates` (`0x00408B80`), and `DestroyScriptObjectStates` (`0x00408D80`). The typed parser now has a production-default observation seam for all three original callees; tests prove integer/image/string fallback order, cursor restoration before each fallback, final string-failure cursor retention, and the unusual treatment of any nonzero image-parser result (including `0xffffffff`) as type 1. Serialization coverage now includes exact `/F:NATURALMOUSE ` emission for flag `0x10000`; destruction and list-head clearing remain covered. Ghidra comments are synchronized and saved. Full Win32 Debug build, CTest (2/2; 0.19 seconds), `check-format`, and tab checks pass. The previous startup-test hang/runtime-check failure remains fixed. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- `ResolveStateFieldReference` (`0x00408660`) completed its branch audit against raw assembly. Existing tests covered object miss, new/existing fields, ON/OFF, positive/zero integers, and empty/nonempty fixed strings; added coverage proves negative integers clear activity, unsupported types and non-ON/OFF boolean values still append a missing field without changing its value/mask, and a 32-field object returns unchanged. Ghidra's comment is synchronized and saved. CTest (2/2; 0.18 seconds) and `check-format` pass. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- Accepted the eight-function script-object lookup/accessor cluster: `FindScriptObjectByName` (`0x00408380`), `FindScriptObjectByIdentity` (`0x00408420`), `QueryOrCreateScriptObjectField` (`0x00408480`), `GetScriptObjectInteger` (`0x004087A0`), `GetScriptObjectString` (`0x00408800`), `AddScriptObjectInteger` (`0x00408870`), `CompareScriptObjectField` (`0x00408900`), and `GetScriptObjectFieldSnapshot` (`0x004089E0`). Source matches Ghidra across primary/container lookup, exact 0x20-byte names/strings, field creation types and masks, active-state polarity, signed integer transitions, deliberate true-on-missing/unsupported comparison, and full 0x68-byte snapshot clearing. Existing branch tests were already extensive; direct primary/contained/name-miss coverage was added. The missing implementation-address comment above `FindScriptObjectByName` was restored. Ghidra comments are synchronized and saved. Full Debug build, CTest (2/2; 0.22 seconds), and `check-format` pass. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- `ParseScriptObjectState` (`0x00407FA0`) completed its branch-fidelity audit. A production-default observation API exposes only its original parser/comparison/visual/creation callees. Deterministic tests now cover missing names, allocation failure, first/tail insertion, existing-object reuse, palette inheritance, positive/zero/negative integers, ON/OFF and ordinary image values, successful/failed string fallback with cursor restoration, existing-field updates, the 32-field cap, command scan/match and zero-definition behavior, MOUSE/AMOUSE targets, natural-mouse visual resolution, and INVERT_NOPAL behavior. The adjacent exact allocator `CreateScriptObjectState` (`0x00408340`) remains covered separately. No production behavior correction was needed. Ghidra is synchronized and saved. Full Win32 Debug build, CTest (2/2; 0.18 seconds), `check-format`, and tab checks pass. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- Began the next fidelity cluster at `ParseScriptObjectState` (`0x00407FA0`). Ghidra confirms the existing 0x3A0-byte body and `uint __fastcall(ScriptParserState *)` prototype. Its adjacent allocator `CreateScriptObjectState` (`0x00408340`) remains exact and already tested for allocation failure, `HEAP_ZERO_MEMORY`, the 0x904-byte size, exact 0x20-byte name copy, and self identity. A production-default `ScriptObjectParseApi` now exposes only the original value/scope/integer/image/comparison/visual/creation callees, preserving production behavior while enabling deterministic branch tests. The existing real-script test still passes. This function is not yet accepted: exhaustive field-type, existing/new field, 32-field cap, scope, command, mouse, palette, and visual-resolution branches remain the active test work.

- `DispatchRuntimeTreeParser` (`0x004056C0`) completed its full branch-fidelity audit. Raw assembly corrected material prior-source errors: `0x0B` publishes operation 4 initially, operation 1 after image flag 2, and operation 2 after image flag 4; `0x0D` fallback is operation `0x50`; `0x0E` fallback is operation `0x60`; token-read failure, initially empty source, callback-preserved source, successful section creation, integer sentinels in `0x0F/0x80/0xA0`, and `0xC0` name mismatch all continue dispatch rather than terminating. The source/section tests cover read failure, both source emptiness states, callback mutation, flag-disabled fallback, null/non-null jump returns using the pre-property cursor, successful command creation, and exact publication ordering. Class tests cover mismatch, `COMMENT`, and `INVENTORY_PACK`. Ghidra is synchronized and saved. Full Win32 Debug build, CTest (2/2; 0.17 seconds total), `check-format`, and tab checks pass. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- The second `DispatchRuntimeTreeParser` (`0x004056C0`) fidelity tranche covers expression/value property families `0x0B`, `0x0F`, `0x80`, and `0xA0`, plus the matching `0xC0` `COMMENT` class path. The test preserves the unusual `0x0B` flag-2 re-entry and flag-4 next-value parse, verifies the exact root property values/order, the `+0x820` integer write, the 0x20-byte class-token target, and flag `0x800`. Adding this coverage exposed and corrected a test seam where `0xC0` still bypassed the production-default value-token callback. Ghidra is synchronized and saved. Full Debug build, CTest (2/2; startup test 0.02 seconds), and `check-format` pass. Remaining dispatcher work is the `0x0D/0x0E` source/section jump family, sentinel exits, and `0xC0` mismatch/`INVENTORY_PACK` variants. Statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**.

- `DispatchRuntimeTreeParser` (`0x004056C0`) now has a deterministic direct-routing harness. A production-default injected call table exposes only the original direct parser/finalization callees without changing their arguments or ordering. Tests cover property codes `0x01..0x0A`, `0x0C`, `0x10..0x70`, `0x90`, `0xB0`, and `0xD0..0xF0`; the `0x60/0x70` owner-flag transitions; terminal `0x2000/0x4000` callbacks; `0x400` auxiliary-name publication; first flagged visual selection; exact fixed-buffer capacities; and final global-link publication. The expression/source/jump families `0x0B`, `0x0D..0x0F`, `0x80`, `0xA0`, and `0xC0` remain explicitly unresolved as the next dispatcher tranche. Ghidra's function comment records this boundary and `/GAG.EXE` is saved. Win32 Debug build, CTest (2/2; startup test 0.04 seconds), `check-format`, and tab checks pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; this remains address coverage rather than completed fidelity verification.

- Corrected `DestroyRuntimeTreeNode` (`0x00405E50`) root unlink storage. Ghidra raw/decompiled control flow writes `ScriptRuntimeRoot::runtime_tree` at `+0xF78` when the removed node has no parent or predecessor; source incorrectly wrote the distinct named-node list at `+0xF84`. The implementation now advances `runtime_tree` and leaves `runtime_nodes` untouched. The focused root-removal test seeds both fields and proves the exact mutation/preservation. The root's `+0x814/+0x818` callbacks are now typed fastcall property setter/getter callbacks in source and Ghidra; Ghidra also has the corrected runtime-tree field type and detailed unlink/callback comment. Program saved; Win32 Debug build, CTest (2/2), and `check-format` pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; fidelity re-verification remains ongoing.

- A major embedded-root initializer offset bug is fixed in `InitializeGraphicsHost` (`0x0041FA00`). Raw Ghidra addresses prove `SetRuntimeScriptProperty` and `GetRuntimeScriptProperty` are stored at absolute `0x00480124/0x00480128`, which are `ScriptRuntimeRoot +0x814/+0x818`, not root `+0x04/+0x08`. Source had been overwriting the independent runtime flags and palette flags; it now initializes the confirmed `set_property`/`get_property` callback fields and leaves both flag DWORDs zero after the whole-allocation clear. Root `+0` is now the confirmed `self` pointer rather than an artificial 0x210-byte storage alias. All call sites and the Ghidra structure use the corrected semantic names/types. Initialization tests verify the exact self/root identity, zero flags, and both callback addresses. Ghidra's initializer comment and structure are synchronized and saved. Win32 Debug build, CTest (2/2), and `check-format` pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; fidelity re-verification remains ongoing.

- Pointer-region/link-84 tail aliasing is now explicit. Ghidra address `0x004808A4` is graphics-host `+0x1944` and embedded `ScriptRuntimeRoot +0xF94`; it is simultaneously the pointer-region list head and `global_link_0084_head`. Source now derives `runtime_pointer_regions` from that semantic root field instead of an independent raw-offset pointer, and likewise derives the scene-slot view from command definition 0's visual field rather than a separate `+0x1444` pointer. Compile-time assertions prove this offset, the command-definition/scene-slot phase mapping, the slot-31 name/global-tail mapping at `+0x1924`, and the serialized-script/palette boundary at `+0x1978`. Bidirectional tests write through both the pointer-region and embedded-root views. `UpdateRuntimePointerRegion` (`0x00423FA0`) documents the shared list in Ghidra and the program is saved. Win32 Debug startup tests, CTest (2/2), and `check-format` pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; fidelity re-verification remains ongoing.

- The command-definition/scene-slot production overlay is now explicitly verified for `ParseRuntimeCommandDefinition` (`0x00409370`) and `ClearRuntimeCommandDefinitions` (`0x004095E0`). The exact 0x141-DWORD clear at embedded root `+0xA70` is graphics-host `+0x1420..+0x1924`, crossing scene slots `+0x1444..+0x1944`, clearing slots 0..30 and the first eight bytes of slot 31 while preserving the final 0x20 bytes. The 0x28-byte definition table at allocation `+0x1424` is phase-shifted by 0x20 against the 0x28-byte slot table: definition N's visual/flags map slot N's visual/metadata, while definition N+1's name maps slot N's name. Embedded-root tests prove the clear boundaries and parse two real definitions to observe the visual pointer, `0x20` slot flag, and following name through the slot view. Ghidra comments on both functions record the physical mapping and the program is saved. Win32 Debug build, CTest (2/2), and `check-format` pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; fidelity re-verification remains ongoing.

- `ResetRuntimeSession` (`0x004263A0`) shared-storage fidelity correction completed. Raw Ghidra operands prove its final `REP STOSD` clears exactly 0x1D3 DWORDs beginning at absolute `0x00480138`, which is graphics-host allocation offset `+0x11D8`, not a detached session array. Source now aliases that range directly. The clear ends at `+0x1924`, so it overlaps the scene-slot array at `+0x1444..+0x1944`: all of slots 0..30 and the first eight bytes of slot 31 are cleared, while slot 31's final 0x20 bytes remain untouched. The preceding two DWORD clears at allocation `+0x9B4/+0x9B8` are simultaneously pointer-event body DWORDs 14/15 and the embedded `ScriptRuntimeRoot::flags/palette_flags`; source now expresses the semantic root fields and cross-view tests prove the exact aliases. Boundary tests lock down both overlap regions. Ghidra names the range `g_dwRuntimeSessionResetRange`, documents both overlaps on the function, and is saved. Win32 Debug build, CTest (2/2), and `check-format` pass. Address statistics remain **559/559 game functions represented; 136/136 library/runtime functions delegated; 0 recognized game functions absent**; fidelity re-verification remains ongoing.

- Function-accounting reconciliation completed against Ghidra's current `/GAG.EXE` function map. Ghidra contains 698 non-external function records: 695 internal bodies and three import thunks. Exactly 559 internal addresses have `// GAG.EXE: 0xXXXXXXXX` source implementations. The remaining 136 internal bodies are all positively classified library/runtime code: 38 bundled gzip/DEFLATE routines below `0x0042BA00` and 98 Microsoft CRT/compiler-runtime routines at or above `0x0042BA10`. There are no recognized game-function addresses absent from `src/`. The previous **560/851, 38 delegated, 253 remaining** statistic mixed an obsolete recognition baseline with incomplete library classification and is retired. Current address coverage is therefore **559/559 game functions represented in source; 136/136 library/runtime functions delegated; 0 recognized game functions absent**. This is an address-coverage statistic, not a claim that every implementation has completed fidelity re-verification; ongoing work is now correction, structure/alias recovery, branch-test expansion, and Ghidra synchronization across already represented functions.

- A range-wide Ghidra inventory of `0x0047EF60..0x00480CDB` reconciled the remaining named detached storage inside the graphics-host allocation. Newly joined source state is archive-state byte `+0x0c`, 260-byte resource directory `+0x110`, two 260-byte pending paths `+0x24c/+0x350`, streamed/runtime resource count `+0x924`, 32-entry `RuntimeSceneSlot` array `+0x1444`, pointer-region list head `+0x1944`, and 257-entry transition palette `+0x1978`. The palette occupies 0x404 bytes and ends exactly at the allocation boundary `+0x1d7c`. Fixed arrays outside the core `RuntimeCommandLoopState` view use direct allocation pointers and explicit byte counts; binary-facing in-structure arrays use direct members. Cross-view tests verify every new mapping. Ghidra fields/data/function comments are synchronized and all gates pass. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- Scene/pointer shared-state recovery now covers the tail immediately before the pointer-event record. Ghidra global audits prove the saved default-comment scene at `RuntimeCommandLoopState +0x954` (`0x0047F8B4`), current special runtime resource at `+0x960` (`0x0047F8C0`), pointer-tree root identity at `+0x964` (`0x0047F8C4`), and active pointer region at `+0x96c` (`0x0047F8CC`). Source detached globals now alias those exact fields; the structure is extended to 0x970 bytes, ending exactly where pointer-event state begins. The external pointer-region list remains correctly separate at `0x004808A4`. Cross-view tests cover resource destruction, pointer-root setup, active-region setup, and saved-comment setup. Ghidra fields/comments are synchronized and all gates pass. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- Runtime game-DLL and property state are now joined to the graphics-host allocation. Ghidra proves `LoadAndInitializeRuntimeGameDll` (`0x00426110`) stores the module and ordinal 1/2/3 pointers at `RuntimeCommandLoopState +0x524/+0x528/+0x52c/+0x530`; `RuntimeGameWindowProcedure` (`0x004231E0`) uses the parent window at `+0`, DLL window procedure at `+0x52c`, result type at `+0x534`, and 0x104-byte result buffer at `+0x538`. `SetRuntimeScriptProperty` (`0x004202D0`) uses nested counters at `+0x91c` and `+0x920`. Detached source globals were replaced by exact shared fields, with direct array access used for the binary-facing result buffer. Cross-view tests verify loader/window/property writes through the complete state object. Ghidra fields/comments are synchronized and all gates pass. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- The allocation-base follow-up merged two more artificial source splits. `runtime_scene_control_flags` and `graphics_host_flags` are both views of `RuntimeCommandLoopState.flags` at allocation offset `+0x930`; `runtime_pointer_x/y` are the scene-coordinate fields at `+0x93c/+0x940`. This matches `InitializeGraphicsHost`, which clears the whole allocation and later writes cursor coordinates directly to `0x0047F89C/0x0047F8A0`; redundant post-clear scalar stores were removed. All tests pass with the shared fields. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- Graphics-host allocation-base correction is complete. Raw `InitializeGraphicsHost` (`0x0041FA00`) loads EDI with `0x0047EF60`, ECX with `0x75f`, and executes `REP STOSD`, proving one 0x1d7c-byte allocation whose `RuntimeCommandLoopState` view begins at offset zero. `RuntimeGameHostContext` is the distinct loader-facing subview at `+0x458`, the 35-entry callback table begins at `+0x498`, and the self-pointing `ScriptRuntimeRoot` begins at `+0x9b0`. Parent window `+0` and child/capture window `+0x458` are therefore distinct. Source globals for archive/async host, cache/media parents, heap, script thread, target flags, host mode, callbacks, and script root now alias their exact locations in that allocation. Initialization and successful shutdown each perform only the original single whole-allocation clear; redundant synthetic clears were removed. Tests were corrected for shared fixture ordering and parent/child distinction and pass. Ghidra fields and comments are synchronized. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- Runtime graphics bootstrap/shutdown storage is now fully joined to `g_RuntimeDisplayContext`. Ghidra proves `InitializeRuntimeGraphics` (`0x0041FEA0`) and `ShutdownRuntimeDisplay` (`0x00420130`) use scene host `+0x214`, the eight-DWORD pixel-format descriptor `+0x228`, root scene identifier `+0x248`, a ten-DWORD backend overlay beginning at `+0x458`, surface `+0x47C`, callback positions `+0x480/+0x484/+0x488`, palette entries `+0x48C`, and script thread `+0x8FC`. Source formerly maintained detached host/scene/thread/backend/pixel arrays; all now alias the original graphics-host allocation. The backend shutdown clear intentionally crosses the 0x20-byte command-target field, width/height, and surface because the original uses a ten-DWORD overlay. Structure assertions and a cross-view shutdown fixture verify the shared representation. Ghidra structures/comments are synchronized. Statistics remain **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- The second `g_RuntimeDisplayContext` alias tranche now covers scene and reset state. `SwitchRuntimeScene` (`0x004242C0`) proves scene X/Y at `+0x93C/+0x940`, deferred identity at `+0x958`, and current identity at `+0x95C`; all former detached source globals now alias those fields. `ResetRuntimeDisplayState` (`0x004262B0`) proves its three scalar clears target input-scene `+0x698`, `input_text[0]` at `+0x63C`, and auxiliary DWORD `+0x954`, then clears exactly 21 DWORDs beginning at current identity `+0x95C`. That range crosses the nominal command-state tail and includes the complete 16-DWORD pointer-event record beginning at `0x0047F8D0` (`+0x970`), so pointer-event state/body now also use the shared graphics-host allocation. Ghidra structure fields and function documentation are synchronized; tests cover the cross-view clears and were corrected so whole-state fixtures initialize the newly confirmed scene fields rather than relying on stale detached globals. This fidelity correction changes no address count: **560/851 functions implemented**, 38 library functions delegated, 253 recognized functions remaining.

- Graphics/runtime storage alias audit corrected a major production-state split. `g_RuntimeDisplayContext` at `0x0047EF60` is the base of the graphics-host allocation, while `RuntimeGameHostContext` is a separate subview at `+0x458`. Source models both over one backing allocation. The parent window at base `+0` is consumed by synchronized archive wrappers; runtime-graphics initialization writes the child window to the loader-facing context at `+0x458`. Runtime flags at `+0x930` and command-pending state at `+0x928` are no longer detached globals. The initialized message-slot block at `0x0043F178..0x004408D3` is represented at its exact 0x175c-byte extent (`"GAG"` followed by zero fill). This entry supersedes the earlier, incorrect interpretation that the two window fields alias.

- Startup/platform production seam audit completed. The byte, pair, and message queues now use their confirmed embedded storage and critical sections in `RuntimeCommandLoopState`; the five contiguous critical sections at `+0x880`, `+0x898`, `+0x8b0`, `+0x8c8`, and `+0x8e0` are respectively the byte queue, pair queue, message queue, synchronized resource/dialog, and path/resource-record locks. Graphics-host initialization and shutdown operate on those exact objects. Display palette operations use the separate display-host lock and recovered surface operation. CDF alternate streams now bind to the recovered async-file subsystem and the compressed-entry reader binds directly to `ReadCompressedCdfEntry`. The only remaining CDF production callbacks are explicitly labeled bundled-library integration boundaries: `DecompressCdfBuffer` (`0x0040F8D0`, stored-copy/raw-DEFLATE) and the gzip writer (`0x00418E90`). No extractor-derived behavior was used. Ghidra's `RuntimeCommandLoopState` fields are synchronized through the queue/lock region and the program is saved. This correction changes no address count: recovery remains **560/851 functions**, with 38 library functions delegated and 253 recognized functions remaining.

- CDF compressed writing now implements `WriteCompressedCdfIndex` (`0x00429070`) and `WriteCompressedCdfEntry` (`0x00429B50`). Both preserve the 0x8000-byte independent-block container, cumulative DWORD offset table, 0x10000-byte temporary buffer, exact seek/write ordering, short-write exits, and `error` transitions `0x20000 -> 2 -> 0`. Index writing compresses `entry_count * 0x2c` bytes from `entries[0]` and accumulates compressed sizes into `index_size`; entry writing uses the selected entry offset/size and deliberately frees both allocations with flags 1. Raw EAX use corrected delegated gzip wrapper `0x00418E90` from `void` to a four-argument fastcall returning compressed byte count; core `0x00404860` is tagged as bundled gzip library code. Injected tests cover multi-block boundaries, exact source stepping/table values/seeks, short writes, null data, allocation failure, state, and cleanup flags. Ghidra is synchronized and saved. Recovery is now **560/851 functions**, with 38 library functions delegated and 253 recognized functions remaining. The production gzip callback remains an explicit library-integration boundary; no compressor behavior was guessed or copied from `tools/cdf_extractor`.

- `ExecuteScriptCommands` (`0x00421530`) is now implemented and bound as the display bootstrap thread entry, replacing the zero-return unresolved seam. The complete dispatcher covers all recognized top-level opcode families, including shared exit `0xB0000000/0x70000000` and load/switch `0x40000000..1/0x50000000..1` control flow. The outer scheduler preserves GDI batch setup, inactive servicing, active-tree resolution, plan synchronization, pending-switch short-circuiting, event acknowledgement, per-link service ordering, activation, retry/commit/finish/restart cursor dispositions, script-clock accumulation, bounded-random refresh, and ten-millisecond sleeps. `RuntimeCommandLoopState +0x960` is confirmed as the loading-scene gate and synchronized in Ghidra. Injected scheduler tests cover immediate shutdown, inactive-to-active service ordering, missing-tree timing, pending-switch bypass, and pause/GOTO/finish/restart cursor handling. Raw assembly proves the SWRAND stack value is uninitialized before its first possible use; source currently uses an explicitly labeled non-original zero guard to prevent MSVC Run-Time Check Failure #3, then follows the original assignment on subsequent outer passes. The implementation count is now **558/851 functions**, with 37 library functions delegated and 256 recognized functions remaining; transition-family deep branch coverage remains the immediate test task before closing this tranche.

- The factored `ExecuteScriptCommands` dispatcher now covers 39 opcode values after adding pointer-zone movement `0xE0000000`, primary-resource movement `0x90000`, and bitmap copy `0x9000`. MOVZ/MOVI preserve absolute versus timed-path forms, target-local retry bits, unsigned deadline pauses, link-8C bounds, per-step timer updates, and pointer rebuild/update ordering. COPY preserves destination resolution through primary resources, scenes, or `BACKGND`, source rectangle defaults, resource locking, and the original scene-source lookup quirk that reuses the destination name. Focused tests cover empty operands and the missing-path bit-2 behavior. Win32 Debug and CTest pass. `0x00421530` remains unbound and uncounted; statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- The factored `ExecuteScriptCommands` dispatcher now covers 36 opcode values after adding global-parent load `0x7000`, current-tree child load `0x8000`, parser-owner load `0x60000000`, label jump `0xB0000`, and session-stop/restart `0xD0000`. Load commands preserve the one-token resource/name substitution from the embedded parser resource, parent selectors, conditional rebuild, and unconditional pointer refresh. Label jumps now return a distinct `commit_cursor` disposition so the outer executor retains the sought cursor rather than restoring the pre-opcode position. STOP performs reset, sends `0x7FFD/0x10000000`, clears state bit `0x100000`, and requests an outer restart. Missing-operand tests cover the load and label paths; all gates pass. `0x00421530` remains unbound and uncounted; statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- The factored `ExecuteScriptCommands` dispatcher now covers 31 opcode values after implementing the shared conditional scanner entered by `0x4000`, `0x40000`, and `0x50000`. It preserves SWVALUE's two-name parse, typed comparison, nested boundary scanning, inclusive RAND range comparison against the executor-supplied random stack value, COND container matching, `CSEND` termination, and EOF behavior. The helper now accepts the random value explicitly so the eventual outer executor can preserve the original stack-slot input without inventing initialization. Tests exercise all three entry opcodes and confirm parser-cursor advancement through `CSEND`. All gates pass. `0x00421530` remains unbound and uncounted; statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- The factored `ExecuteScriptCommands` dispatcher now covers 28 opcode values after adding resource wait `0xA0000000`, inverse/resource-end wait `0xF0000000`, and GAME `0xC000`. Resource waits preserve their asymmetric missing-tree polarity and exact playback/frame flag tests. GAME pauses under flag `0x10`, pauses after a successful initial DLL load, then revisits the opcode under result flag `0x20`, clears that flag, resolves the returned typed value, and zeroes the 260-byte result block/type. Focused tests cover missing operands, opposite missing-tree outcomes, both GAME gates, and result-flag clearing. Build, CTest, format, and whitespace gates pass. `0x00421530` remains unbound and uncounted; statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- The factored `ExecuteScriptCommands` dispatcher slice now covers 25 opcode values. Newly added exact branches are text input `0x200`, private-window command `0x300`, scene-region update `0x400`, sound fade `0x600`, control-boundary scan `0x60000`, and timed wait `0xF0000`. The implementation preserves the input command's duplicate second-name parse, synchronous private-message wait/restart result, BACKGND defaults, sound-record type gate, and timed wait's pause-on-create/pause-before-deadline/complete-at-equality behavior. The dispatcher now distinguishes `restart_outer` from `pause`. Focused tests cover missing-operand cutoffs, nested-state behavior, unhandled opcodes, and every timed-wait boundary. All Win32 Debug build, CTest, format, and whitespace gates pass. `0x00421530` remains uncounted until the remaining complex opcode families and scheduler are integrated; statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- `ExecuteScriptCommands` recovery now has a compiled/tested first dispatcher slice covering opcode values `0x90000000`, `0x80000000`, `0xD0000000`, `0xC0000000`, `1`, `0x100`, `0x500`, `0x1000`, `0x3000`, `0xA000`, `0xB000`, `0xD000`, `0xE000`, `0xF000`, `0x10000`, `0x20000`, `0x30000`, `0xC0000`, and `0xE0000`. It preserves parsing cutoffs, backend-child creation/wait dispositions, resource/list/pointer side effects, scene update bracketing, and nested-state counter transitions. This is factored as a clearly non-original dispatcher helper until all shared-label opcode families and the outer scheduler are complete, so `0x00421530` is not yet counted or bound. Link-7C fields at `+0x60/+0x64/+0x68/+0x6c` are confirmed as backend child, fixed identity, secondary identity, and wait deadline in source and Ghidra. Win32 Debug build, CTest, format, and whitespace gates pass. Statistics remain **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining.

- An address-level comparison of all Ghidra functions against `// GAG.EXE:` source annotations shows that, after excluding bundled compression and Microsoft runtime/library routines, `ExecuteScriptCommands` (`0x00421530`) is the only recognized game function still absent from `src/`. The audit found `FUN_00429EC0` was a false split inside already-recovered `CreateRuntimeAnimationBackend` (`0x00429EB0`): it had no callers/references, consumed flags/registers from the four-instruction prefix, used its 0x88-byte stack frame, and shared its epilogue. Ghidra now contains one repaired 761-byte function body with the confirmed fastcall prototype and retained documentation. The recognized denominator is therefore **851**, and statistics are now **557/851 functions**, with 37 library functions delegated and 257 recognized functions remaining. `ExecuteScriptCommands` remains the active implementation target.

- `ExecuteScriptCommands` (`0x00421530`) remains the active recovery target. Its complete 1,078-line decompile and raw entry/exit control flow have been captured. Direct operands extend `RuntimeCommandLoopState` through `0x96c`: GAME result type/data at `+0x534/+0x538`, nested runtime-state count at `+0x91c`, script clock at `+0x938`, runtime-tree identity at `+0x964`, and active link-7C pointer at `+0x968`. Re-decompilation now uses these fields instead of false `state[1]` aliases. Raw caller/callee auditing also corrected already-counted `ActivateRuntimeTreeLink7C` (`0x0040C4B0`) from `void` to `uint32_t`: it returns zero for null root/link and failed matching, and one for an already-active or newly matched link; the command executor branches on this EAX value. Source tests cover all return paths, Ghidra prototype/comment/structure are synchronized and saved, and Win32 Debug build plus CTest pass. Statistics remain **557/852 functions**, with 37 library functions delegated and 258 recognized functions remaining.

- Post-type re-decompilation corrected the previously accepted `InitializeRuntimeInputSession` (`0x00420790`) and `CopyRuntimeInputSessionRecord` (`0x004208E0`): their globals at `0x0047F59C..0x0047F60C` are aliases inside `g_RuntimeDisplayContext`, not a separate abstract input-object subsystem. Initialization now exactly seeds the `'-'` text buffer, cursor, input flags, end/default 0x20, and caret tick; acquires the selector `RuntimeLockRecord`; initializes standalone text; chooses a free `0x80000` scene slot; selects record scene `+0x1c` or the display-context alternate scene at `+0x248` under flag `0x04000000`; locks, creates, draws, unlocks, releases, and sets runtime flag `0x100`. The original null-record dereference remains possible only after successful text initialization. Copy now moves the exact 32-byte text buffer, returns the cursor, and clears only the cursor. Obsolete placeholder APIs/globals were removed, `RuntimeLockRecord` is confirmed through `+0x1c` at size `0x20`, and tests now cover the exact real branches/order. This is a fidelity correction to already-counted functions, so statistics remain **557/852 functions**, with 37 library functions delegated and 258 recognized functions remaining. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass; Ghidra structures/comments and saved program are synchronized.

- Runtime command-thread text input now implements `ProcessRuntimeTextInput` (`0x00420E10`). It preserves the flag-`0x100` gate, one-byte dequeue, full-buffer/CR completion and scene release, cursor-zero backspace asymmetry, signed-byte `> '@'` case conversion under modes `0x10/0x20`, strict unsigned 250/500 ms caret thresholds, and the exact standalone-text initialization plus display-scene acquire/begin/draw/end sequence. `RuntimeCommandLoopState` now exposes the confirmed input buffer, embedded standalone-text state, scene identifier, text flags, scene index, caret tick, cursor, and end fields from `+0x63c` through `+0x6ac`. Focused tests cover every branch, threshold equality/crossing, signed high bytes, failure cutoffs, arguments, and call ordering. Raw epilogue auditing also corrected `ExecuteScriptCommands` (`0x00421530`) to its `DWORD WINAPI(RuntimeCommandLoopState *)` thread-entry prototype in Ghidra; its 1029-line opcode body remains the active recovery target. Ghidra name/prototype/fields/comment and saved program are synchronized. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass. Recovery is now **557/852 functions**, with 37 library functions delegated and 258 recognized functions remaining.

- Runtime-session teardown now implements `ResetRuntimeSession` (`0x004263A0`) and replaces the command-loop terminal seam. It preserves DLL shutdown, repeated root resource destruction/deactivation, display reset, visual/fixed-resource retirement, exact global-list cleanup ordering, conditional archive close, unconditional async-host destruction, full-surface operation, both named-list status checks, pointer-event/session-storage clearing, and restoration of reset values `6/5/5` plus host mode `0x6a4`. The apparently reversed timeout branch is confirmed from raw assembly: a nonzero status sleeps only when the current tick is at or beyond `start + 5000`; an earlier tick exits immediately. `RuntimeCommandLoopState` now exposes confirmed archive/async-host pointers and reset fields, and `GetOrCreateRuntimeNamedNode` has its corrected typed return throughout production/test APIs. Focused tests cover full cleanup/order, archive-flag behavior, both timeout outcomes, exact surface arguments, storage clearing, and state restoration. Ghidra name/prototype/fields/comment and saved program are synchronized. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass. Recovery is now **556/852 functions**, with 37 library functions delegated and 259 recognized functions remaining.

- Complete current-state serialization now implements `SerializeCurrentRuntimeState` (`0x00404990`) and replaces the final unresolved snapshot dependency used by the primary window procedure. Ghidra incorrectly split the prologue at `0x0040499E`; raw control flow proves one body through `0x00404ED9`, so both false bodies were deleted and recreated as one 0x54a-byte function. The implementation preserves retained-buffer creation/clearing, resource-operation queries 8/12/4/1/2/5, exact global/object/tree serializer ordering, optional fields, try/catch output, tail-tree `e_START` event generation, and null/allocation failures. Raw helper auditing also corrected `BeginScriptTextDocument` (`0x0040D0F0`) and `EndScriptTextDocument` (`0x0040D140`) to their actual one-argument fastcall prototypes and embedded `[CFG]`/`[END]` strings; the former EDX inputs were decompiler phantoms. Byte-exact minimal output and complete optional/event-path tests pass. Production seams in the capture window, state activation, save/open dialogs, state transition, and comment-tree rebuild now call the already recovered functions proven by each caller's Ghidra call graph. `ProcessStateActivation` now passes the scene identity at game-context `+0x960` to `QueryRuntimeSceneFlags`, correcting its former zero-argument seam, and its test verifies the exact identity. Ghidra prototypes/comments and saved program are synchronized. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass. Recovery is now **555/852 functions**, with 37 library functions delegated and 260 recognized functions remaining.

- The primary application window procedure `GagMainWindowProcedure` (`0x0041D560`) is accepted and wired as the default primary class callback. It preserves the recovered create/destroy/close/activation/window-position/cursor/system-command paths, keyboard and `0x30F..0x311` forwarding, menu commands `0x8780..0x8910`, private `0x7FFD` state queries, snapshot retention/release, deferred open/save/screenshot/display operations, restart, and shutdown. Raw operands confirmed the embedded path strings and copies: `NEWGAME`, `START.CFG`, `Credits.cfg`, `CREDITS`, and the shutdown append of `CREDITS`. `ApplicationStateFieldQuery` is a distinct 0x40-byte pair of 32-byte names used by query commands; it must not be conflated with the lifecycle `StateFieldReference`. Focused tests cover creation/destruction, suppression/default forwarding, keyboard and private-message forwarding, screen-saver suppression, boolean-query polarity and gates, fresh/retained snapshot capture and release, duplicate-close suppression, system-menu flag changes, and exact hook/lock/validation/path/runtime-flag ordering for new-game, restart, and credits commands. Ghidra has the corrected stdcall prototype, detailed comment, query structure, and saved program. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass. Recovery is now **554/852 functions**, with 37 library functions delegated and 261 recognized functions remaining.

- Multi-format runtime resource construction now implements `ConstructRuntimeResourceObject` (`0x00424EC0`). Raw prologue and `RET 0x18` auditing confirms the eight-argument fastcall ABI: ECX path, EDX scene identifier, followed by x, y, width, height, scale-or-loop, and flags; there is no caller-frame or saved-register argument. The implementation preserves scene/flag normalization and the bitmap, sound, animation, tree/script, generic-data, and CDF branches, including format-specific allocation, scaling, looping, backend configuration, registration, visibility callbacks, and failure cleanup. Focused injected tests cover all resource families, normal and half-size animation scaling, finite/infinite sound and animation looping, primary/ordinary configuration, memory/stream release selection, allocation/backend/scene/archive failures, tree activation suppression, generic fallthrough, registration, and callback creation. Ghidra has the corrected name, prototype, plate comment, callers/callees, and saved program. Win32 Debug build, both CTest tests, `check-format`, and whitespace checks pass. Recovery is now **553/853 functions**, with 37 library functions delegated and 263 recognized functions remaining.

- Generic-backend child attachment now implements `AttachRuntimeGenericBackendChild` (`0x00425D50`) and replaces the final stub used by full runtime-tree resource rebuilding. The five-argument fastcall acquires main/secondary/fixed resources, inherits missing linked identities from main `+0x68/+0x64`, creates the backend child with the exact two-DWORD owner/scene context, selects the fixed resource's scene or the global display scene for backend flag `0x04000000`, creates the 16x16 display scene, publishes linked identities/type flag `0x01000000`/child at main `+0x64/+0x68/+0x08/+0x74`, and releases records in secondary/fixed/main order. It deliberately returns the created child pointer even when a later display failure destroys that child. Tests cover missing acquired records, child failure, lock failure, scene failure, explicit and inherited links, successful publication, global-scene selection, owner selection without a main resource, and cleanup ordering. `RuntimeResourceObject` has the confirmed linked identities and child fields; Ghidra name/prototype/structure/comment are synchronized and saved. Recovery is now **552/853 functions**, with 37 library functions delegated and 264 recognized functions remaining.

- Runtime-tree activation resource rebuilding now implements `RebuildRuntimeTreeResources` (`0x004268B0`) and corrects the previously recovered `ProcessPendingRuntimeTreeSwitch` (`0x004210A0`) against raw assembly. Activation receives two null trailing arguments; matching identities reset parser contexts with the activated node in ECX; rebuilding receives the activated identity on success and the original identity on failure. The rebuild covers scene links, secondary resources, deduplicated pointer owners, the first primary resource, fixed-name resources, visual resources, all active primary resources, backend-child configuration flags, count waiting, root-only queue/transient/resource resets, and the final `0x7FFD/0x40000000` notification. `RuntimeFixedNameListNode` is now confirmed as 0x58 bytes with resource flags/identity/previous identity at `+0x48/+0x4c/+0x50` and next at `+0x54`. Ghidra has the synchronized function, structure, and visual/fixed/primary list globals and is saved. Recovery is now **551/853 functions**, with 37 library functions delegated and 265 recognized functions remaining.

- Runtime-tree top-level parsing now implements `DispatchRuntimeTreeParser` (`0x004056C0`) and `ParseScriptObjectState` (`0x00407FA0`). The dispatcher preserves every recovered property-code branch, root callback ABI and ordering, source/section jump behavior, termination callbacks, primary-visual selection, and global-link publication. Object parsing preserves fixed-width lookup/insertion, the 32-field limit, integer/image-flag/string classification, active masks, command masks, mouse names, natural-mouse resolution, palette inheritance, and inverse-NOPAL behavior. Production creation, section-dispatch, destruction, conditional-reset, and auxiliary-name seams now call their recovered implementations. `ScriptRuntimeRoot` fields at `+0x820`, `+0x824`, `+0x828`, `+0x848`, `+0x868`, and `+0x96c`, plus the script object's alternate visual at `+0x474`, are confirmed in source; the root fields and both function prototypes/comments are synchronized in the saved Ghidra program. Direct parser and real `object=` dispatcher tests pass. Recovery is now **550/853 functions**, with 37 library functions delegated and 266 recognized functions remaining.

- Pointer-owner slot synchronization and resource rebuilding now implement `SynchronizeRuntimePointerOwnerSlots` (`0x00407A80`) and `RebuildRuntimePointerResources` (`0x00426700`). Synchronization walks the configured named-list count from its circular cursor, updates matching link-84 owner/command data, selects ordinary versus natural mouse files, enables populated primary links, and disables exhausted slots. Rebuilding deduplicates adjacent owners, clears link-84 previous-owner state at `+0x60`, retires previous resources, constructs missing active resources, removes inactive resources with exact permanent/immediate behavior, restores positions, balances scene-resource counts, enables comment mode, sends `0x7FFD/0x01000000`, and waits for the target count. `RuntimePointerRegion` is now confirmed as the full 0x68-byte link-84-compatible record through previous primary identity `+0x64`; the Ghidra structure was resized and extended accordingly. Focused tests cover missing owners/roots, populated and exhausted slots, ordinary and natural mouse file selection, adjacent-owner deduplication, every stale/inactive destruction class, resource creation arguments, count balancing, position restoration, and notification ordering. Both production seams now call the recovered rebuild. Ghidra names/prototypes/comments/types are synchronized and saved. Recovery is now **548/853 functions**, with 37 library functions delegated and 268 recognized functions remaining.

- Runtime pointer right-button press now implements `HandleRuntimeRightButtonDown` (`0x00423CA0`) and replaces the final production pointer-button seam. It preserves active/comment/input gates, release-bit clearing, ordinary mode-`0x10000` event construction, the slot-DWORD `0x00200000` command path, state command-mask gating, lazy mouse-visual creation, scene switching, descendant teardown, and mode-`0x30000` flags `3/11/15`, pointer refresh, and temporary-visual cleanup. Tests cover all gates, ordinary and command slots, lazy visual creation with exact constructor arguments, both mode-`0x30000` event variants, and state changes. Ghidra name/prototype/comment are synchronized and saved. Recovery is now **546/853 functions**, with 37 library functions delegated and 270 recognized functions remaining.

- Runtime pointer left-button press now implements `HandleRuntimeLeftButtonDown` (`0x004238B0`) and replaces the production left-down seam. It preserves active/comment/input gates, the distinct script-flag 2/4 derivation, mode-`0x10000` cyclic bit rotation with wrap and 32-attempt limits, state-command-mask priority over slot flag `0x20`, special `IView`/`Hide` event construction, ordinary scene switching, and mode-`0x30000` pointer/temporary-visual cleanup. `RuntimeSceneSlot` now exposes the confirmed 32-byte name at `+0x08`. Tests cover inactive, comment-destruction, comment-display, flag-2/flag-4, ordinary rotation/switch, both special names with and without state objects, exact event records, and mode cleanup. Ghidra's `0x004238BE` entry was proven false by its inherited ZF input and lack of callers, then deleted; the real `0x004238B0` body re-decompiles completely and has the synchronized name/prototype/comment. Recovery is now **545/853 functions**, with 37 library functions delegated and 271 recognized functions remaining.

- Runtime pointer left-button release now implements `HandleRuntimeLeftButtonUp` (`0x00423BC0`) and replaces its production dispatch seam. The handler preserves inactive/input-disabled no-ops, unconditional `0x10000000` release-event initialization on active input, mode-`0x10000` region lookup, current-bit versus selector resolution, scene-slot flag `0x20` rejection, optional state-object publication, exact flag values `0x10000009/0x1000000d`, and unconditional event enqueue after the active gates. Tracing corrected the pointer-event representation: the original event begins at `0x0047F8D0`, with state mask, owner, state object, the 0x40-byte body from `0x0047F8DC`, and flags at overall DWORD 14. A non-original adapter now composes that exact 16-DWORD record; existing pointer-region enqueue tests were corrected from the prior shifted model. Focused tests cover every gate, empty regions, explicit/current scene selection, blocked scene slots, state-object publication, and selector invocation. Ghidra name/prototype/comment are synchronized and saved. Recovery is now **544/854 functions**, with 37 library functions delegated and 273 recognized functions remaining.

- Runtime-tree global publication and script-object serialization now implement `PublishRuntimeTreeGlobalLinks` (`0x00406190`) and `SerializeScriptObjectStates` (`0x00408B80`). Publication preserves all seven link families and its three parent modes: ordinary nested nodes do nothing, sentinel-parent nodes update only non-null global tails, and parentless nodes either establish global heads or append through existing tail selectors without changing the selectors. Object serialization emits the leading blank line once, every object in list order, nonzero integer and nonempty string field values, an unconditional ON/OFF record for every declared field, both mouse names, image/natural-mouse flags, command-mask entries, and exact statement endings. `ScriptObjectState` now has confirmed mouse-name fields at `+0x430/+0x450`, image flags at `+0x478`, and command mask at `+0x47c`. Focused tests cover all publication modes and byte-exact multi-object output. Ghidra names/prototypes/comments are synchronized and saved. Recovery is now **543/854 functions**, with 37 library functions delegated and 274 recognized functions remaining.

- Basic runtime-tree commands and generic-resource bulk cleanup now implement `RemoveAllRuntimeGenericResources` (`0x00405080`), `DispatchRuntimeTreeSectionCommand` (`0x00406A70`), `ParseRuntimeLanguage` (`0x00406B90`), and `CreateRuntimeTreeCommand` (`0x00406C00`). The section dispatcher preserves owner flag clearing, local-resource name reuse, the two distinct parenthesized-text extraction points, explicit-resource load failure, and owner-identical result suppression. Tree creation preserves complete scope consumption, the `0xffffffff` global-parent selector, global-owner rejection, local-resource reuse, and explicit-resource forwarding even when loading returns null. The language field is confirmed as 0x20 bytes at script root `+0x828`. Bulk cleanup saves each next pointer before removal, so active-reference removal failures do not stop traversal. Focused tests cover every branch, and failed requirements now terminate directly after printing their line instead of opening a modal abort dialog. Ghidra names/prototypes/comments are synchronized and saved. Recovery is now **541/854 functions**, with 37 library functions delegated and 276 recognized functions remaining.

- Runtime sound pause/resume control now implements `ToggleRuntimeSoundState` (`0x004010A0`), `PauseRuntimeSoundOutput` (`0x004015D0`), and `ResumeRuntimeSoundOutput` (`0x004016D0`). Toggle bitwise-complements the complete state DWORD. Pause preserves the enabled gate, lifecycle mutex ordering, unconditional suppression, conditional initialized clearing, two-header waveOut teardown, optional thread shutdown, and ordinary common release. Resume clears suppression, recreates the sound window thread and waveOut device only when both initialized/ready are zero, waits for window and callback readiness, cleans up failed opens, and preserves the unusual fault return before mutex release. Focused injected-WinAPI tests cover disabled, pause-only, full teardown, already initialized, thread creation failure, waveOut failure/success, and the early fault exit with exact event order and state publication. Ghidra names/prototypes/comments and saved program are synchronized. Recovery is now **537/854 functions**, with 37 library functions delegated and 280 recognized functions remaining.

- Archive selection/save UI now implements `ArchiveSelectionDialogProcedure` (`0x004188A0`) and `RunArchiveSelectionDialog` (`0x004175F0`). The callback preserves initialization of list/custom/edit controls, all enumeration outcomes, edit selection clearing, initial-value routing, list selection/double-click behavior, exact EN_SETFOCUS/EN_CHANGE matching, replacement confirmation (`SAVE`, flags `0x34`), new-name construction from fixed prefix plus three-digit maximum identifier and extension, optional 100 ms sleep, retained-path cleanup, private-message forwarding, and dialog results 0/1/2/0x10000. The launcher clears both outputs before its registration gate, initializes the shared 0x98-byte state, launches resource 103, and returns the raw dialog result; raw assembly corrected Ghidra's prior void rendering. Focused tests cover the recovered message families, generation/replacement decisions, cleanup, and six-argument fastcall wrapper. Ghidra prototype/comment/post-type caller decompilation are synchronized and saved. Recovery is now **534/854 functions**, with 37 library functions delegated and 283 recognized functions remaining. Win32 Debug CTest passes both tests in 0.22 seconds.

- Archive-comment dialog orchestration now implements `ArchiveCommentDialogProcedure` (`0x00418560`) and `RunArchiveCommentDialog` (`0x00417550`). The callback preserves the exact initialization outcomes from enumeration (normal list setup, no-entry placeholder/custom-control state, and fatal termination), list selection/double-click forwarding, private `0x30f`/`0x311` forwarding, OK/cancel cleanup, focus/show behavior, and dialog return codes. The wrapper preserves the custom-control registration gate, path splitting, resource 101 launch, shared output pointers, and output clearing on nonzero results. Focused tests cover every message family, enumeration result, notification, cleanup path, and wrapper result. A missing real callback boundary was repaired at `0x004188A0`, increasing the recognized denominator to 854; that second callback remains unresolved. Ghidra names/prototypes/comments are synchronized and saved. Recovery is now **532/854 functions**, with 37 library functions delegated and 285 recognized functions remaining. Win32 Debug CTest passes both tests in 0.22 seconds.

- Archive-comment discovery now implements `EnumerateArchiveComments` (`0x004182A0`). The exact 0x98-byte dialog state exposes directory and extension inputs, two output pointers, maximum numeric identifier, count/capacity, and the archive-path array. Enumeration builds `directory + "*" + extension`, allocates ten 0x104-byte path slots, opens each CDF, accepts only readable `COMMENT.TXT` entries smaller than 0x104, sends comments to the listbox, retains archive paths, and grows in ten-slot increments. It preserves signed identifier comparison, unsigned count fallback, nonfatal open skipping, fatal-open deletion/reset, allocation failure, no-entry teardown, and `HeapReAlloc` failure retaining the original allocation/count. Tests cover no matches, initial allocation failure, two-entry success, oversized comments, fatal open, successful growth, and failed growth with exact cleanup counts. Ghidra has the synchronized 0x98-byte structure, name, fastcall prototype, comment, post-type decompile, and saved program. Recovery is now **530/853 functions**, with 37 library functions delegated and 286 recognized functions remaining. Win32 Debug build, both CTest tests, formatting, debug-helper, and whitespace checks pass.

- Runtime-tree command target parsing now implements `ParseRuntimeTreeCommandTarget` (`0x00421440`). It preserves the exact initial flag-probe rejection, cursor restoration only after a zero probe, fixed 0x20-byte implicit-resource name copies, parse-failure flag normalization, and signed-positive trailing-flag rule. Focused injected-parser tests cover initial failure/positive rejection, implicit resource with missing/positive flags, explicit resource/tree with positive trailing flags, and the nonpositive trailing path including exact cursor positions. Ghidra is synchronized and saved. The compressor-only routines at `0x00418E60` and `0x00418E90` are documented and delegated as bundled gzip code. Ghidra's false `0x004204BB` boundary was deleted: it consumed flags and the 0x10c-byte frame created by `GetRuntimeScriptProperty` at `0x004204B0` and had no callers. Corrected recovery statistics are **529/853 functions**, with 37 library functions delegated and 287 recognized functions remaining. Win32 Debug build, both CTest tests, formatting, debug-helper, and whitespace checks pass.

- Archive read-throughput measurement now implements `MeasureArchiveReadSpeed` (`0x00417990`). The recovered fastcall accepts the archive path in ECX and requested byte limit in EDX, initializes async I/O, derives an optional drive root, creates the exact 0x10000-byte host, skips the first 0x8000-byte block, reads the selected remainder in 0x8000-byte chunks, and reports bytes per millisecond only when `timeGetTime` advances. The source preserves the original unsigned `file_size - 0x8000` calculation, signed-positive loop gate, ignored read results, and close-before-host-destroy ordering. Focused tests cover explicit/file-size limits, drive-prefix selection, exact chunking, equal timestamps, host failure, and record failure. Ghidra has the corrected fastcall prototype/comment and is saved. Recovery is now **528/854 functions**, with 35 library functions delegated and 291 recognized functions remaining. The apparent CTest nontermination was an interactive MSVC Run-Time Check #3 dialog: `ParseScriptObjectContainer` directly read its intentionally uninitialized carried scalar. It now obtains the same four raw stack bytes through `memcpy` from the uninitialized character buffer, preserving the assembly behavior without a checked uninitialized-scalar access. Win32 Debug CTest now passes both tests in 0.22 seconds.

- Script-object condition-container parsing now implements `ParseScriptObjectContainer` (`0x0040C570`). It allocates and immediately inserts the exact 0x1b4-byte record, consumes object/field/typed-value triples, preserves existing-object ownership, creates missing object states, records active-field pointers/masks, and implements all four `GLOBAL_SYSTEM_STATE` fields with their distinct required-mask rule. Assembly confirms ordinary type-0/type-3 entries inherit a truth marker initially loaded from uninitialized field-name stack storage; the source retains that native behavior. Focused tests cover system ON/OFF masks, all system fields, owned-object creation, slot wiring, and insertion retained after incomplete input. Ghidra is synchronized and saved. Recovery is now **527/854 functions**, with 35 library functions delegated and 292 recognized functions remaining. The Win32 Debug target builds; the monolithic startup-test executable did not terminate within this tranche's verification timeout, so the full CTest gate remains pending.

- Runtime link-7C interaction matching and activation now implement `MatchRuntimeTreeLink7CInteraction` (`0x0040BF60`) and `ActivateRuntimeTreeLink7C` (`0x0040C4B0`). Raw assembly resolves the matcher's recursive fastcall ABI: it copies the complete 16-DWORD event state, passes that copy in ECX and each alternate link's `+0x74` criteria block in EDX, and rejects NOMATCHES when any compatible alternate recursively succeeds. The matcher preserves exact low/high flag tests, five gated identity/value comparisons, bounded random checks, condition-container state, primary-resource rectangle overlap, NOMATCHES publication, and transparent versus consuming state mutation. Activation preserves null/already-active gates, empty-queue flag clearing, match-driven owner flag `0x80000000`, conditional flagless-event consumption, and 32-slot read-index wrap. Tests cover zero criteria, every gated comparison, high-nibble mismatch and UPDOWN inheritance, random pass/fail, condition pass/fail, primary null/boundary/failure, recursive NOMATCHES rejection and success, transparent retention, null/already-active activation, empty/nonempty queues, failed matches, consumption, and wraparound. Ghidra names/fastcall prototypes/comments and corrected recursive decompilation are synchronized and saved. Recovery is now **526/854 functions**, with 35 library functions delegated and 293 recognized functions remaining.

- Link-7C embedded-script navigation now implements `SeekRuntimeTreeLink7CLabel` (`0x0040C1E0`), `FindRuntimeTreeLink7COpcodeValue` (`0x0040C260`), and `ScanRuntimeTreeLink7CControlBoundary` (`0x0040C2F0`). Label seeking resets to the saved start and deliberately retains the post-label cursor only on a match; misses restore the entry cursor. Opcode/value lookup returns the exact post-value cursor and retains it only when its fourth argument is zero, restoring on requested preservation and every failure. Boundary scanning starts at the current cursor, retains all advancement, tracks nesting only for SWVALUE/SWRAND/SWLOCK (not VALUE), returns outer CSEND unconditionally, and returns outer BREAK only when requested. Tests cover missing identities, label hits/misses, exact cursor positions, opcode/value hits with both cursor policies, value misses/end behavior, nested BREAK suppression, nested and outer CSEND, the VALUE non-nesting distinction, and end-of-script returns. Ghidra names/fastcall prototypes/comments/post-type decompilation are synchronized and saved. Recovery is now **524/854 functions**, with 35 library functions delegated and 295 recognized functions remaining.

- Runtime link-7C interaction parsing now implements `ParseRuntimeTreeLink7C` (`0x0040B850`). The exact 0xb4-byte record contains its fixed name/self/next prefix, a complete 0x28-byte parser snapshot at `+0x38` with both offsets reset to the post-name cursor, owner-derived state at `+0x70`, command/source/destination/zone associations at `+0x74..+0x80`, direct RECT values at `+0x88..+0x94`, primary resource and condition links at `+0x98/+0x9c`, random bounds at `+0xa0/+0xa4`, and accumulated flags at `+0xac`. Parsing preserves every observed UPDOWN, NOMATCHES, R, IMAGE, TRANSPARENT, KEYUP, SOUR, RECT, COMM, DEST, C, and ZONE branch, including missing-name behavior and flag gating. Tests cover empty input, the exact parser snapshot, owner flag propagation, all successful associations and values, the complete combined mask, every lookup-miss path, first/global and local-tail insertion, and zero defaults. Ghidra structure/name/fastcall prototype/comment and post-type decompilation are synchronized and saved. Recovery is now **521/854 functions**, with 35 library functions delegated and 298 recognized functions remaining.

- Runtime link-8C script parsing now implements `ParseRuntimeTreeLink8C` (`0x0040B3E0`). The exact 0x54-byte record now exposes its fixed name/self/next prefix, TIME at `+0x28`, flags at `+0x2c`, direct RECT fields at `+0x30..+0x3c`, and LINE values at `+0x4c/+0x50`. `/RAD` sets flag bit 2 and `/LINE` sets bit 1 even when an integer is absent; RECT does not perform scene-link endpoint conversion. Tests cover missing input, complete TIME/RAD/LINE/RECT parsing, exact fields and flags, first/global insertion, second local-tail insertion, zero defaults, and the unchanged global-tail quirk. Ghidra structure/name/fastcall prototype/comment and post-type decompilation are synchronized and saved. Recovery is now **520/854 functions**, with 35 library functions delegated and 299 recognized functions remaining.

- Runtime link-84 script parsing now implements `ParseRuntimeTreeLink84` (`0x0040AAC0`). It allocates the exact zeroed 0x68-byte record, copies the fixed identifier and self identity, parses `/P`, `/IMAGE`, `/RECT`, `/POS`, `/MOUSE`, `/COMM`, `/PCOMM`, `/OWNER`, and `/C`, and preserves the original ordered insertion behavior (including the fact that parser insertion does not update the runtime-global tail field). RECT stores x/y plus the third/fourth values, while POS stores all four values directly. Command names become indexed mask bits and PCOMM additionally replaces the primary bit. Tests cover missing input, condition rejection and free, all resolved association fields, both geometry modes, first/global and local-tail insertion, command masks, and exact zero defaults. Ghidra now exposes the command count/table at root `+0xa70/+0xa74`, the confirmed link-84 semantic fields, and the synchronized name/fastcall prototype/comment; the program is saved. Recovery is now **519/854 functions**, with 35 library functions delegated and 300 recognized functions remaining.

- Primary-resource parsing and LIST expansion now implement `ParseRuntimeTreePrimaryResourceLink` (`0x00409E50`), `CreateOrUpdateRuntimeTreePrimaryResourceLink` (`0x0040A3C0`), and `CreateOrUpdateRuntimeTreeLink84` (`0x0040AE40`). The 0x8c-byte primary record is confirmed through every field used by this cluster: identifier, file/resource snapshot, flags/image flags, loop/source/position/size/ratio values, secondary/fixed-name links, and prior coordinates. Direct parsing preserves global/default flags, SOURCE lookup, ratio/loop/FILE/POS/image flags, NOPAL inversion, condition rejection, insertion, and auxiliary-file registration. LIST mode finds the exact named list, expands three-digit identifiers, consumes circular children then null slots, creates primary/link-84 pairs with the original coordinate stepping, marks null-resource primaries global, and always frees the template. Tests cover empty input, missing tree helpers, full direct parsing, false condition rejection, missing lists, child-backed and null-child expansion, exact generated names/coordinates/flags/identities, file/resource snapshots, zero-delta snapshots, and existing link-84 propagation. Ghidra primary/link-84 layouts, names, fastcall prototypes, comments, and post-prototype caller decompilation are synchronized. Recovery is now **518/854 functions**, with 35 library functions delegated and 301 recognized functions remaining.

- Runtime-tree scene-link and secondary-resource-link constructors now implement `ParseRuntimeTreeSceneLink` (`0x00409600`) and `ParseRuntimeTreeSecondaryResourceLink` (`0x00409A80`). Scene links are confirmed as 0x44-byte records with name/self identity, Z, x/y, width/height, flags, scene identifier, and next pointer. RECT converts inclusive endpoints to dimensions while POS stores dimensions directly, and only flags 2, 0x20, 0x02000000, and 0x04000000 are retained. Secondary links are confirmed as 0x4c-byte name/self/file/resource/next records; FILE parsing, ordered insertion, and flag-0x400 auxiliary registration are preserved. Tests cover RECT conversion, POS values, accepted/ignored image flags, first and tail insertion, global publication, fixed identifiers, selector-validated FILE parsing, and exact identities. Ghidra structures/names/fastcall prototypes/comments are synchronized and saved. Recovery is now **515/854 functions**, with 35 library functions delegated and 304 recognized functions remaining.

- Runtime visual-object parsing, programmatic mutation, serialization, and image-flag override output now implement `ParseRuntimeVisualObject` (`0x00408DD0`), `CreateOrUpdateRuntimeVisualObject` (`0x00409060`), `SerializeRuntimeVisualObjects` (`0x00409210`), and `SerializeImageFlagOverrides` (`0x0040A9D0`). The visual record is confirmed as 0x164 bytes with fixed names, a 0x104-byte serialized FILE expression, coordinates, current/prior scene identities, ordinary flags, and palette flags. Parsing preserves exact fixed-width lookup, dirty-bit and scene snapshot ordering, PRIMARY exclusivity, NOPAL inversion, and missing-FILE clearing. Serialization preserves scoped FILE tokens, conditional POS output, and palette-default differences, including colonless `/INVERT_NOPAL`. Tests cover byte-exact output, matching and changed files, primary exclusivity, palette accumulation/inversion/default matching, parser selection publication, position changes, and scene clearing. Ghidra structure/name/fastcall prototypes/comments and post-type decompilation are synchronized. Recovery is now **513/854 functions**, with 35 library functions delegated and 306 recognized functions remaining.

- Named-list script parsing now implements `ParseRuntimeNamedNode` (`0x00407490`). It preserves first-token exact node lookup, zeroed 0x50-byte tail allocation and self identity, exact 32-byte script-object lookup, circular-child identity duplicate suppression, unchecked 0x34-byte child allocation, `/F` flag accumulation, and five-value `/ZONE` parsing at offsets `+0x30,+0x34,+0x28,+0x38,+0x3c`. The third ZONE value is uniquely always assigned and converts sentinel `0x7fffffff` to one; other missing values preserve prior fields. All exits return zero. Tests use fully determined 31-character fixed identifiers to avoid masking the original fixed-width comparison, and cover new node/child creation, duplicate suppression, flags, all five ZONE values, partial ZONE sentinel behavior, existing-node reuse, missing initial input, exact allocation/free counts, and cleanup. Ghidra name/fastcall prototype/comment and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **509/854 functions**, with 35 library functions delegated and 310 recognized functions remaining.

- Runtime command-table parsing and serialization now implement `AppendNaturalMouseImageFlag` (`0x00408B20`), `ParseRuntimeCommandDefinition` (`0x00409370`), `AppendDualImageFlag` (`0x004094C0`), `SerializeRuntimeCommandDefinitions` (`0x00409510`), and `ClearRuntimeCommandDefinitions` (`0x004095E0`). The root command region is confirmed as count `+0xa70` followed by 32 entries of 0x28 bytes at `+0xa74`: name `+0x00[32]`, visual pointer `+0x20`, flags `+0x24`. Parsing preserves the `count > 31` capacity check, exact name reuse, `/F` accumulation, `/MOUSE` visual resolution, and the unusual unconditional count increment even when reusing a slot. Serialization retains empty entries created by that quirk. Flag helpers are address-verified as NATURALMOUSE at `0x0043E360` for bit `0x10000` and DUAL at `0x0043E378` for bit `0x200000`. Clearing zeroes exactly 0x504 bytes. Tests cover null/zero/unrecognized helper inputs, both exact flag strings, new and reused definitions, visual lookup, accumulated flags, count-32 rejection, byte-exact two-entry serialization including the empty reused-count slot, and full-region clearing. Ghidra has the command structure plus synchronized names/fastcall prototypes/comments and saved program. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **508/854 functions**, with 35 library functions delegated and 311 recognized functions remaining.

- Runtime named-node membership, navigation, serialization, and pruning now implement `AddScriptObjectToRuntimeNamedNode` (`0x004078D0`), `RemoveScriptObjectFromRuntimeNamedNode` (`0x00407990`), `RotateRuntimeNamedNodeCursorPrevious` (`0x00407C00`), `RotateRuntimeNamedNodeCursorNext` (`0x00407C60`), `ClearRuntimeNamedNodeChildren` (`0x00407CC0`), `SerializeRuntimeNamedNodes` (`0x00407DD0`), and `PurgeDisabledRuntimeNamedNodes` (`0x00407EE0`). Object membership preserves exact 32-byte node matching, zero-terminated object matching, identity duplicate suppression, unchecked zero-allocation, and circular-list ordering. Cursor rotation is gated by child count `+0x40 > +0x28` and follows native previous/next links exactly. Serialization emits circular children plus the five `/ZONE` values in exact offset order. Pruning frees exactly the recorded count of circular children and rebuilds the retained top-level list in order. Tests cover node/object misses, duplicate suppression, add/remove, both rotations and their gate, complete clearing, mixed retained/deleted pruning, null-root pruning, exact free counts, and byte-exact serialization. Ghidra names/fastcall prototypes/comments are synchronized and saved. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **503/854 functions**, with 35 library functions delegated and 316 recognized functions remaining.

- Fixed-name record parsing now implements `ParseScriptFileValue` (`0x0040CDA0`) and `CreateOrUpdateRuntimeFixedNameNode` (`0x00407240`). File values receive the runtime `+0x828` suffix only when unqualified, optionally serialize a cursor-neutral one-token lookahead, and validate explicit integer expressions against runtime `+0x824`. Fixed-name records preserve exact 32-byte matching, 0x58-byte zero-allocation, self identity, tail publication, `/FILE` snapshot/clear ordering, and the split `/F` behavior between record flags `+0x24` and palette flags `+0x48`. Tests cover suffix/no-suffix, serialization, matching/mismatching selectors, creation, in-place update, PRIMARY palette clearing, NOPAL palette setting, exact fields, heap ownership, and restoration of shared test state. Ghidra names/fastcall prototypes/comments are synchronized and saved. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **496/854 functions**, with 35 library functions delegated and 323 recognized functions remaining.

- Runtime-tree auxiliary-name creation now implements `AddRuntimeTreeAuxiliaryName` (`0x00407040`), `ParseRuntimeTreeAuxiliaryNames` (`0x004070F0`), and `AddDefaultRuntimeTreeAuxiliaryNames` (`0x00407130`). The 0x28-byte auxiliary node is now confirmed as name `+0x00[32]`, self identity `+0x20`, and next `+0x24`. Addition rejects exact duplicate names, zero-allocates, invokes runtime operation 6 with the provisional identity pointer, frees if the callback clears it, copies the name, publishes self identity, and prepends. Parser-driven addition consumes all value tokens and always returns zero. Default addition splits the root `+0x96c` string using spaces only and returns one iff a nonempty token was encountered. Tests cover duplicate suppression, prepend ordering, callback rejection/free, parser exhaustion, repeated tokens, leading/trailing/repeated spaces, empty defaults, identities, and cleanup. Ghidra structures/names/fastcall prototypes/comments are synchronized and saved. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **494/854 functions**, with 35 library functions delegated and 325 recognized functions remaining.

- Conditional runtime-tree creation now implements `UpdateConditionalRuntimeTree` (`0x00406CB0`) and `CreateConditionalRuntimeTree` (`0x00406EA0`). Both preserve one-token local-resource versus two-token explicit-resource syntax, exact 32-byte fallback copying, `/V` typed object-field conditions, `/C` container conditions, and `/GLOBAL` parent-sentinel handling. The update form leaves an existing node unchanged when no condition appears, creates a missing node only when at least one condition was observed and all pass, destroys an existing node when conditions fail, and forces comment-tree flag `0x800` nodes false. The create form rejects only fully parsed false conditions; incomplete condition arguments are ignored before unconditional terminal creation. Tests cover local/explicit resources, true/false field and container conditions, existing-node destruction, global parent selection, top-level rejection, resource loading, and exact create arguments. Ghidra names/fastcall prototypes/comments are synchronized and saved. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **491/854 functions**, with 35 library functions delegated and 328 recognized functions remaining.

- Integer-expression parsing now implements `ParseScriptIntegerExpression` (`0x0040F4F0`) and removes the final production seam beneath typed parameter evaluation. It preserves literal-first parsing; first-four-byte `PARAM`, `RAND`, `RELZ`, `RELI`, `RELM`, `VALU`, and `PHAS` recognition; recursive bounds/offsets; entry-cursor rollback on ordinary failures; RAND defaults `-10000`/`10000`; zero defaults for absent relative offsets; link-84 `+0x2c/+0x30` and primary-link `+0x5c/+0x60` coordinate selection by token byte 4; runtime callback operations 9/10 for mouse coordinates and 11 with the address of primary-link `+0x4c` for phase; and object integer lookup. Tests cover null/literal/unknown input, PARAM success/failure, explicit/default RAND bounds, X/Y link coordinates with and without offsets, missing names/links, both mouse operations, VALUE success/missing field, PHASE success/missing link/name, exact callback sources, cursor restoration, and integration through typed-value parsing. Ghidra name/fastcall prototype/comment and post-type decompilation are synchronized and saved. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **489/854 functions**, with 35 library functions delegated and 330 recognized functions remaining.

- Parameter token evaluation now implements `ParseScriptTypedValue` (`0x00408AA0`), `ParseScriptParameterToken` (`0x0040EEB0`), and `EvaluateScriptParameter` (`0x0040F070`). Typed parsing preserves integer â†’ image-flag â†’ string ordering and cursor rollback before each fallback; the original nonzero test deliberately accepts image-flag `0xffffffff` as type 1. Parameter-token parsing selects a zero-based token using only tab/LF/CR/space separators, retains the unchecked 260-byte stack token, applies exact missing-token defaults (`0x07000000` for type 1, zero for types 2/4), and enforces requested-type matching only after parsing. Context evaluation maps a name through scratch text at parser `+0x0c` and reads the same index from creation text at `+0x08`. The `PARAM` branches recovered in the prior tranche are now production-wired to this implementation; the remaining seam is narrowed to the larger integer-expression parser `0x0040F4F0`. Tests cover integer/flag/string classification, the nonzero sentinel quirk, cursor behavior, whitespace/index selection, missing defaults, type match/mismatch, name lookup success, and absent names. Ghidra names/fastcall prototypes/comments, post-type decompilation, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **488/854 functions**, with 35 library functions delegated and 331 recognized functions remaining.

- Image/value flag parsing now implements `ParseScriptValueToken` (`0x0040F2C0`), `ParseImageFlag` (`0x0040E580`), and `ApplyRuntimeTreeImageFlags` (`0x00406B40`). Value tokens preserve recursive `PARAM`/`SVALUE` expansion and their fixed success lengths. Image flags cover the complete confirmed token table, including executable-resolved `OFF`/`ON`/`DUAL`, plus `BVALUE` object-field queries and `PARAM` evaluation. The consumer routes bit 1 to script-root `+4`, bit `0x04000000` to the newly confirmed palette-flags DWORD at `+8`, and all other bits to tree-owner flags `+0x2c`. The runtime-tree jump parser now uses the recovered value-token routine instead of its former seam. Tests cover every ordinary mapping, null/unknown/exhausted input, parameter success/failure, object boolean/string reads, recursive substitutions, exact flag routing, and empty termination. `0x0040F070` remains an explicit narrow parameter-evaluation seam. Ghidra names/prototypes/comments, the global root pointer, root `+8` field, post-type decompilation, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **485/854 functions**, with 35 library functions delegated and 334 recognized functions remaining.

- Runtime-tree configuration serialization now implements `AppendScriptRuntimeFlags` (`0x004068F0`), `SerializeRuntimeTreeSections` (`0x004069D0`), `SerializeRuntimeLanguage` (`0x00406BB0`), and `SerializeRuntimeFixedNameNodes` (`0x004073D0`). Flag output preserves nonzero gating, fixed PAL_NOADJUST/NOCOMMENT/NOSAVE ordering, and the empty statement produced by unrecognized-only bits. Tree output emits one leading CRLF, skips roots without a same-name parser context, writes resource/tree names, and adds GLOBAL only for parent sentinel `-1`. Language reads the confirmed root `+0x828` string. Fixed-name nodes now expose identity `+0x20`, flags `+0x24`, serialized value `+0x28[44]`, and next `+0x54`, and serialize as time/FILE statements after one leading CRLF. Tests cover missing/empty runtime data, recognized and unknown flags, skipped tree nodes, global scope, exact multi-node ordering, and byte-exact output. `RuntimeTreeNode +0x00[32]` is now confirmed as its name. Ghidra structures, names/fastcall prototypes/comments, post-type decompilation, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **482/854 functions**, with 35 library functions delegated and 337 recognized functions remaining.

- Top-level runtime-tree link routing now implements `UpdateRuntimeTreeGlobalLinks` (`0x00406360`) and replaces the final narrow seam in `DestroyRuntimeTreeNode`. A non-null removed-node parent suppresses all mutations. For top-level removal, seven replacement heads are routed independently: scene `+0x74`, secondary `+0xa4`, primary `+0x9c`, link-84 `+0x84`, link-8c `+0x8c`, link-7c `+0x7c`, and containers `+0x94`. Each goes either to its runtime-global head or, when the corresponding selector pointer is active, to the confirmed override field (`+0x40`, `+0x48`, or `+0x24`). Null replacement clears the same selected destinations. Tests cover the parent gate, all seven global destinations, all seven override destinations, null clearing, replacement publication, and non-selected destination preservation. Ghidra name/fastcall prototype/comment, caller decompilation, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **478/854 functions**, with 35 library functions delegated and 341 recognized functions remaining.

- Recursive runtime-tree destruction now implements `DestroyRuntimeTreeNode` (`0x00405E50`) and production-wires it into `DeactivateRuntimeTreeAndVisuals`. It resolves both identities, returns the replacement unchanged on a target miss, emits operation `0x40` for flag `0x8000`, recursively destroys every child while preserving the saved next sibling, removes and releases all six inclusive link ranges plus object containers, releases auxiliary/parser contexts, emits operations `0x0e`/`0x20` for flags `0x2000`/`0x4000`, optionally redispatches replacement flag `0x200`, calls the `0x00406360` global-link updater seam, unlinks through root/parent/previous branches, repairs the next back-link, frees the node, and returns the possibly redispatched replacement. Tests cover target/replacement misses, recursion, all owned range families and exact ordering, all notification flags, replacement redispatch, parent/root/previous unlink paths, back-link repair, and final frees. Ghidra name/fastcall prototype/comment/post-type decompilation and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **477/854 functions**, with 35 library functions delegated and 342 recognized functions remaining. The remaining narrow production seam is `0x00406360`.

- Runtime-tree auxiliary-node teardown now implements `ReleaseRuntimeTreeAuxiliaryNodes` (`0x004071E0`). The newly confirmed node list lives at tree `+0xb0`; its 0x28-byte entries link at `+0x24`. Teardown repeatedly unlinks the head, invokes the runtime callback with ECX operation 7, zero EDX, and the node on the stack, then calls `HeapFree` on the script runtime heap. Callback and free results are ignored. Tests cover an empty list, complete three-node head-order consumption, exact callback arguments, free order, and final null head. Ghidra now has the auxiliary-node structure, tree field, synchronized name/fastcall prototype/comment, post-type decompilation, and saved program. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **476/854 functions**, with 35 library functions delegated and 343 recognized functions remaining.

- Named runtime-tree section dispatch now implements `DispatchRuntimeTreeSection` (`0x00405380`) with its confirmed four-argument fastcall ABI (resource identity in ECX, node identity in EDX, two stack strings). It resolves the node and already-loaded resource, locates the named section, creates/reuses its parser context, and returns the parser dispatcher result. Only parser-context creation failure releases the resource identity; lookup and section misses do not. Tests cover every early return, the unique rollback path, and result propagation. Ghidra name/prototype/comment/post-type decompilation and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **475/854 functions**, with 35 library functions delegated and 344 recognized functions remaining.

- Recursive runtime-tree parser reset now implements `ResetRuntimeTreeParserContextRecursive` (`0x00405E00`) and `ResetRuntimeTreeParserContexts` (`0x00405DC0`). Each context cursor is restored to its start offset before scanning to the `0xffffffff` terminator; property code 10 resolves an included tree and recursively resets its complete parser-context chain. The identity wrapper performs no work on a lookup miss and otherwise visits the selected node's entire context chain. Tests cover lookup miss, ordinary properties, termination, multiple contexts, included-tree recursion, and a failed include resolution. Ghidra names/fastcall prototypes/comments and the saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **474/854 functions**, with 35 library functions delegated and 345 recognized functions remaining.

- Runtime-tree jump scanning and parser-context lifecycle now implement `FindAndCreateRuntimeTreeJump` (`0x00405D00`), `ReleaseRuntimeTreeParserContexts` (`0x004052F0`), and `FindExistingRuntimeTreeParserContext` (`0x00405350`). Jump scanning searches property `0x70`, matches the requested target, distinguishes local-resource two-name syntax from external-resource three-name syntax, creates with null parent/context, and rolls the cursor back on every failure while publishing the caller success cursor on success. Context release preserves decrement-if-nonzero semantics, removes resources at zero, frees every context, and deliberately leaves owner `+0x6c` unchanged. Tests cover exhaustion/mismatch, missing names, both resource forms, create failure/success cursor behavior, first/later/missing lookup, empty release, positive/one/zero reference counts, resource removal, free order, and the dangling owner head. `ScriptParserState` is now confirmed as the first 0x28 bytes of `RuntimeTreeParserContext`, with owner/resource/data/length/start/cursor fields synchronized in Ghidra. Win32 Debug build and CTest pass. Recovery is now **472/854 functions**, with 35 library functions delegated and 347 recognized functions remaining.

- Generic-resource cursor/token parsing now implements `SetRuntimeGenericResourcePosition` (`0x004050E0`) and `ReadRuntimeGenericResourceToken` (`0x00405110`). Position changes require an existing identity and a value strictly below resource length. Token reading preserves exact CR/LF skipping, delimiter/semicolon sentinel returns, output pre-clearing, non-committed failure cursors, capacity-sized copying with the original one-byte terminator overrun edge, delimiter consumption versus semicolon retention, and cursor publication. Tests cover missing roots/identities, strict position bounds, normal multi-token parsing, both sentinel stops, capacity truncation, zero capacity, output bytes, and exact positions. Ghidra names/prototypes/comments and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **469/854 functions**, with 35 library functions delegated and 350 recognized functions remaining. These functions now production-back the first shared parsing operations used by `DispatchRuntimeTreeParser` (`0x004056C0`).

- Runtime tree-node admission now implements `CreateRuntimeTreeNode` (`0x00405410`). It resolves resource/current-tree identities, returns existing same-name roots or descendants, requires an exact script section, rejects `class=TEMPLATE` and duplicate class names within the applicable top-level or descendant scope, allocates the exact 0xbc-byte node, copies exactly 32 name bytes, and preserves sentinel-root versus ordinary child/sibling linking. Parser-context failure frees the node before releasing the resource identity; successful parser dispatch returns its possibly different node and invokes callback operation `0x30` only when result flag `0x8000` is clear. Tests cover missing resources/sections, both existing-name paths, template and both duplicate-class scopes, allocation/context failures and rollback order, new root, sentinel prepend, sibling append, parser return propagation, and activation suppression. Ghidra name/prototype/comment, parser-dispatch prototype, and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **467/854 functions**, with 35 library functions delegated and 352 recognized functions remaining. The next dependency is the large property dispatcher `DispatchRuntimeTreeParser` (`0x004056C0`).

- Generic runtime-resource loading and parser-context creation now implement `FindOrLoadRuntimeGenericResource` (`0x00404EE0`) and `FindOrCreateRuntimeTreeParserContext` (`0x00405210`). Resource loading clears a 256-byte basename workspace, matches existing records by exactly eight DWORDs, preserves the callback-6 in/out contract, zero-allocates the exact 0x3c-byte record, publishes self identity plus resource data/metadata, and appends at the list tail. Parser-context creation reuses a named context or zero-allocates the exact 0x254-byte layout, publishes resource/start/cursor state, increments the resource reference count, initializes all inline-buffer pointers, and appends at owner `+0x6c`. Tests cover missing roots, first/later hits, callback failure, allocation failure, head/tail insertion, optional creation text, exact arguments/layouts, and mutation-free failures. Ghidra has synchronized prototypes/comments and the confirmed resource/context structures. Win32 Debug build and CTest pass. Recovery is now **466/854 functions**, with 35 library functions delegated and 353 recognized functions remaining. The immediate constructor `0x00405410` is audited but remains unresolved pending its complete branch/dependency closure.

- Runtime-tree activation now implements `ActivateRuntimeTreeWithNotifications` (`0x00426560`) with its exact four-input fastcall wrapper ABI. It sends the begin notification unconditionally, resolves the first runtime name through `0x00404EE0`, and calls `0x00405410` with the resolved resource in ECX, the third wrapper input in EDX, and the second/third inputs on the stack; the fourth wrapper input is unused. Successful activation preserves ordered flag clearing, comment activation, and completion notification, while a miss omits all post-create effects. `ProcessPendingRuntimeTreeSwitch` is corrected to pass the confirmed adjacent 0x104-byte globals at `0x0047F1AC`/`0x0047F2B0` plus the pending node twice instead of null placeholders. Tests cover miss, ordinary success, empty-name and flag-0x1000 clearing, flag-0x800 comment activation, exact ordering, argument duplication, and pending-switch global forwarding. Ghidra names/prototypes/comments/globals and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **464/854 functions**, with 35 library functions delegated and 355 recognized functions remaining.

- Runtime-tree deactivation now implements `DeactivateRuntimeTreeAndVisuals` (`0x00426600`) with its corrected two-register fastcall ABI: ECX is the identity and EDX is forwarded to `0x00405E50`. A resolve miss returns zero untouched. A hit sends begin/end private notifications, performs root-only primary script-object visual cleanup using newly confirmed object flags `+0x42c` and visual pointer `+0x470`, requests destruction through visual scene identity `+0x158`, clears script flags 2 then 4 for flag `0x1000` or the empty-name sentinel, conditionally deactivates comment mode for flag `0x800`, and returns the exact downstream tree-destruction result. Tests cover miss, root/non-root behavior, protected and removable visuals, notification/callback order, both flag paths, visual clearing despite failed removal, and return propagation. `DestroyRuntimeCommentTrees` now uses the real recovered implementation rather than its former seam. Ghidra name/fastcall prototype/comment/caller decompilation and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **463/854 functions**, with 35 library functions delegated and 356 recognized functions remaining.

- Pending runtime-tree switching and comment-tree cleanup now implement `ProcessPendingRuntimeTreeSwitch` (`0x004210A0`) and `DestroyRuntimeCommentTrees` (`0x00423740`). The switch routine is gated by flag `0x04000000`, clears and later publishes the confirmed accumulated-tree flags DWORD at context `+0x90c`, distinguishes null/same/different activation identities, preserves finalize/rebuild/pointer-refresh ordering, and always clears the pending flag after processing. Comment cleanup enumerates the entire pointer-root tree, destroys/deactivates every node with flag `0x800`, and runs its two finalizers once iff any matching node was found. Tests cover gating, empty/plain/multiple-node enumeration, null/same/different activation, return values, accumulated flags, coordinates, and exact callback order/counts. Downstream functions `0x00405DC0`, `0x00426560`, `0x00426600`, `0x00426700`, and `0x004268B0` remain explicit unresolved seams. Ghidra has the synchronized names/prototypes/comments, confirmed `RuntimeCommandLoopState +0x90c` field, post-type decompilation, and saved program. Win32 Debug build and CTest pass. Recovery is now **462/854 functions**, with 35 library functions delegated and 357 recognized functions remaining.

- Runtime plan-mode synchronization and queued pointer-message dispatch now implement `SynchronizeRuntimePlanMode` (`0x00421130`) and `ProcessRuntimePairMessage` (`0x004211A0`). Plan synchronization preserves the asymmetric desired/applied bits (`0x40000000`/`0x80000000`), invokes set/clear only on an edge, rebuilds through still-unresolved `0x00426700` only when the transition reports a change, and always publishes the applied bit after an attempted edge. Pair processing always attempts one dequeue, suppresses dispatch when display flag 4 is set, and maps only `0x200/0x201/0x202/0x204`, with unsigned low/high-word pointer coordinates. Tests cover every plan-bit combination and helper result, empty/suppressed/unknown pairs, all four dispatches, exact coordinates, returns, and callback counts. The three pointer-button handlers and plan rebuild remain explicit narrow unresolved seams. Ghidra names/prototypes/comments/post-type decompilation and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **460/854 functions**, with 35 library functions delegated and 359 recognized functions remaining.

- Audit of dialog wrappers `0x00417550`/`0x004175F0` confirmed their fixed 0x98-byte stack contexts and dialog resources 101/103, but the second dialog procedure begins at `0x004188A0` inside Ghidra's merged `0x00418560` body rather than at a recognized function boundary. Both wrappers remain unresolved until that shared/merged callback control flow is repaired; no callback behavior was guessed.

- CDF error reporting and the comment-package wrapper now implement `GetCdfError` (`0x00428280`) and `WriteCommentCdfPackage` (`0x004176A0`). Error lookup returns the archive-local error for a non-null archive and the global constructor/open error otherwise. The package wrapper creates an exact three-slot writer, conditionally appends NUL-inclusive `COMMENT.TXT`, only an 8-bit `COMMENT.BMP` using the unaligned width/height/bit-count fields at `+0x12/+0x16/+0x1c`, and `START.CFG`; every append failure captures the error before finalization. Tests cover construction failure, empty success, all three payload layouts, non-8-bit bitmap suppression, each append failure, and error/finalization ordering. Ghidra names/prototypes/comments and saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **458/854 functions**, with 35 library functions delegated and 361 recognized functions remaining.

- CDF writer construction now implements `CreateCdfWriter` (`0x00429630`). It creates/truncates a `CDF97a` output with exact Win32 arguments, zero-allocates the 0x207c-byte archive and `capacity * 0x2c` entry storage, publishes only the low 16 bits of capacity, initializes the pointer table, and emits the 19-byte placeholder header as 7/4/4/4 writes. Header-write results are ignored. Error state is `0x1000000` before open, `0x20000` during allocation, and zero only on success; every allocation failure has the original cleanup order. Tests cover open failure/GetLastError, both allocation failures, handle cleanup, successful layout/pointer construction, signature/header writes, ignored write failures, and error transitions. Ghidra name/prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **456/854 functions**, with 35 library functions delegated and 363 recognized functions remaining.

- CDF entry append/dispatch now implements `AppendCdfWriterEntry` (`0x004297E0`). It rejects null/full writers without side effects, copies the unchecked NUL-terminated name into the selected 0x2c-byte entry, classifies payload data, stores size and current file position, and dispatches raw or compressed writing. The compressed path adds `0x10` to the byte flag with byte wrapping. After either writer returns—including failure—it increments writer index and adds the original size to index-data size before returning the writer result. Tests cover both rejection paths, exact metadata and callback ordering, raw success, compressed failure, byte-wrap behavior, and unconditional counter publication. The compressed writer `0x00429B50` remains explicitly unresolved behind the narrow dependency seam. Ghidra name/prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **455/854 functions**, with 35 library functions delegated and 364 recognized functions remaining.

- CDF writer destruction now implements `FinalizeCdfWriter` (`0x004298E0`). A null writer returns zero untouched. Otherwise it copies writer index `+0x134` into entry count `+0x120`, invokes the still-unresolved compressed-index serializer `0x00429070`, seeks to byte 7, writes the index size, writer count, and index-data size as three ordered DWORDs, obtains the process heap, closes the file, and frees entry storage followed by the archive. Every callback result is ignored and the function always returns zero. Tests prove the complete nine-event order and deliberately return failures/short writes from every observable lifecycle callback to verify unconditional continuation. Ghidra name/prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **454/854 functions**, with 35 library functions delegated and 365 recognized functions remaining.

- Uncompressed CDF output now implements `WriteUncompressedCdfEntry` (`0x00429A90`). It selects the entry at the confirmed writer index `CdfArchive +0x134`, performs the original otherwise-unused `GetProcessHeap` call, clears the archive error, seeks to the entry offset, and writes in exact 0x200000-byte chunks. A short write sets error 2 and returns zero; zero-length entries still clear the error and seek before returning one. Tests cover three-chunk output, exact source advancement and chunk sizes, failure after one successful chunk, and the zero-length path. Ghidra now names the `+0x134` field `dwWriteEntryIndex`; function name/prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **453/854 functions**, with 35 library functions delegated and 366 recognized functions remaining.

- Runtime-media classification and the GAG-owned compression input adapter now implement `ClassifyRuntimeMediaData` (`0x004299B0`) and `ReadCompressorInput` (`0x00404920`). Classification preserves the exact animation/BMP/WAVE/config priority and compares only seven bytes of `WAVEfmt` and five bytes of `[CFG]`, ignoring the following source byte as the original zero-padded `strncpy` locals do. The input adapter copies the smaller of requested/remaining bytes and advances only by the copied size. Tests cover priority collisions, ignored signature-adjacent bytes, partial/exhausted/zero-size reads, and cursor advancement.
- The surrounding compression island is confirmed as embedded GNU gzip/deflate library code. Twenty-five functions now carry canonical names/prototypes and `library`/`gzip-deflate` tags in Ghidra: the complete `trees.c`/`bits.c` family from `ct_init` through `bi_init`, plus `lm_init`, `longest_match`, `fill_window`, `deflate`, and `deflate_fast`. These are delegated rather than reconstructed. The false function at `0x00412B9A`, which began inside `deflate_fast`, was deleted after boundary verification; the authoritative recognized total is therefore 854. Ghidra comments, post-type decompilation, tags, and saved program are synchronized. Recovery is now **452/854 functions**, with 35 library functions delegated and 367 recognized functions remaining.

- Path numeric identification now implements `ParsePathNumericIdentifier` (`0x00418230`) with its corrected integer return type. It scans from the first backslash when present, otherwise rescans a nonempty full string, concatenates every digit while ignoring nondigits, and returns `-1` for empty/no-digit inputs. Tests cover both scan origins, ignored prefix digits before a backslash, interspersed nondigits, and all failure paths. Ghidra name/fastcall prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **450/855 functions**, with 10 library functions delegated and 395 recognized functions remaining.

- Scoped serialization plus standalone text setup/rendering now implement four functions: `AppendScriptTextScopedTokens` (`0x0040CE90`), `InitializeRuntimeStandaloneText` (`0x00411800`), `DrawRuntimeStandaloneText` (`0x004118C0`), and `GetRuntimeMediaBackendType` (`0x0042B5B0`). The scoped serializer preserves its valid-input grammar assumptions, decimal suffix handling, unchecked 32-byte scratch, and terminal-token quirk that emits only the scope without a final space/colon. Media type lookup performs a mutex-protected first-match list scan. Standalone text accepts only backend type `0xac`, rejects empty text without touching output, clears/initializes an exact 0x3c-byte state, measures through `strlen+1`, and draws through the generic text renderer with flags zero.
- Tests cover multi-token/colon-number/terminal/empty scoped serialization, missing/first/later media backend type lookup and mutex ordering, wrong-type and empty-text state preservation, exact standalone state fields/bounds, and real font-backed rendering. Ghidra has the 0x3c-byte state structure, synchronized names/fastcall prototypes/comments, post-type decompilation, and saved program. Recovery is now **449/855 functions**, with 10 library functions delegated and 396 recognized functions remaining.

- Generic token and integer-literal parsing now implement `ExtractScriptToken` (`0x0040F0A0`) and `ParseScriptIntegerLiteral` (`0x0040F380`). Both preserve the exact skip/reject delimiter sets and skip complete parenthesized groups while rejecting malformed/bracket-crossing groups. Token extraction clears the caller-specified capacity, applies the `capacity-1` ceiling, and commits the cursor on success. Integer parsing accepts one optional sign, consumes only decimal digits, returns `0x7fffffff` on failure, and leaves the cursor unchanged on every failure path.
- Tests cover skipped whitespace/punctuation/groups, delimiter stopping, capacity truncation, null/exhausted/rejected/malformed token paths, positive/negative/zero integer parsing, sign-without-digits, rejected delimiters, malformed groups, and cursor commit/rollback. Ghidra names, fastcall prototypes, comments, post-type decompilation, and saved program are synchronized. Recovery is now **445/855 functions**, with 10 library functions delegated and 400 recognized functions remaining. Recursive expression/string functions `0x0040EEB0`, `0x0040F070`, `0x0040F2C0`, and `0x0040F4F0` remain unresolved pending their full callback and recursive ABI closure.

- Executable opcode classification now implements `ParseScriptOpcode` (`0x0040DFD0`). It reuses the recovered slash-token extractor and maps all 52 exact uppercase opcode strings to their sparse codes, including the high-bit command families and distinct `GAME=0x0000c000`/`GEXIT=0x0000d000`. Extraction failure returns `0xffffffff`; an unknown extracted token returns zero. Table-driven tests exercise every mapping plus unknown, missing-slash, and null-parser paths. Ghidra prototype/comment/post-type decompilation and saved program are synchronized. Recovery is now **443/855 functions**, with 10 library functions delegated and 402 recognized functions remaining.

- Parser token extraction/classification now implements six functions: `ParseScriptPropertyCode` (`0x0040D8A0`), `ParseScriptScopeCode` (`0x0040DC00`), `ExtractScriptPropertyName` (`0x0040EA40`), `ExtractScriptScopeName` (`0x0040EB70`), `ExtractScriptParenthesizedText` (`0x0040ECB0`), and `FindWhitespaceTokenIndex` (`0x0040ED80`). `ScriptParserState` is confirmed only through 0x28: opaque `+0x00..+0x17`, text `+0x18`, bounded length `+0x1c`, unresolved DWORD `+0x20`, and cursor `+0x24`. The extractors preserve exact delimiter sets, 31-byte limits, cursor advancement, pre-clearing, failure sentinels, and bracket rejection. The classifiers map 30 exact lowercase property tokens and 34 exact uppercase scope tokens from raw executable strings, returning `0xffffffff` on extraction failure and zero for an unknown extracted token.
- Tests cover successful, exhausted, boundary, null-context, truncation, cursor, missing-parenthesis, bracket-abort, and exact whitespace-token behavior; every one of the 64 classifier mappings is exercised. Ghidra has the confirmed parser structure, synchronized names/fastcall prototypes/comments, post-type decompilation, and saved program. Recovery is now **442/855 functions**, with 10 library functions delegated and 403 recognized functions remaining.

- Bounded script-text lookup now implements `FindScriptPropertyValue` (`0x0040D740`) and `FindScriptSection` (`0x0040D830`). The property scanner clears exactly 32 output bytes, searches only within the supplied bounds and before a section boundary, limits names/values to 31 bytes, recognizes the exact five value delimiters, and preserves the original one-past stopping cursor (including the distinct null-output result). The section scanner performs an exact case-sensitive bracketed-name comparison and preserves the original decrement-before-test behavior that rejects `]` in the final supplied byte.
- Focused tests cover first/later/missing properties, section-boundary and exhausted searches, null output, 31-byte name/value limits, exact/prefix/missing sections, no brackets, zero length, and the final-byte boundary. Ghidra names, fastcall prototypes, comments, post-type decompilation, and saved program are synchronized. Recovery is now **436/855 functions**, with 10 library functions delegated and 409 recognized functions remaining.

- Script serializer property/scope emission now implements three functions: `AppendScriptTextProperty` (`0x0040D180`), `AppendScriptTextScope` (`0x0040D440`), and `AppendScriptTextPreloadDirective` (`0x0040D610`). Raw instruction operands and executable strings establish every recognized code/token mapping. Property emission writes `name=`, then writes the value and one trailing space only for a non-null value. Scope emission writes `/TOKEN:` for eight exact codes, while the separate preload emitter accepts only `0x50000000`. Null buffers and unrecognized codes are no-ops.
- Exhaustive tests cover all 20 property codes, all eight general scope codes, the preload code, null values, null buffers, and unrecognized-code preservation. Ghidra names, fastcall prototypes, comments, post-type decompilation, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **434/855 functions**, with 10 library functions delegated and 411 recognized functions remaining.

- Bounded random selection and the first script-text serialization tranche now implement eight functions: `SelectBoundedRandomValue` (`0x0040CD00`), `CreateScriptTextBuffer` (`0x0040D0B0`), `ClearScriptTextBuffer` (`0x0040D0E0`), `BeginScriptTextDocument` (`0x0040D0F0`), `EndScriptTextDocument` (`0x0040D140`), `EndScriptTextStatement` (`0x0040D400`), `AppendScriptTextDelimiter` (`0x0040D650`), and `AppendScriptTextInteger` (`0x0040D690`). The serialization buffer is confirmed as a 12-byte header over one 64,000-byte `VirtualAlloc` region: length `+0x00`, capacity `0xf9f4` at `+0x04`, and inline data pointer `+0x08`; script-root `+0xfc4` holds it. The routines preserve exact terminator overwrites, CR/LF placement, one-space trimming, signed-magnitude decimal formatting, and null-buffer behavior. Random selection preserves one-time tick seeding, asymmetric `-10000/10000` clamps, empty interval behavior, and CRT modulo selection.
- Deterministic tests cover allocation failure/header initialization, reset/null paths, all text and delimiter placements, trailing-space/no-space statement endings, zero/positive/negative integer output, seed-once behavior, both clamps, modulo selection, and empty ranges. Production calls still use `VirtualAlloc`, `GetTickCount`, `srand`, and `rand`; the non-original test seam preserves exact arguments. Ghidra has the new structure/root field plus synchronized names, ABIs, prototypes, comments, post-type decompilation, and saved program. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **431/855 functions**, with 10 library functions delegated and 414 recognized functions remaining.

- Script-condition container ordering/state and runtime-event consumption now implement ten functions: `AcknowledgeCurrentRuntimeEventRecord` (`0x0040C390`), `ReadRuntimeEventRecord` (`0x0040C440`), `FindLastScriptObjectContainer` (`0x0040C8A0`), `FindScriptObjectContainerInsertionPredecessor` (`0x0040C900`), `InsertScriptObjectContainer` (`0x0040C950`), `RemoveScriptObjectContainerRange` (`0x0040C9F0`), `DestroyScriptObjectContainer` (`0x0040CB40`), `ScriptObjectContainerStateMatchesByIdentity` (`0x0040CBA0`), `ScriptObjectContainerStateMatchesByName` (`0x0040CC20`), and `FindScriptConditionContainerByName` (`0x0040CCB0`). The container is confirmed as 0x1b4 bytes: fixed 0x20-byte name, identity/next, current and required masks, count, and 32 twelve-byte object/active-mask/field-mask slots. Its ordering uses node `+0x94/+0x98` and script-root `+0xf98/+0xfb4`. Event consumption preserves pre-clearing output `+0x38`, exact 0x40-byte copies, optional ring advance/wrap, and two-stage acknowledge/consume behavior.
- Tests cover all event empty/copy/peek/advance/wrap/acknowledge paths; all container tail/predecessor/insertion/removal branches; exact name/identity misses and matches; state-mask rebuilding and asymmetric missing-container success; and ordered destructor calls with success/failure accumulation. Ghidra structures, fields, names, calling conventions, prototypes, comments, and saved program are synchronized. Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **423/855 functions**, with 10 library functions delegated and 422 recognized functions remaining. Parser-dependent functions `0x0040B850`, `0x0040BF60`, `0x0040C1E0`, `0x0040C260`, `0x0040C2F0`, and `0x0040C570` remain unresolved pending their parser closure.

- The opaque node `+0x7c/+0x80` ordered-link family now implements `FindLastRuntimeTreeLink7C` (`0x0040BCD0`), `FindRuntimeTreeLink7CInsertionPredecessor` (`0x0040BD30`), `InsertRuntimeTreeLink7C` (`0x0040BD80`), and `RemoveRuntimeTreeLink7CRange` (`0x0040BE20`). Ghidra confirms 0xb4-byte records with `next +0x24`, node head/tail `+0x7c/+0x80`, and script-root globals `+0xf90/+0xfac`; semantic naming remains intentionally offset-neutral. Tests cover recursive reverse-sibling tail selection, both parent sentinels, predecessor fallback, global and hierarchical insertion, empty removal, both global removal modes, interior splicing, ancestor propagation, and tail repair. Ghidra structures, fields, names, fastcall prototypes, comments, and saved program are synchronized; Win32 Debug build, CTest, formatting, and whitespace gates pass. Recovery is now **413/855 functions**, with 10 library functions delegated and 432 recognized functions remaining. `0x0040B850` and `0x0040BF60` remain unresolved because their parser/matcher dependencies are not yet closed.

- Runtime-tree link mutation/lookups and the next ordered family now implement eight functions: `UpdateRuntimeTreeLink84` (`0x0040B280`), `FindGlobalRuntimeTreeLink84ByName` (`0x0040B380`), `FindGlobalRuntimeTreeLink84ByIdentity` (`0x0040B3C0`), `FindLastRuntimeTreeLink8C` (`0x0040B560`), `FindRuntimeTreeLink8CInsertionPredecessor` (`0x0040B5C0`), `InsertRuntimeTreeLink8C` (`0x0040B610`), `RemoveRuntimeTreeLink8CRange` (`0x0040B6B0`), and `FindGlobalRuntimeTreeLink8CByName` (`0x0040B800`). The 0x68-byte link84 layout is now confirmed through rectangle fields, conditional scalar fields, two current identities, and their prior snapshots. Its rectangle update forwards deltas to the associated primary link before replacement. The new opaque 0x54-byte link8c family uses next `+0x24`, node head/tail `+0x8c/+0x90`, and globals `+0xf9c/+0xfb8`; semantics remain unresolved and naming stays offset-neutral.
- Tests cover all link84 misses, sentinel/nonzero scalar rules, identity snapshot rules, unchanged/nonzero rectangles and primary-link delta propagation, both global lookup modes, plus every recursive/predecessor/insertion/removal/global-lookup branch for link8c. Ghidra structures, fields, prototypes, names, comments, and program are synchronized; Win32 Debug build and CTest pass. Recovery is now **409/855 functions**, with 10 library functions delegated and 436 recognized functions remaining.

- A fourth runtime-tree ordered-link family now implements `FindGlobalRuntimeTreePrimaryResourceLinkByName` (`0x0040A990`), `FindLastRuntimeTreeLink84` (`0x0040AFE0`), `FindRuntimeTreeLink84InsertionPredecessor` (`0x0040B040`), `InsertRuntimeTreeLink84` (`0x0040B090`), and `RemoveRuntimeTreeLink84Range` (`0x0040B130`). Ghidra confirms the opaque 0x68-byte records link at `+0x24`, node head/tail are `+0x84/+0x88`, and global head/tail are script-root `+0xf94/+0xfb0`. The record's semantic role remains unresolved, so source and Ghidra deliberately retain offset-neutral `RuntimeTreeLink84` naming. Tests cover global primary fixed-width lookup, recursive reverse-sibling tail selection, predecessor sentinels/siblings/fallback, all global and hierarchical insertion paths, and all removal/tail-repair paths. Structures, prototypes, names, comments, and the saved Ghidra program are synchronized. Recovery is now **401/855 functions**, with 10 library functions delegated and 444 recognized functions remaining.

- Primary-link mutation and fixed suffix formatting now implement `UpdateRuntimeTreePrimaryResourceLink` (`0x0040A860`) and `AppendThreeDigitDecimalSuffix` (`0x0040A920`). The updater preserves tree/link misses, exact identity matching at link `+0x20`, exact 32-byte name comparison/copy, resource-identity snapshot and clearing only on a changed name, nonzero-only signed coordinate deltas with snapshots, and nonzero-only `+0x54` replacement. The formatter zeroes exactly 32 bytes, copies the prefix, emits exactly three quotient/remainder characters for 100/10/1 without clamping values above 999, then terminates. `RuntimeTreePrimaryResourceLink` is now confirmed through 0x8c and synchronized in Ghidra. Focused tests cover all mutation gates, first/later/missing links, unchanged/changed/null names, signed deltas, snapshots, zero/nonzero replacement, suffix boundaries, clearing, and the above-999 byte behavior. Recovery is now **396/855 functions**, with 10 library functions delegated and 449 recognized functions remaining.

- Primary-resource tree ordering now implements `FindRuntimeTreePrimaryResourceInsertionPredecessor` (`0x0040A560`), `InsertRuntimeTreePrimaryResourceLink` (`0x0040A5B0`), and `RemoveRuntimeTreePrimaryResourceLinkRange` (`0x0040A650`). Raw assembly confirms primary `next` at `+0x24`, node head/tail at `+0x9c/+0xa0`, and the global head/tail at script-root `+0xf8c/+0xfa8`; the latter storage is also interpreted by plan-state functions as `RuntimePlanNode`, so source preserves the confirmed prefix-layout overlap explicitly. Tests cover both parent sentinels, preceding-sibling and parent-tail predecessor selection, global head/tail insertion, ancestor propagation, empty removal, real-parent head/interior removal, global head/interior removal, and tail repair. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Recovery is now **394/855 functions**, with 10 library functions delegated and 451 recognized functions remaining. Continue with adjacent primary-resource/plan helpers after auditing their dependency closure.

- Ordered runtime-tree removal now implements `RemoveRuntimeTreeSceneLinkRange` (`0x00409920`) and `RemoveRuntimeTreeSecondaryResourceLinkRange` (`0x00409CB0`). Each preserves the initial empty-tail no-op, real-parent interior splice, head-range removal and ancestor propagation, null-parent global rewrite, `-1`-parent head/interior removal, and conditional global-tail repair. Tests cover every control-flow family for both native link layouts; Ghidra names/prototypes/comments are synchronized and saved. Win32 Debug build and CTest pass. Recovery is now **391/855 functions**, with 10 library functions delegated and 454 recognized functions remaining. Continue with the analogous primary-resource ordering helpers around `0x0040A500`.

- Runtime-tree ordered insertion now implements `FindRuntimeTreeSceneInsertionPredecessor` (`0x00409830`), `InsertRuntimeTreeSceneLink` (`0x00409880`), `FindRuntimeTreeSecondaryResourceInsertionPredecessor` (`0x00409BC0`), and `InsertRuntimeTreeSecondaryResourceLink` (`0x00409C10`). The predecessor helpers preserve null/`-1` parent sentinels, reverse preceding-sibling scans through subtree-tail helpers, and parent-tail fallback. The insertion helpers preserve distinct null-parent append and `-1`-parent splice behavior, plus repeated parent-head insertion and upward propagation when no local predecessor exists.
- Tests cover both sentinel parents, global head/tail paths, sibling-tail selection, parent-tail fallback, local splicing, and hierarchical propagation for both link types. `ScriptRuntimeRoot` is confirmed through 0xfc4 with secondary and scene global tails at `+0xfbc/+0xfc0`; Ghidra structures, names, fastcall prototypes, comments, and the saved program are synchronized. Win32 Debug build and CTest pass. Recovery is now **389/855 functions**, with 10 library functions delegated and 456 recognized functions remaining. Continue with the adjacent scene/secondary removal or primary-link ordering cluster, selecting only a dependency-closed group.

- Runtime-tree ancestry and global link lookup now implement `FindRuntimeTreeAncestorRoot` (`0x00406600`), `FindGlobalRuntimeTreeSceneLinkByName` (`0x00409A40`), and `FindGlobalRuntimeTreeSecondaryResourceLinkByName` (`0x00409E00`). Raw assembly resolves `0x00406600` as a pointer-return function despite the decompiler's misleading 64-bit display: it finds the selected node and follows `+0x24` parents until NULL or the `-1` sentinel, returning the highest real node. The two global lookups preserve exact 0x20-byte comparison, heads at script-root `+0xfa4/+0xfa0`, next offsets `+0x40/+0x48`, first-match order, and their asymmetric null-root checks.
- Tests cover multi-level ancestry, NULL and `-1` parent termination, missing identities/runtime, first/later/missing global matches, and the secondary lookup's null-runtime path. Ghidra names/prototypes/comments and `ScriptRuntimeRoot +0xfa0/+0xfa4` fields are synchronized and saved. Recovery is now **385/855 functions**, with 10 library functions delegated and 460 recognized functions remaining. Continue through the scene/secondary global-link insertion and removal helpers as a coherent typed-list cluster.

- Runtime-tree root and fixed-name-list leaves now implement `GetRuntimeTreeRoot` (`0x00406980`), `FindRuntimeTreeTail` (`0x004069A0`), `FindRuntimeFixedNameListNode` (`0x00407380`), and `DestroyRuntimeFixedNameListNodes` (`0x00407440`). They preserve null-runtime/null-list returns, root-level `+0xb4` tail traversal, exact 0x20-byte fixed-name comparison, `+0x54` list traversal, forward runtime-heap freeing with flags zero, and post-destruction head clearing. `RuntimeFixedNameListNode` is confirmed as 0x58 bytes and the root list pointer is confirmed at `ScriptRuntimeRoot +0xf88`.
- Focused tests cover missing runtime, empty/single/multi-root results, first/later/missing name matches, ordered destruction, empty destruction, and null-runtime destruction. Ghidra names/prototypes/comments, the new structure, and the `+0xf88` root field are synchronized and saved. Recovery is now **382/855 functions**, with 10 library functions delegated and 463 recognized functions remaining. `0x00408AA0` remains unresolved: its apparent scalar dependency at `0x0040F4F0` is actually a recursive expression evaluator with numerous unresolved callbacks, and its string alternative at `0x0040F2C0` is recursive; recover that parser closure before the dispatcher.

- Script-field creation/snapshot and list teardown now implement `QueryOrCreateScriptObjectField` (`0x00408480`), `GetScriptObjectFieldSnapshot` (`0x004089E0`), `DestroyScriptObjectStates` (`0x00408D80`), `RemoveRuntimeVisualObject` (`0x004091B0`), and `DestroyRuntimeVisualObjects` (`0x004092E0`). Field creation preserves the 32-field limit, output-bit overwrite, `0x03000000`/`0x07000000` active tokens, `0x7fffffff` failures, type-specific active-bit rules, exact 0x20-byte copies, and the type-4 caller-buffer overwrite after copying. Snapshot export zeroes all 0x68 bytes before lookup and publishes two strings, normalized activity, integer, and string. Teardown preserves list order, head/interior unlinking, runtime-heap arguments, returned `HeapFree` result, and the original null-root asymmetry.
- Tests cover existing active/inactive fields, missing objects, full capacity, all create types and value boundaries, unsupported types, exact snapshot layout/content/zero-on-miss, ordered object destruction, null-root no-op, visual head/interior/missing removal, propagated free failure, and empty/nonempty visual teardown. Ghidra has the new 0x68-byte snapshot structure, names/prototypes/comments, `RuntimeVisualObject::pIdentity`, and a saved program. Recovery is now **378/855 functions**, with 10 library functions delegated and 467 recognized functions remaining. Next audit `0x00408AA0` only after its three parser dependencies are reconstructed, or continue with another dependency-closed script/runtime cluster.

- The fixed-name/script-object tranche now implements `CopyRuntimeTreeCommandName` (`0x00406580`), `CreateScriptObjectState` (`0x00408340`), `FindScriptObjectByIdentity` (`0x00408420`), `GetScriptObjectInteger` (`0x004087A0`), `GetScriptObjectString` (`0x00408800`), `AddScriptObjectInteger` (`0x00408870`), and `CompareScriptObjectField` (`0x00408900`). It preserves the four exact command-token mappings, invalid-command NUL write, zeroed 0x904-byte heap allocation, exact 0x20-byte name copies/comparisons, self identity, primary-list-before-container lookup order, 0x0c-byte container slots, integer sentinel `0x7fffffff`, exact string output, signed integer adjustment with active-bit updates, and the comparison routine's intentional true result for missing/unsupported cases.
- Focused tests cover every command mapping, allocation failure/success and arguments, both identity-search scopes and misses, field hits and misses, return sentinels, exact string copies, positive/non-positive integer adjustment, all three comparison types, and unsupported/missing comparisons. Ghidra names/prototypes/comments and `GagScriptObjectState::pIdentity` are synchronized and saved. Recovery is now **373/855 functions**, with 10 library functions delegated and 472 recognized functions remaining. Continue with the next exact script-object/tree leaf cluster; do not use `tools/cdf_extractor` as evidence.

- Runtime-target begin is now production-backed through `BeginDisplayTarget` (`0x00414360`), replacing the diagnostic begin seam used by the already-recovered `UpdateRuntimeTarget` (`0x004280D0`). It preserves the target-active acquisition loop and raced recheck, DirectDraw `IsLost`/`Restore`/`Lock` vtable slots `0x60/0x6c/0x64`, exact `0x887601c2` lost-surface comparison, zeroed 0x6c-byte lock descriptor, output surface/pitch/dimensions, GDI pitch `(bpp>>3)*width`, and failure-specific flag behavior. DirectDraw restore failure returns `0x200000` while deliberately retaining active flag `0x40000000`; lock and missing-DIB failures clear it.
- The production target API now routes draw mode through `SynchronizeDisplayRegion` and begin mode through a non-original ABI adapter to `BeginDisplayTarget`. Focused tests cover GDI success/missing pixels, DirectDraw success, restore failure, lock failure, initial/raced busy acquisition, exact outputs and event order. Ghidra is synchronized with the corrected mode-dependent `RuntimeCommandBounds` interpretation: draw mode uses four coordinates, while begin mode uses DWORDs `+0x04/+0x08/+0x0c` as rectangle/pitch/pixel output pointers. Recovery is now **342/855 functions**, with 10 library functions delegated and 503 recognized functions remaining. Continue from the target lifecycle's unresolved release callback behind `EndDisplayTarget`, then audit adjacent display begin/end helpers.

- The remaining scene-transition leaf and shared region operation are now reconstructed: `ApplyRectangleRuntimeSceneTransition` (`0x004272D0`) and `SynchronizeDisplayRegion` (`0x00414220`). The rectangle transition preserves size quantization, special `0xff` full-frame behavior, centered close/open geometry, four exact edge strips, signed clamps/comparisons, distinct `timeGetTime` versus `GetTickCount` clocks, two-ms pacing, optional backend palette application, DirectDraw/GDI lock failure paths, and the unqualified acquired-record leak. The synchronization function preserves the 5-ms busy wait and post-lock flag recheck, DirectDraw vtable slots `0x1c/0x14`, exact 100-byte effects block, GDI `BitBlt`/`PatBlt` operations, and unconditional final lock release.
- Focused tests cover fallback/leak and unsupported paths, one-step and immediate close/open transitions, exact clip/strip geometry and ordering, palette setup, both DirectDraw modes, both GDI modes, unsupported synchronization mode, initially busy and raced-busy lock paths. Both prior transition seams now call the recovered synchronization function. Ghidra function names/prototypes/comments, the 257-entry palette workspace, and opaque 24-byte display critical section are synchronized and saved. Recovery is now **341/855 functions**, with 10 library functions delegated and 504 recognized functions remaining. The complete transition selector closure from `0x00426D50` is now production-backed; continue to the next unresolved startup/platform dependency identified from current callers.

- Palette transition `ApplyPaletteRuntimeSceneTransition` (`0x00426F40`) is reconstructed from its full x86 body. It preserves the separate activation and caller-step bytes, 257-DWORD backend palette copy from `+0x1c`, protected palette entry zero, reverse-order entries 1–236 traversal, byte-wrapping fade-out/fade-in arithmetic, two-ms `timeGetTime` pacing, `Sleep(0)` contention path, optional center-surface preparation, forced `0xff` stepping and invalidation under scene flag `0x40000`, final surface clear, and every resource/display lock release edge. Missing records, unsupported types, and failed animation display acquisition remain inactive regardless of the caller step.
- Focused tests cover missing and unsupported resources, failed display acquisition, fade-out completion, fade-in restoration, optional palette preparation, forced-step invalidation, dimensions/rectangles, and exact call order. Ghidra now names/types the function and its 257-entry workspace at `0x004808D8`; the program is saved. Recovery is now **339/855 functions**, with 10 library functions delegated and 506 recognized functions remaining. Continue with rectangle transition `0x004272D0`, then recover region synchronization `0x00414220` to close the immediate-transition production seam.

- The runtime resource-state and immediate-transition tranche now implements `SelectRuntimeSceneTransition` (`0x00426D50`), `ApplyImmediateRuntimeSceneTransition` (`0x00426E30`), and `SetRuntimeResourceState` (`0x00425930`). Selection preserves the executable's depth-dependent transition mask, bit-2 suppression outside 8-bpp, constrained `rand() % 3` fallback cycling, and the otherwise-unhandled combined value 3. Resource state preserves bitmap forced refresh, animation flag masking/frame-one transition selection, sound buffer requeue/start/stop behavior, child-state changes, current-resource transition flags, and release ordering.
- Immediate animation transitions preserve full-host update ordering, `Sleep(0)`, optional palette publication, display clip restoration, and the assembly-confirmed acquired-record leak when type bits `0x3000` are absent. Focused tests cover every observed selector branch and fallback, missing/current resources, all resource types, forced and ordinary state changes, display-lock failure, palette/no-palette animation paths, and exact call order. Ghidra names, prototypes, comments, and program are synchronized and saved; Win32 Debug build and CTest pass. Recovery is now **338/855 functions**, with 10 library functions delegated and 507 recognized functions remaining. Continue with the palette transition at `0x00426F40`; rectangle transition `0x004272D0` and region synchronization `0x00414220` remain unresolved and must be recovered directly from Ghidra.

- The teardown scene-publication closure now implements `UpdateRuntimeResourceSceneRegion` (`0x00427900`), `UpdateDisplayRootRegion` (`0x0041B1F0`), `RenderRuntimeBitmapBackendRegion` (`0x0042B140`), and `CopyRuntimeBitmapRegion` (`0x00417370`). It preserves default-scene substitution, lock/acquisition/release order, bitmap-owner flag gates, resource/scene coordinate subtraction, begin/end update behavior, optional root callbacks, clipping, mutex exception cleanup, direct/remapped 8-bit copying, signed bitmap orientation, destination origins/stride, and the executable's exact source-row adjustment.
- Ghidra had falsely split `0x0041B1F0` at `0x0041B1FB` immediately after a NOP despite one shared stack frame and `RET 4`. Deleting the inner boundary and recreating the outer function restored the 132-byte body and reduces the authoritative denominator from 856 to **855**. Tests cover display-lock rejection, root substitution, callback/no-callback and begin failure, missing scenes/resources, both scene publication paths, rectangle arithmetic, backend lookup/type/empty/clipping branches, and direct/remapped top-down/bottom-up bitmap copies. Ghidra is synchronized and saved. Recovery is now **335/855 functions**, with 10 library functions delegated and 510 recognized functions remaining. The next production seam is resource-state controller `0x00425930`; its direct scene-transition dependency begins at `0x00426D50`.

- Runtime-tree teardown is reconstructed through `DestroyRuntimeTreeResources` (`0x00426BD0`) and its reverse-tree lookup cluster at `0x00406860`, `0x00406880`, `0x004068A0`, `0x004097D0`, `0x00409B60`, and `0x0040A500`. The parent preserves active-pointer-root shutdown ordering, the three exact linked-list layouts/traversals, resource flag `0x3000` and link exemption `0x01000000`, local count adjustment, field clearing, comment-mode shutdown, and final count wait. Selector 64 now invokes this recovered function.
- `FinalizeRuntimeResourceDestruction` (`0x00425C40`) is also recovered for bitmap and animation resources, including zero-extended scene coordinates, immediate bitmap count rules, deferred animation flagging/release, shared-resource bit 2, and the 1-ms count wait. Assembly confirms that its non-resource/missing-record paths forward four uninitialized stack slots to `0x00427900`; this edge is preserved in source and documented in Ghidra, but deterministic unit assertions cover only the fully defined bitmap/animation paths. `0x00427900` and resource-state controller `0x00425930` remain isolated production seams.
- `RuntimeTreeNode` is corrected from 0xb8 to **0xbc** with scene-link head/tail `+0x74/+0x78`, primary-resource head/tail `+0x9c/+0xa0`, secondary-resource head/tail `+0xa4/+0xa8`, child/next/previous `+0xac/+0xb4/+0xb8`. Three exact link structures (0x44, 0x50, 0x4c bytes) are asserted and synchronized in Ghidra. Focused tests cover reverse sibling preference/fallback, wrapper misses, null teardown, active-root and ordinary-root orchestration, every defined primary-resource disposition, secondary/scene clearing, count targets, and bitmap/animation destruction branches. Recovery is now **331/856 functions**, with 10 library functions delegated and 515 recognized functions remaining.

- The script-property tranche is complete through `SelectRuntimeResource` (`0x004244E0`), `SetRuntimeScriptProperty` (`0x004202D0`), and `GetRuntimeScriptProperty` (`0x004204B0`). Resource selection preserves critical-section ordering, conditional archive closure/flag and path-byte clearing, parent notification, and exact constructor constants. The setter preserves all 14 observed selectors, ignored context, both reference-counted state transitions and underflow guards, and unsupported no-ops. The getter preserves all ten selectors, the mutable 260-byte path-message round trip, resource data-pointer replacement, scalar/pointer/frame results, and unsupported no-ops.
- Focused tests cover selection cleanup/order; every setter selector and transition boundary; and every getter selector including path mutation and resource-load outputs. The large resource-tree teardown at `0x00426BD0` remains an explicitly isolated unresolved dependency behind selector 64. Ghidra names, prototypes, comments, and program are synchronized and saved. Recovery is now **323/856 functions**, with 10 library functions delegated and 523 recognized functions remaining. Continue by recovering the smallest exact dependency closure of `0x00426BD0`; do not infer behavior from `tools/cdf_extractor`.

- Runtime graphics bootstrap InitializeRuntimeGraphics (0x0041FEA0) now replaces the application initializer's null seam. It preserves inactive/already-active gates, explicit versus current-mode format selection, surface creation and partial-failure retention, all host-context handle/pointer publications, 8/16-555/16-565/24-bit palette cardinalities, mask publication, scene-host/root-node creation, the assembly-confirmed scene field +0x1c copied to context +0x28/+0x2c/+0x30, optional clip reset under a successful display lock, surface clear, display-state reset, thread creation, and final flags 0x600.
- Tests cover every bootstrap failure boundary, current-mode failure/success, already-active and inactive gates, display-lock contention, thread failure, every pixel-depth cardinality, context/descriptor offsets, scene identifier, thread arguments, and side-effect ordering. The very large script-command worker 0x00421530 remains an explicitly labeled diagnostic thread-entry seam and is not counted as recovered.
- The script-property dependency cluster additionally implements SetRuntimePropertyValue (0x004258C0), GetRuntimePropertyValue (0x00425F00), and QueryRuntimeResourceFrameNumber (0x00426080), including null/type/backend gates and unconditional release after acquisition. The setter/getter dispatchers 0x004202D0/0x004204B0 now have corrected fastcall prototypes in Ghidra, but remain unresolved because setter cases call unrecovered resource selection/tree teardown functions.
- Ghidra names, prototypes, the corrected scene-field comment, and program are synchronized and saved. Recovery is now **320/856 functions**, with 10 library functions delegated and 526 recognized functions remaining. Continue through dispatcher dependencies 0x004244E0 and 0x00426BD0, then replace the two diagnostic dispatcher seams.

- `InitializeGraphicsHost` (`0x0041FA00`) and callback slot 0, `InvalidateGameFramebufferRect` (`0x00427830`), are reconstructed. The host preserves its already-active return, GDI batch limit, 0x75f-DWORD state clear, five-stage subsystem chain, both intentional heap lifetimes, exact child class/window parameters, 16-bit width rounding, ignored cursor API results, strict cursor-bound comparisons, display option mask, unconditional target-flag publication, script-root/named-node setup, five critical sections, all 35 callback slots, show ordering, and final flag `0x800`. The state backing explicitly overlays `RuntimeGameHostContext` at graphics-state `+0x458`, so context bpp `+8` aliases the caller-visible graphics result at `+0x460` as in the executable.
- Tests cover every initialization failure stage, success, already-active behavior, both cursor clamping and exact-boundary retention, state/context fields, all callback slots being populated, key newly recovered slots, and framebuffer lock success/failure ordering and rectangle arithmetic. The callback audit also discovered real functions at `0x004202D0` and `0x004204B0`; Ghidra boundaries and their confirmed setter/getter dispatch are recorded, but source remains explicitly diagnostic/unresolved until their transitive operations are recovered.
- Ghidra prototypes/comments are synchronized and saved. The two newly discovered callback functions raise the recognized total to 856; completing the host and framebuffer callback brings recovery to **316/856 functions**, with 10 library functions delegated and 530 recognized functions remaining. Next recover the script property setter/getter pair and their direct dependencies, then proceed to runtime bootstrap `0x0041FEA0`.

- The graphics-host callback-table tranche now implements seven original entry points: media scale update `0x0042B2A0`, animation stop `0x0042A440`, locked media-extension lookup `0x0042B600`, palette application `0x0042B720`, and CDF name/count/index-size accessors `0x00428620`, `0x00428690`, and `0x00428710`. The implementations preserve mutex ordering, recursive acquisition semantics, unchecked CDF indexing, exact `+0x138` index-data result, all 236-entry palette flag writes, foreground/background selection, animate-versus-realize paths, and return values. Focused tests cover found/missing/null paths and every palette update branch.
- Auditing the callback addresses identified six real independently padded functions missed by the original Ghidra analysis; creating those boundaries raises the recognized total from 848 to 854. With the previously completed Windows display enumerator `0x00413030` and this seven-function callback tranche, recovery is now **314/854 functions**, with 10 library functions delegated and 530 recognized functions remaining. Ghidra names, prototypes, comments, and the program are synchronized and saved. `InitializeGraphicsHost` (`0x0041FA00`) remains the active integration target; its complete 35-slot callback table is now mapped.

- Legacy display bootstrap now implements `CollectDirectDrawDisplayMode` (`0x00412db0`), `InitializeDirectDrawRuntime` (`0x00412f40`), `EnumerateDirectDrawDisplayModes` (`0x00412fe0`), and `InitializeDisplayModeHost` (`0x00413380`). DirectDraw loading preserves Win32 platform-ID gates and staged error codes; enumeration preserves cooperative-mode entry/restoration, exact descriptor flag requirements, 0x40-byte mode allocation, duplicate merge keys, 8/16/24-bit pixel-value counts, 5:5:5 versus 5:6:5 masks, unsupported-16-bit rejection, append order, and allocation-failure callback termination. The coordinator preserves flag replacement/retention, Windows-only fallback results, critical-section lifetime, mode selection, and initialized bit 31.
- `DisplayMode` now has confirmed fields through its full 0x40-byte layout: caps `+0x0c`, pixel-value count `+0x14`, dimensions `+0x18/+0x1c`, pixel flags `+0x20`, bpp `+0x28`, RGBA masks `+0x2c..+0x38`, and next `+0x3c`. Focused tests cover every DirectDraw load error, callback filter/merge/format branch, disabled/success/failure enumeration, coordinator initialization/fallback/failure/already-active paths, exact flags, and critical-section cleanup. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build, CTest (2/2), `check-format`, and tab checks pass. Recovery is now 306/848 functions, with 10 library functions delegated and 532 recognized functions remaining. The Windows `EnumDisplaySettingsA` path at `0x00413030` is the final unresolved display-mode dependency before integrating `InitializeGraphicsHost` (`0x0041fa00`).

- Graphics-host dependency recovery now implements `InitializeRuntimeMediaBackend` (`0x00429df0`), `InitializeRuntimeGenericBackend` (`0x00410b70`), `InitializeAsyncFileSubsystem` (`0x00414e10`), `SetScriptRuntimeRootIfValid` (`0x00404970`), and `SetRuntimeNamedNodeEnabled` (`0x00407ea0`). These preserve all existing-initialization gates, the media heap/mutex leak on partial failure, sound-class ordering, generic mutex coverage, async critical-section one-time setup, the `0xffffffff` root sentinel, identity-list traversal, and bit-0-only node flag changes.
- Focused tests cover every failure/success/already-initialized branch, handles and call ordering, root replacement/sentinel behavior, missing and later-list identities, and flag preservation. `RuntimeNamedNode::flags` is now confirmed at `+0x24`. Ghidra names/prototypes/comments and the media-initialized global are synchronized and saved; Win32 Debug build, CTest (2/2), `check-format`, and tab checks pass. Recovery is now 302/848 functions, with 10 library functions delegated and 536 recognized functions remaining. `InitializeGraphicsHost` (`0x0041fa00`) remains active; its next unresolved dependency is display-host initialization `0x00413380`, whose direct display enumeration/DirectDraw dependencies at `0x00413030`, `0x00412f40`, and `0x00412fe0` have been captured from Ghidra but are not yet implemented.

- Top-level application initialization now implements `InitializeGagApplication` (`0x0041f4f0`) instead of the previous always-null diagnostic entry. It preserves zeroed 0x48c-byte allocation, intentional failure leaks, class registration gates, `Gag01.cdf` validation, exact desktop/window/content geometry (including unsigned centering shifts), `FlcAppClassNT`/`GAG` creation, graphics-host custom ABI, second validation and display switch, optional selected-mode pixel format, layout/runtime activation, high-color state update, and `Start.cfg` versus installed-version selection. The command-line argument is confirmed unused.
- Focused tests cover allocation and each of the six later failure exits, selected/default pixel formats, both optional flag paths, 8-bit/high-color behavior, archive result selection, constants, arguments, state fields, and observable call ordering. Ghidra has the corrected pointer return type, confirmed prototype/comment, and is saved. Win32 Debug build, CTest (2/2), `check-format`, and tab checks pass. Recovery is now 297/848 functions, with 10 library functions delegated and 541 recognized functions remaining. Production dependencies `InitializeGraphicsHost` (`0x0041fa00`) and runtime bootstrap `0x0041fea0` remain unresolved and intentionally retain isolated null diagnostic seams; the writable message block beginning at `0x0043f178` is represented only by its confirmed `GAG` prefix pending complete block recovery. Continue with these two direct initializer dependencies.

- Runtime input/window recovery now implements `RuntimeGameWindowProcedure` (`0x004231e0`) and `UpdateRuntimePointerPosition` (`0x004243f0`). The window procedure preserves the ordinal-2 pre-dispatch contract, callback-consumed mouse forwarding, paint/erase handling, input queueing, translated host coordinates, runtime controls `0x7ffc/0x7ffe`, strict `<0x104` result copying, and default dispatch. Pointer updates preserve the recursive scene lock, 5-ms contention retry, host-origin adjustment, scene offset delta, and unconditional global-coordinate publication.
- Ghidra's `0x004231e0` prologue and former `0x004231eb` body were repaired into one function; the false split reduced the authoritative denominator to 848. Focused tests cover callback-consumed and ordinary dispatch, paint/default/character/mouse messages, each runtime-control family, result-size boundary behavior, absent/acquired/contended scene nodes, coordinate adjustment, and lock release. Win32 Debug build and CTest pass; Ghidra names, prototypes, comments, result/pointer globals, and program are synchronized and saved. Recovery is now 296/848 functions, with 10 library functions delegated and 542 recognized functions remaining. Continue through the window/platform call graph from the newly recovered window procedure without using `tools/cdf_extractor` as reverse-engineering evidence.

- Game-DLL command dispatch now implements `StopRuntimeGameDll` (`0x00426210`), `PauseRuntimeGameDll` (`0x00426270`), and `ResumeRuntimeGameDll` (`0x00426290`). Each preserves the loaded-bit gate and exact ordinal-3 fastcall command in ECX (`1`, `2`, or `4`). Stop additionally preserves the initial timestamp after dispatch, loaded-bit-first polling, unsigned DWORD elapsed arithmetic including wrap, strict `<5000` timeout, 10-ms sleeps, and success return even when the loaded bit remains set after timeout.
- Tests cover all unloaded gates, pause/resume command values, stop exit when the DLL clears its loaded flag, exact timeout boundary and sleep count, and tick-count wrap. Assembly confirms the command registers and loop ordering; Ghidra names/prototypes/comments are synchronized and saved. At completion of that tranche, recovery was 294/849 functions, with 10 library functions delegated and 545 recognized functions remaining.

- The main executable's minigame load boundary now implements `LoadAndInitializeRuntimeGameDll` (`0x00426110`). It preserves the lifecycle lock, already-loaded false return, resource-host/path setup, `m_DEF_LOAD` bracketing, LoadLibrary failure path, unconditional ordinal 1/2/3 lookup after a successful module load, retained failed/partial module state, flag `0x10` set plus `0x20` clear only on complete resolution, fastcall initialization with the host context in ECX and callback table in EDX, queue-reset ordering, and final unlock.
- `RuntimeGameHostContext` is confirmed as 0x40 bytes with window `+0`, bpp `+8`, dimensions `+0x20/+0x22`, framebuffer `+0x2c`; the adjacent table at `0x0047f3f8` is exactly 35 pointers (0x8c bytes), matching the finished XTET replacement ABI. Tests cover already loaded, LoadLibrary failure, each independently missing ordinal while still resolving all three, success, exact event order, context contents, all 35 forwarded callback pointers, stored entry points, and flag changes. Ghidra has the structure, typed/named context, callback array, module/ordinal globals, function prototype/comment, and is saved. Recovery is now 291/849 functions, with 10 library functions delegated and 548 recognized functions remaining.

- Constructor-adjacent resource control now implements `RequestRuntimeResourceDestruction` (`0x00425bd0`), `QueryRuntimeResourceFrameLimit` (`0x00425fb0`), `QueryRuntimeResourcePlaybackFlags` (`0x00425ff0`), and `UnloadRuntimeGameDll` (`0x004260b0`). Destruction preserves immediate bitmap/other teardown versus animation flag `0x10000`, exact pending-count exemption bits, and the original lock-release asymmetry. Queries preserve the frame-limit offset and bitmap/animation passthrough versus synthesized sound state. DLL unloading preserves critical-section ordering, ignored `FreeLibrary` result, optional state-0x1000 exit, and exact flag replacement.
- Focused tests cover missing identities, bitmap count decrement/exemption, immediate other-type destruction, deferred animation teardown, frame-limit locking, all sound playback/schedule/loop flag combinations, unloaded DLL no-op, ordinary unload, and state-0x1000 unload ordering. Assembly was checked for all offsets and conditions; Ghidra names/prototypes/comments are synchronized and saved. Recovery is now 290/849 functions, with 10 library functions delegated and 549 recognized functions remaining. Next map `LoadAndInitializeGameDll` (`0x00426110`) only after the complete 0x40-byte host context and 35-entry callback table at `0x0047f3b8/0x0047f3f8` are confirmed; continue other constructor lifecycle helpers in parallel with that mapping.

- The generic text/parser/backend-child tranche is complete. Recovered entry points are `0x004118f0`, `0x004119a0`, `0x00411a20`, `0x0042b6a0`, `0x004122c0`, `0x00411cf0`, `0x00411bc0`, `0x00411560`, `0x00412370`, `0x00411ff0`, `0x00411420`, and `0x00427ab0`. The implementation preserves delimiter-mode parsing, directive scanning, recursive media-lock release, 16x16 glyph-atlas metrics/rendering, generic-text selection and drawing, child-state caching/publication, and the generic-child update callback's visibility/resource paths.
- Focused tests cover parser boundaries and position effects, directive names, recursive release, glyph measurement/drawing/remapping/clipping, text measurement/selection/drawing and control paths, cached child-state behavior, publication state/descriptor copying, and the update callback's identity guard paths. The renderer now explicitly preserves the executable's zero-extension of signed-looking 16-bit destination coordinates. Win32 Debug build, CTest (2/2), `check-format`, and tab checks pass; Ghidra names, prototypes, comments, structures, and program are synchronized and saved.
- Constructor-adjacent recovery also implements the animation resource callback `UpdateRuntimeResourceAnimationBackend` (`0x00427ef0`). Its original priority chain covers destruction/count decrement, suppression, refresh rendering, completion/count acknowledgement, finite/infinite frame-loop handling, palette and dirty-bound updates, generic-child publication, scene completion, and final flag clearing. Focused tests cover suppression, count acknowledgement with the resource exemption, finite loop decrement/reload, loop restart flagging, and unlimited-loop behavior; Ghidra is synchronized and saved.
- Recovery is now 286/849 functions, with 10 library functions delegated and 553 recognized functions remaining. The scene-heavy branches of `UpdateRuntimeGenericBackendChild` and `UpdateRuntimeResourceAnimationBackend` are composition-tested through their recovered dependencies but still need direct callback-level coverage when the required resource-scene fixture is available. Continue into the remaining constructor/script-tree dependencies; do not use `tools/cdf_extractor` as reverse-engineering evidence.

- The constructor dependency tranche now includes the complete generic-backend child accessor cluster at `0x004112b0`, `0x00411340`, `0x00411360`, `0x00411380`, `0x004113a0`, `0x004113c0`, `0x004113f0`, and `0x004114d0`. The confirmed 0xb4-byte child layout has flags `+8`, context `+0x0c`, state `+0x14`, descriptor `+0x9c`, and next `+0xb0`; lock release is the original direct bit clear. Focused tests cover missing identities, exact sentinels, flag mutations, and context/state copies.
- `BlitBitmapWithOptionalPaletteRemap` (`0x0041afa0`) and `RenderRuntimeGenericBackendChild` (`0x00427a30`) are now reconstructed and synchronized. The blit dispatcher preserves source-then-destination locking, raw versus remapped 8-bit paths, 8-to-16 conversion, transparent/opaque selection, temporary 256-entry palette storage, end-update ordering, and first-error return. The render callback uses generic-child state words 6/7 and rectangle words 11..14, resolves context word 1 as an optional display-scene index, and targets the resource scene at offsets relative to resource x/y.
- Win32 Debug build, CTest (2/2), `check-format`, and tab checks pass; Ghidra was re-decompiled and saved after prototype/comment synchronization. Recovery is now 273/849 functions, with 10 library functions delegated and 566 recognized functions remaining. Continue through `0x00427ab0`; its immediate unresolved dependencies are the state publisher at `0x00411420` and the generic-child stream/state builder at `0x00411560` plus its parser call graph. Complete those from Ghidra before integrating constructor `0x00424ec0`.

- Multi-format constructor recovery has begun with an assembly-level audit of `ConstructRuntimeResourceObject` (`0x00424ec0`). Its exact pre-dispatch normalization is now isolated and tested: flags `1/2/4/0x20/0x40`, automatic scene-index modes `1/0x8000/0x80000/0x100000`, forced coordinates `10000,10000`, scene flags `0x40/0x20000/0x08000000`, and the `0x100600` augmentation preserve their original ordering and overwrite behavior.
- The constructor's registered display callback `UpdateRuntimeResourceVisibility` (`0x00428160`) is reconstructed. It preserves fatal/dirty and rectangle-validity gates, palette-state tracking against global bit `0x8000`, the alternating `0xff/0x00` mask copy, context flag `0x80000000`, and the original uninitialized return path when neither callback-mode bit is supplied. Tests cover all defined return paths, palette transitions, rectangle bounds, and exact masked bytes.
- `RuntimeResourceObject` is expanded and asserted at confirmed offsets through its 0x198-byte size: state/backend/type/storage, scene descriptor and identity, positions, output/requested dimensions, animation frame limit/count, prior positions, `+0x74`, and callback position `+0x194`. Ghidra has the matching layout, visibility context, callback name/prototype/comment, and a constructor audit comment documenting the dual-use eighth argument and decompiler-lost animation width assignment. Build, CTest, formatting, and whitespace checks pass. Recovery is now 263/849 functions, with 10 library functions delegated and 576 recognized functions remaining. Continue implementing the constructor's bitmap, sound, animation, generic, script/tree, and CDF branches plus common registration/rollback before considering `0x00424ec0` complete.

- The animation worker tranche is complete through `RunRuntimeAnimationThread` (`0x0042a520`) and `ConfigureRuntimeAnimationBackend` (`0x0042a340`). The worker preserves the nested outer-delay/inner-decode loop, termination/error/refresh/rewind/pause/resume control, signed frame scheduling, memory and streamed frame acquisition, `0xf1fa` validation, exact allocation/error behavior, audio chunk types `0x200/0x300/0x400/0x600`, all mapped FLIC/MVZ decoder dispatch, presentation handshakes, completion/loop notifications, cursor rewinds, and `ExitThread` ordering. Configuration preserves lookup/mutex behavior, default palette/callback pointers, exact descriptor copies, dirty bounds, OR-only flags, unchecked thread creation/handle close, and thread-ID storage at `+0x970`.
- Focused tests cover every extracted worker phase, memory/stream acquisition failures and allocation transitions, all visual dispatch cases, audio initialization/append/start/create paths, streamed ping-pong buffers, timing arithmetic, completion flags and rewinds, worker termination orchestration, and configuration success/failure/default/custom behavior. Ghidra now has exact function names/prototypes/comments, typed head/tail globals, a repaired 0x9cc-byte `RuntimeMediaBackend`, and the confirmed 0xa70-byte animation extension; the program is saved. Win32 Debug build, CTest, `check-format`, and whitespace checks pass. Source recovery is now 262/849 functions, with 10 library functions delegated and 577 recognized functions remaining. Continue from the completed animation backend into `CreateRuntimeResource` (`0x00424ec0`) and its remaining format-dispatch behavior.

- `RunRuntimeAnimationThread` (`0x0042a520`) is being recovered as exact, independently tested phases before the original worker entry is considered implemented. The non-original control helper now preserves termination/error/refresh/rewind/pause/resume/deadline behavior, callback and `0x7ffe` notification ordering, storage-specific rewinds, sound lifecycle calls, and the original flag masks. The refresh callback deliberately leaves notification bit `0x8000` set after clearing refresh bit `0x40`.
- The next extracted worker phase preserves exact frame scheduling: correction begins only after frame 1, signed negative correction uses four-thirds duration, nonnegative correction advances the deadline, corrections at least one duration catch up immediately, flag `0x100000` forces a fixed-duration deadline, and `+0x9b0` adjusts both prior/current deadlines. Focused tests cover each branch. Win32 Debug build, CTest, and `check-format` pass. This preparation adds no completed original entry points, so recovery remains 260/849 functions, with 10 library functions delegated and 579 recognized functions remaining. Continue with frame acquisition/chunk parsing, then integrate and test the complete original worker.

- Animation-wide control now implements `PauseAllRuntimeAnimations` (`0x0042a4a0`) and `ResumeAllRuntimeAnimations` (`0x0042a4b0`). They preserve all unrelated bits while setting/clearing only `0x01000000` in the confirmed global at `0x00442198`; workers test this bit together with their per-backend pause bit. Idempotent and mixed-bit tests pass. Ghidra functions/global are named, typed, commented, and saved.
- The source count is now 260/849 functions, with 10 bundled library functions delegated and 579 recognized functions remaining. Win32 Debug build, CTest, and `check-format` pass. Continue the outer worker state machine using the newly explicit backend layout and global pause control, then implement the frame/audio half and `0x0042a340` configuration.

- Worker preparation replaced the opaque tail of `RuntimeMediaBackend` with confirmed binary fields and assertions: comparison palette `+0x18`, PALETTEENTRY/RGBQUAD/remap storage `+0x20/+0x420/+0x824`, destination descriptor and GDI handles `+0x924..+0x948`, frame/chunk/sound pointers `+0x978..+0x988`, dirty rectangle `+0x98c..+0x998`, frame/timing state `+0x99c..+0x9b4`, callback `+0x9b8`, sound handle `+0x9bc`, buffer capacities and stream `+0x9c0..+0x9c8`. Ghidra's corresponding confirmed tail fields are renamed and saved. Existing Win32 Debug build, CTest, and `check-format` remain green. Recovery remains 258/849; implement the worker against these names next.

- The mapped decoder set is complete with `DecodeRuntimeAnimationDeltaFlc` (`0x00416da0`). It preserves signed line-skip controls, end-of-line pixel controls, per-packet x skips, positive literal word packets, negative repeated-word packets, optional palette remapping, all three horizontal-scale code paths, vertical source-line replay, and incremental min/max dirty bounds rather than replacing them with a full-frame rectangle.
- Tests cover skipped logical lines, EOL pixels, literal and repeated packets, sparse untouched pixels, remapped and direct output, 2x horizontal/vertical scaling, exact scaled-row replay, and preinitialized dirty-bound expansion. Ghidra is named, typed, commented, and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery is 258/849 functions, with 10 bundled library functions delegated and 581 recognized functions remaining. All worker-dispatched FLIC/MVZ functions are now available; proceed with `RunRuntimeAnimationThread` (`0x0042a520`) and `ConfigureRuntimeAnimationBackend` (`0x0042a340`).

- Custom video decoding now implements both `DecodeRuntimeAnimationMvz8` (`0x00415ee0`) and `DecodeRuntimeAnimationMvz5` (`0x00416420`) after matching their control formats to the historical decoder and verifying the executable's computed-jump fragments. The shared non-original implementation preserves MVZ8 packet counts/skips, MVZ5 width termination, fill/literal packets, fixed three-pixel and variable back-references, forward overlap behavior, palette remapping versus direct indices, x/y scaling, area offsets, origins/stride, and scaled dirty bounds.
- Tests exercise all four MVZ encodings, MVZ5 row termination and cross-row back-references, MVZ8 sparse packets, remapped and direct pixels, 2x horizontal/vertical expansion, untouched skipped pixels, offsets, and dirty bounds. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery is 257/849 functions, with 10 bundled library functions delegated and 582 recognized functions remaining. `DecodeRuntimeAnimationDeltaFlc` (`0x00416da0`) is the final decoder leaf before implementing worker `0x0042a520` and configuration `0x0042a340`.

- The first animation-decoder leaf tranche implements `DecodeRuntimeAnimationPalette` (`0x00415e60`), `DecodeRuntimeAnimationLiteral` (`0x00416900`), `DecodeRuntimeAnimationByteRun` (`0x00416ad0`), and the exact single-RET chunk handlers at `0x0042b820`, `0x0042b830`, and `0x0042b840`. Each address was mapped from the worker's chunk switch to COLOR_256, LITERAL, BYTE_RUN, COLOR_64, DELTA_FLI, and BLACK respectively; the first three were then compared against `data/flic_player.cc` and verified against Ghidra assembly before adaptation.
- Decoder tests cover COLOR_256 packet skips, RGB/PALETTEENTRY versus BGR/RGBQUAD byte ordering, zero-count 256-color packets, literal remap/direct paths, x/y scaling, origins/stride, BYTE_RUN positive fill and negative literal packets, vertical repetition, and dirty bounds. The executable differs from the historical decoder in two confirmed ways: COLOR_256 count zero ignores that packet's skip, and chunk types 11/12/13 are all no-ops (including BLACK). Ghidra is synchronized and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery is 255/849 functions, with 10 bundled library functions delegated and 584 recognized functions remaining. Next verify and implement MVZ5/MVZ8 (`0x00416420`/`0x00415ee0`) and DELTA_FLC (`0x00416da0`), then close the worker/configuration pair.

- Animation support now also implements `FailRuntimeAnimation` (`0x0042a4c0`) and `PresentRuntimeAnimationFrame` (`0x0042b850`). Failure preserves error storage, fatal/stop flag ordering, callback suppression, and the `0x7ffe` notification. Presentation preserves both decode handshake acknowledgements, 8-bpp animate-versus-replace palette paths, DIB color publication, dirty rectangles, normal and exact 2x blits, and normal-path dirty-bit clearing. Focused tests cover callback outcomes, both handshake exits, animated/replaced palettes, normal BitBlt, scaled StretchBlt, flags, and call order.
- Ghidra previously held the animation worker and default presenter only as labels. Verified executable entry points were created at `0x0042a520` and `0x0042b850`, adding two real functions to the authoritative denominator. The worker is a 2,958-byte FLIC/audio/timing routine and is now explicitly mapped for the permitted historical-decoder comparison. Ghidra is synchronized and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery is 249/849 functions, with 10 bundled library functions delegated and 590 recognized functions remaining. Next recover the worker's individually addressed FLIC decoders against `data/flic_player.cc`, then implement `RunRuntimeAnimationThread` and `ConfigureRuntimeAnimationBackend` (`0x0042a340`).

- Bitmap publication is complete through `BuildPaletteIndexRemap` (`0x00415d90`), `ConvertRuntimeBitmapToSurface` (`0x00417260`), and `FinalizeRuntimeMediaBackend` (`0x0042b300`). The remapper preserves the byte-wrapping tolerance scan, first-match ordering, and 236/256 candidate split. Conversion preserves palette byte order, original RGBQUAD publication, BMP pixel-offset/padding rules, signed-height orientation, destination origin/stride, remapped and direct-copy paths, and the original return byte. Finalization preserves list lookup, type `0xaa` flag clearing, pending conversion, flag reload, 8-bpp palette realization, DIB color-table update, BitBlt coordinates, output suppression, and mutex ordering.
- Tests cover remap candidate bounds/tolerance/skip, positive and negative bitmap heights, source padding, palette copies, missing and inactive finalizer identities, animation flags, pending conversion, palette and non-palette GDI paths, output suppression, and exact observable call ordering. A false Ghidra split at `0x0042b326` was repaired into the real `0x0042b300..0x0042b4dd` body, reducing the authoritative denominator to 847. Ghidra is named, typed, commented, and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery is 247/847 functions, with 10 bundled library functions delegated and 590 recognized functions remaining. Continue with animation worker `0x0042a520` and configuration `0x0042a340`, then complete the multi-format constructor `0x00424ec0`.

- Multi-format constructor dependency recovery now implements `ConfigureRuntimeBitmapBackend` (`0x0042a290`) and `ConfigureRuntimeResourcePalette` (`0x00427e60`). Bitmap configuration preserves mutex lookup, missing-identity return, flag accumulation, default/custom callback selection, and exact 40-byte transform plus 16-byte descriptor copies to backend `+0x934/+0x924`. Palette configuration preserves primary/non-primary ownership differences, global 236-entry publication, scene 256-entry publication, the `0x04000000` suppression flag, and the exact `bits-per-pixel > 8` override.
- Tests cover missing and second-list identities, default/custom callbacks, OR-only flags, byte-exact copies, failed/successful ownership, 8/16-bpp behavior, primary global-only and global-plus-scene publication. Ghidra functions and confirmed display-depth global `0x0047f3c0` are named, typed, commented, and saved. Win32 Debug build, CTest, and formatting pass. Current source recovery is 244/848 functions, with 10 bundled library functions delegated and 594 recognized functions remaining. Next trace `0x00417260` and its palette-remap dependency before completing bitmap finalization `0x0042b300`; animation configuration `0x0042a340` remains coupled to the worker beginning at `0x0042a520`.

- `CreateSoundHandle` (`0x00401820`) is now reconstructed with its complete lifecycle/slot locking, post-readiness fault check, equal-format allocation, capacity-dependent conversion/rebuild choice, exact waveOut teardown and worker replacement, failure cleanup, and slot initialization. Tests cover every reachable decision cluster, exact WinAPI/helper ordering, fields deliberately left untouched, conversion flags, the old-handle destruction range, and both thread/mixer failures.
- Assembly verification repaired a split Ghidra body: the real function is `0x00401820..0x00401ba1`; the spurious `FUN_0040182b` boundary was merged into it. The terminal allocator scan genuinely accepts handle 1024 and aliases adjacent globals at `0x0043e048`; source explicitly reproduces the confirmed fault/output/thread-id/window-byte side effects.
- The first remaining constructor dependency, `CreateRuntimeGenericBackend` (`0x00410c40`), is also reconstructed. It preserves the zeroed 0x24-byte allocation, ECX/EDX storage at `+0x10/+0x0c`, mutex-protected enabled gate, disabled free, and head insertion. Allocation, disabled, and insertion tests pass. Ghidra is synchronized and saved. The current authoritative Ghidra count is 848 functions; source recovery is 242/848, with 10 bundled library functions delegated and 596 recognized functions remaining. Continue resolving the format-specific dependencies of multi-format resource constructor `0x00424ec0` before implementing the whole function.

- The waveOut mixer tranche now implements `InitializeWaveOutMixer` (`0x00401330`) and all four PCM callbacks: 8-bit mono/stereo (`0x004023c0`/`0x00402770`) and 16-bit mono/stereo (`0x00402b10`/`0x00402f10`). Initialization preserves the exact packed two-block allocation, format arithmetic/copy, callback selection, thread-window and WOM_OPEN readiness waits, cleanup, and fault paths. Mixers preserve original clear widths, conversion arithmetic, fades, saturation thresholds, descriptor advancement/freeing, finite/infinite repetition, and marker writes.
- Focused tests cover initializer gates/failures/success and mixer clear, direct/conversion, pause, suppression, fade, completion, and loop branches. Ghidra prototypes/comments are synchronized and saved; Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 240/847 recognized functions, with 10 bundled library functions delegated and 597 recognized functions remaining. `CreateSoundHandle` (`0x00401820`) is now the active recovery target; its authoritative decompile and complete reconfiguration/slot-allocation control flow have been captured.

- Hidden sound-window recovery now implements `InitializeRuntimeSoundClass` (`0x00401000`), `RunRuntimeSoundThread` (`0x00402100`), and the newly defined `RuntimeSoundWindowProcedure` (`0x004021e0`). The worker creates the exact hidden 580x480 `WS_POPUP` window, raises priority, dispatches every nonzero `GetMessageA` result (including `-1`), clears the window, restores priority, and exits with code 1. Class setup preserves register-class gating and unchecked creation of the lifecycle then slot mutex.
- The sound window procedure preserves WOM_OPEN format/header preparation, the two-pass done-buffer mixer loop for WOM_OPEN/WOM_DONE, mutex ordering, wrapping output index, WM_DESTROY quit posting, WOM_CLOSE destruction, and default dispatch. Exact format copying intentionally leaves `WAVEFORMATEX::cbSize` unchanged. Focused tests cover all branches and ordering; Ghidra gained the previously missing 476-byte function boundary at `0x004021e0`, so the authoritative recognized-function denominator is now 847. Win32 Debug build and CTest pass. Current source recovery count is 235/847 recognized functions, with 10 bundled library functions delegated and 602 recognized functions remaining. Continue with the four mixer callbacks `0x004023c0`, `0x00402770`, `0x00402b10`, and `0x00402f10`, then complete `InitializeWaveOutMixer` (`0x00401330`) and `CreateSoundHandle` (`0x00401820`).

- Sound lifecycle recovery now implements `ShutdownRuntimeSound` (`0x00401190`) and `EnsureRuntimeSoundReady` (`0x004010b0`). Shutdown preserves its disabled return, lifecycle serialization, output-state clearing, double reset/two-header unprepare sequence, callback-thread WOM_CLOSE/join/close, handle destruction range `1..maximum-1`, slot-mutex wait, format cleanup, and the original final `CloseHandle` calls on both mutex handles. Readiness preserves the unusual result contract: once sound is enabled it returns 1 even when thread creation or mixer initialization fails, while performing the exact cleanup and ready-flag transitions.
- Focused tests cover disabled, output-not-ready, threadless and threaded shutdown paths with exact call ordering, plus already-ready, thread-creation failure, mixer failure, and mixer success readiness paths. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 232/846 baseline functions, with 10 bundled library functions delegated and 604 baseline functions remaining. Continue with `InitializeWaveOutMixer` (`0x00401330`) and sound thread `0x00402100`, then close `CreateSoundHandle` (`0x00401820`) and the resource constructor `0x00424ec0`.

- Sound transition/callback support now implements `FadeOutRuntimeSound` (`0x00401de0`), `FadeInRuntimeSound` (`0x00401f10`), `SetRuntimeSoundVolume` (`0x00403310`), and `RuntimeWaveOutCallback` (`0x004035c0`). Sound slots now expose fade block/current/step and transition flags at exact offsets `+0x14/+0x18/+0x1c/+0x2c`.
- Fade tests cover playing/stopped gates, reset fields, transition-bit replacement, normal and byte-truncated long fades, zero-duration step 100, and handle failures. Volume tests preserve clamping and the original invalid-handle mutex leak. Callback tests cover ignored messages and exact WOM_OPEN/CLOSE/DONE forwarding, timestamping, and output-ready clearing. Ghidra layout/prototypes/comments/globals are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 230/846 baseline functions, with 10 bundled library functions delegated and 606 baseline functions remaining. Continue through `InitializeWaveOutMixer` (`0x00401330`) and readiness `0x004010b0`, then implement `CreateSoundHandle` (`0x00401820`).

- Sound format/playback support now implements `RuntimeWaveFormatsEqual` (`0x004012c0`), `CalculateRuntimeWaveConversion` (`0x00403410`), `CleanupRuntimeSoundFormatBuffer` (`0x00401300`), `QueueSoundData` (`0x00401bb0`), `StartSound` (`0x00401cd0`), `StopSound` (`0x00401d50`), `SetRuntimeSoundLoopValue` (`0x00403380`), and `GetRuntimeSoundSlot` (`0x004033e0`). Confirmed 0x34-byte sound slots now expose playing/base/playback/schedule, loop, volume/conversion, and descriptor-head fields; queued descriptors are exact 0x14-byte non-owning PCM references.
- Conversion tests cover exact/unequal formats, bit-depth and channel conversions, equal/2x directions, unsupported ratios with unchanged output, and buffer cleanup. Playback tests cover enable/bounds gates, append/replace queues and free order, start/stop timing resets, active-gated loop values, and fixed-table slot lookup independent of maximum handle. Ghidra structures/names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 226/846 baseline functions, with 10 bundled library functions delegated and 610 baseline functions remaining. Continue with sound subsystem readiness/reconfiguration dependencies of `CreateSoundHandle` (`0x00401820`), then implement it and close `0x00424ec0`.

- Format backend construction now implements `CreateRuntimeBitmapBackend` (`0x0042a1b0`), `GetAsyncFilePosition` (`0x00415aa0`), `SetAsyncFilePosition` (`0x00415a20`), and `CreateRuntimeAnimationBackend` (`0x00429eb0`). Bitmap creation preserves its unused-ECX/custom fastcall ABI, `0x978 + extension` allocation, BM validation, format pointers/words, error flags, and media-list insertion.
- Animation creation preserves separate `0xa58` memory and `0xa70` streamed bases, exact 0x80-byte header copies, AF11/AF12 parsing, memory-relative versus saved-stream-position offsets, stream position restoration only after successful allocation, extension/pointer placement, error flags, scale/frame defaults, and list insertion. Async positioning preserves inclusive ranges, buffer cursor retention/invalidation, and shared-record invalidation. Focused tests, Ghidra types/prototypes/comments, Win32 Debug build, and CTest pass. Current source recovery count is 218/846 baseline functions, with 10 bundled library functions delegated and 618 baseline functions remaining. Continue with sound-handle creation and playback setup dependencies used by `0x00424ec0`, then implement the constructor dispatcher.

- Resource ownership dispatch now implements `DestroySoundHandle` (`0x00402040`), `DestroyRuntimeResource` (`0x00424d80`), and `GetOrCreateRuntimeChildByData` (`0x00407860`). Sound teardown preserves its 1024-slot, 0x34-byte table, reserved zero handle, mutex coverage, queued-buffer freeing, and backward maximum-handle scan. The 0x198-byte resource object is confirmed through backend/type/storage/data/lock/scene fields at `+4..+0x1c`.
- Resource destruction covers generic fallback, bitmap `0x1000`, animation `0x2000` with memory/stream/neither storage, sound `0x8000`, generic backend `0x10000`, unknown classes, the original inverted scene-release success test, current-resource clearing, registry unlinking, and final heap-result folding. Tests also cover data-identity registry reuse/creation. Ghidra types/globals/names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 214/846 baseline functions, with 10 bundled library functions delegated and 622 baseline functions remaining. Continue through format constructors `0x0042a1b0` and `0x00429eb0` and sound creation dependencies before implementing `0x00424ec0`.

- Resource-destruction dependencies now implement `FindRuntimeGenericResource` (`0x004050b0`), `RemoveRuntimeGenericResource` (`0x00405000`), `AcquireRuntimeMediaBackend` (`0x0042b620`), `DestroyRuntimeMediaBackend` (`0x0042b4e0`), `AcquireRuntimeGenericBackend` (`0x00410cc0`), `LockRuntimeBackendChild` (`0x00411220`), `DestroyRuntimeGenericBackendChild` (`0x004112c0`), and `DestroyRuntimeBackendTree` (`0x00410d50`). Confirmed layouts include the generic-resource list at script root `+0xf74`, its destructor callback `+0x814`, 0x9cc-byte media backends, 0x24-byte generic backends, and 0xb4-byte generic children.
- Tests cover missing identities, reference-gated generic removal, recursive/same-thread and contended media ownership, media list unlinking and auxiliary frees, generic parent/child lock-bit contention, child unlink/count accounting, and recursive tree destruction. Ghidra structures/globals/names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 211/846 baseline functions, with 10 bundled library functions delegated and 625 baseline functions remaining. Continue with sound handle destruction `0x00402040`, then close ownership dispatcher `0x00424d80` and constructor `0x00424ec0`.

- Runtime resource release now implements `RemoveRuntimeNamedChild` (`0x00407d50`), `EraseRuntimeChildByDataIdentity` (`0x00407a20`), `ReleaseRuntimeMemoryResource` (`0x00424c50`), `FreeCachedResourcePayload` (`0x00424cc0`), and `ReleaseRuntimeStreamedResource` (`0x00424d30`). Circular child-list head/tail/cursor updates, final-entry clearing, script-heap freeing, low-word cache reference accounting, payload/name lookup variants, async close results, streamed-count guards, and the one-to-zero script flag transition are preserved.
- Focused tests cover multi-entry and final-entry unlink, missing parent/child identities, non-final/final cache release by name and payload, streamed close failure, non-final close, and final close. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 203/846 baseline functions, with 10 bundled library functions delegated and 633 baseline functions remaining. The next active cluster is the format-specific ownership/destruction chain around `0x00424d80`, followed by the multi-format constructor `0x00424ec0`.

- The runtime resource acquisition tranche implements `UpdateRuntimeResourceHost` (`0x00424570`), `DetectRuntimeResourceType` (`0x00424710`), `DuplicateAsyncFileRecord` (`0x00415360`), `OpenRuntimeCdfEntryStream` (`0x00428720`), and `LoadRuntimeResource` (`0x00424870`). It preserves cache reference increments, loose-file and CDF selection, exact 16-byte signature classification/overwrite order, forced/small memory loading, large streamed loading, missing-media retry messages, load-scene activation/reset ordering, allocation/read failures, and the streamed-resource zero-to-one script-flag transition.
- Tests cover cache, loose and archive memory/stream paths, allocation failure, both loose and archive read failure cleanup/reporting, header classification, CDF regular/alternate streams, and notification-driven retry. Ghidra names/prototypes/comments/globals are synchronized and saved; Win32 Debug build and CTest pass. Current source recovery count is 198/846 baseline functions, with 10 bundled library functions delegated and 638 baseline functions remaining. Continue through memory/stream release helpers `0x00424c50`/`0x00424d30` and their exact cache-unlink dependency, then reconstruct the multi-format resource constructor `0x00424ec0`.

- The async buffered-reader cluster now implements `PositionAsyncHost` (`0x00414930`), `SeekAsyncHost` (`0x00414ae0`), `CopyAsyncHostBytes` (`0x00414bb0`), `ActivateAsyncFileRecord` (`0x00414cb0`), `DestroyAsyncFileHost` (`0x00415120`), and `ReadAsyncFileRecord` (`0x00415720`). Both shared-host and record-local read paths preserve exact sector alignment, circular wrap, refill pacing, EOF/failure results, shared-handle invalidation, active-record locking, current offsets, and byte counts.
- Post-typing assembly verification corrected disk geometry: async host `+0x44` is bytes per sector and `+0x48` is sectors per cluster. The constructor passes `+0x48` as `lpSectorsPerCluster` and `+0x44` as `lpBytesPerSector`; all buffer alignment arithmetic uses `+0x44`. Source has explicit offset assertions, and Ghidra now has the complete 0x90-byte structure through active/list pointers `+0x88/+0x8c`.
- Tests cover fresh and saved-buffer positioning, small and locked seeks, wrap copies, activation, direct cached reads, aligned disk refill, read failure, zero-byte EOF, forced shared-host reads, missing identities, and host destruction/unlink/worker/resource cleanup. Ghidra names, prototypes, comments, structure, and program are synchronized and saved. Win32 Debug build, CTest, `check-format`, and whitespace checks pass. Current source recovery count is 193/846 baseline functions, with 10 bundled library functions delegated and 643 baseline functions remaining. Continue with resource-host manager `0x00424570`, then resource type detector `0x00424710` and loader `0x00424870`.

- Async host creation and worker support now implement `AdvanceAsyncHostRead` (`0x004148b0`), `AdvanceAsyncHostWrite` (`0x00414900`), `InvalidateSharedAsyncRecords` (`0x00414a50`), `CreateAsyncFileHost` (`0x00414ec0`), `HandleAsyncHostShortRead` (`0x00415ae0`), and `RunAsyncFileWorker` (`0x00415b70`). The worker entry was recovered from executable bytes after Ghidra had not originally defined it; its verified body is 543 bytes.
- The 0x90-byte host is now confirmed through all offsets: file/read state occupies `+0x4c..+0x6c`, available bytes and capacity are `+0x70/+0x74`, the consumer/producer/secondary cursors are `+0x78/+0x7c/+0x80`, the circular buffer is `+0x84`, and active/list records remain `+0x88/+0x8c`. Constructor tests cover disabled, heap, disk-query, and buffer failures, exact cleanup, original `0xffff` then sector-size rounding, list insertion, default mode `-1`, and worker-thread wiring. Accounting tests cover sector return, cursor wrap, shared-record invalidation, both short-read branches, and immediate worker termination.
- Ghidra names, prototypes, comments, and the newly created worker boundary are synchronized and saved. Win32 Debug build, CTest, `check-format`, and whitespace checks pass. Current source recovery count is 187/846 baseline functions, with 10 bundled library functions delegated and 649 baseline functions remaining. Continue with async positioning/copy helpers `0x00414930`, `0x00414ae0`, `0x00414bb0`, `0x00414cb0`, then implement buffered reader `0x00415720`.

- Async file record lifecycle now implements `OpenAsyncFileRecord` (`0x00415230`) and `CloseAsyncFileRecord` (`0x00415420`). Open acquires the host, calls `CreateFileA` with `GENERIC_READ`, `FILE_SHARE_READ`, `OPEN_EXISTING`, and `0x20000000`, allocates an exact zeroed 0x40-byte record plus a 0x8000-byte committed/reserved read-write buffer, initializes range/state fields, inserts at host `+0x8c`, and releases the host. End offset zero means file size; remaining size is end minus start without validation.
- Close spins on record bit `0x10000`, unlinks under the global lock, clears host active record `+0x88` under nested secondary/primary locks, and handles flag `0x2` shared handles exactly. One surviving same-handle sibling loses `0x2`; any survivor suppresses destruction. With no survivor it combines `CloseHandle`, `VirtualFree(buffer,0,MEM_RELEASE)`, and `HeapFree` results with the successful-removal value.
- Tests cover missing host/identity, invalid file, heap failure, virtual-buffer failure and cleanup ordering, default/explicit end offsets, every initialized record field, list insertion, active clearing, full destruction, and retained shared siblings. Ghidra names/prototypes/comments remain synchronized and saved; Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 181/846 baseline functions, with 10 bundled library functions delegated and 655 baseline functions remaining. Continue with async host constructor/destructor and buffered read internals before closing resource load `0x00424870`.

- The asynchronous file ownership cluster now implements `AcquireAsyncFileHost` (`0x00415040`), `ReleaseAsyncFileHost` (`0x004150d0`), `AcquireAsyncFileRecord` (`0x004155c0`), `ReleaseAsyncFileRecord` (`0x00415690`), `SetAsyncFileHostMode` (`0x00415210`), and `GetAsyncFileSize` (`0x00415ac0`). Confirmed structures are the 0x90-byte host with self/flags/next/mode `+0/+4/+8/+c`, active/file-list `+0x88/+0x8c`, and the 0x40-byte file record with self/flags/next/handle/size at `+0/+4/+8/+c/+0x10` and host `+0x3c`.
- Host and file acquisition use a global enable gate, global critical section, bit `0x10000`, and exact `Sleep(0)` contention retry. File flag `0x2` extends acquisition/release to every sibling in the same host sharing handle `+0x0c`. Host release intentionally lacks the global-enabled check; file release retains it. Host-mode zero leaves `+0xc` unchanged, while file-size query always releases after reading.
- Tests cover disabled/missing, normal and contended host/file acquisition, lock-call counts, `Sleep(0)`, shared-handle propagation and unrelated-handle exclusion, release, zero/nonzero host modes, and locked size access. Ghidra structures/globals/names/prototypes/comments are synchronized and saved; Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 179/846 baseline functions, with 10 bundled library functions delegated and 657 baseline functions remaining. Continue through async host creation/open/read/close (`0x00414ec0`, `0x00415230`, `0x00415720`, `0x00415420`) required by resource loader `0x00424870`.

- Loose-resource path support now also implements `ExtractRuntimeDrivePrefix` (`0x00414dd0`) and `OpenRuntimeResourceFile` (`0x0042b6b0`). Drive extraction copies at most 12 bytes and returns success only for a colon terminator, copying the colon plus following source byte and adding NUL; non-colon exits intentionally leave the destination unterminated. File opening calls `GetVersionExA`, uses `GENERIC_READ`, `FILE_SHARE_READ`, `OPEN_EXISTING`, and flags `0x28000000` only for `VER_PLATFORM_WIN32_WINDOWS`, otherwise `0x08000000`; `INVALID_HANDLE_VALUE` becomes null.
- Tests cover drive, backslash, and unterminated-result behavior plus exact Win95/98 versus NT `CreateFileA` arguments and result conversion. Ghidra names/prototypes/comments are synchronized and saved; Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 173/846 baseline functions, with 10 bundled library functions delegated and 663 baseline functions remaining. Continue with asynchronous file-cache host functions used by `0x00424570`, then close resource type detector `0x00424710` and loader `0x00424870`.

- Resource-constructor leaf recovery now implements `BuildRuntimeResourcePath` (`0x004246b0`), `FindRuntimeResourceCacheEntry` (`0x00407720`), `GetOrCreateRuntimeResourceCacheEntry` (`0x00407780`), and `AppendRuntimeNamedChild` (`0x00407d10`). Path construction uses two exact 0x80-byte locals, preserves an explicit source directory, and otherwise prefixes the confirmed 260-byte fallback at `0x0047f070`.
- Runtime resource cache entries are confirmed as 0x34 bytes: 32-byte inline name, data `+0x20`, size `+0x24`, flags/reference count `+0x28`, and circular next/previous `+0x2c/+0x30`. Runtime named parents use count/head/tail/cursor at `+0x40/+0x44/+0x48/+0x4c`. Creation allocates zeroed memory from root heap `+0x81c`, copies exactly eight DWORDs of name storage, and appends into the circular doubly linked list; there is no allocation-null guard before the copy in the original.
- Tests cover explicit/fallback path selection, head/sentinel lookup, duplicate reuse, missing parent, exact allocation size/flags, first-entry self links, second-entry circular splicing, and count/head/tail/cursor changes. Ghidra types/names/prototypes/comments are synchronized and saved; Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 171/846 baseline functions, with 10 bundled library functions delegated and 665 baseline functions remaining. Continue through resource type detection `0x00424710`, data acquisition `0x00424870`, and their remaining file/archive/cache dependencies before reconstructing multi-format constructor `0x00424ec0`.

- The pointer-region/comment propagation cluster is complete: `EnqueueRuntimeEventRecord` (`0x0040c3d0`), `SelectPointerRegionScene` (`0x004237f0`), `UpdateRuntimePointerRegion` (`0x00423fa0`), `DeactivateDefaultCommentScene` (`0x00423660`), `DeactivateRuntimeTreeNodeComment` (`0x00423710`), and `SetRuntimeTreeCommentMode` (`0x00426320`). This closes the activation/deactivation dependency chain without a production callback substitute.
- Confirmed layouts include the 32-by-0x40 event ring at runtime root `+0x0c` with read/write indices `+0x80c/+0x810`; 0x28-byte scene slots at `0x004803a4` with visual pointer `+0` and restriction flag byte `+6`; 0x5c-byte pointer regions with link `+0x24`, inclusive bounds `+0x2c..+0x38`, scene-mask state `+0x40..+0x48`, priority `+0x4c`, visual override `+0x50`, and state object `+0x58`; and runtime-tree default visual `+0x70`.
- Tests cover ring copy/wrap/overwrite behavior, scene-bit rotation and state-mask restrictions, inclusive overlapping hit tests with unsigned priority selection, unchanged-selection suppression, runtime/default-comment gates, slot and fallback scene selection, `0x10000`/`0x20000`/`0x30000` mode transitions, event emission, no-hit restoration, deactivation identity matching, and root-plus-post-order descendant propagation of node flag `0x8000`. Ghidra structures/globals/names/prototypes/comments are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 167/846 baseline functions, with 10 bundled library functions delegated and 669 baseline functions remaining. Continue with refresh routine `0x00426700` through resource release/create helpers `0x00425bd0`, `0x00425c40`, and `0x00424ec0`, then implement the refresh orchestration itself.

- The runtime comment-scene activation half now implements `FindRuntimeVisualObject` (`0x00409330`), `ActivateDefaultCommentScene` (`0x004235e0`), and `ActivateRuntimeTreeNodeComment` (`0x004236e0`). The distinct visual-object chain is confirmed at script root `+0xf80`; each 0x15c-byte object has inline name `+0`, next `+0x24`, and scene identity `+0x158`. The fixed activation name at `0x00442134` is exactly `m_DEF_COMMENT`.
- Activation preserves all `g_dwRuntimeSceneControlFlags` gates: bits `0x20008` prohibit entry, `0x100000` is required, the current scene identity must be non-null, missing lookup returns `-1`, success returns `1`, and bit `0x80` suppresses a redundant switch only when the saved identity also matches. A non-root runtime tree node sets bit `0x8` only after a positive activation result. Focused tests, Win32 Debug build, and Ghidra synchronization pass.
- Current source recovery count is 161/846 baseline functions, with 10 bundled library functions delegated and 675 baseline functions remaining. The paired deactivation path `0x00423660`/`0x00423710` reaches pointer-region routine `0x00423fa0`; recover that actual routine and its data model before completing `0x00426320`, without a production no-op seam.

- Runtime scene/resource helpers now implement `QueryRuntimeSceneFlags` (`0x00425fd0`), `WaitForRuntimeResourceCount` (`0x00426d20`), and `UpdateRuntimeScenePosition` (`0x00425eb0`). The scene record is confirmed through size 0x60: scene identifier `+0x1c`, current x/y `+0x38/+0x3c`, offsets `+0x48/+0x4c`, and previous x/y `+0x58/+0x5c`. Position updates snapshot the old pair, store the new pair, offset the display scene by the exact delta, and release the acquired record. Resource waiting compares for exact equality and calls `Sleep(0)` until it matches.
- Runtime tree traversal now implements `BeginRuntimeTreeEnumeration` (`0x00406770`) and `GetNextRuntimeTreeNode` (`0x004067f0`). The 0xb8-byte node additionally has parent `+0x24` and root-local iterator current/ascent fields `+0x64/+0x68`. Traversal is post-order below the selected root, with sibling advancement, parent ascent, and the original `-1` parent sentinel stop. Focused tests and the Win32 Debug build pass; Ghidra types, names, prototypes, and comments are synchronized and saved.
- Current source recovery count is 158/846 baseline functions, with 10 bundled library functions delegated and 678 baseline functions remaining. Next continue through plan-flag propagation `0x00426320` and its activation/deactivation chain before returning to refresh routine `0x00426700`.

- Runtime tree lookup now implements `FindRuntimeTreeNode` (`0x0040cd60`) and its global-root wrapper `ResolveGlobalRuntimeTreeIdentity` (`0x004065e0`). The 0xb8-byte tree node has identity `+0x20`, child `+0xac`, and sibling `+0xb4`; the script-runtime tree root is confirmed at `+0xf78`. Search ordering is current node, complete child subtree, then next sibling.
- Tests cover root, child, sibling, missing, and null-root lookups. Ghidra type/names/prototypes/comments are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 153/846 baseline functions, with 10 bundled library functions delegated and 683 baseline functions remaining. This lookup is the first dependency of refresh routine `0x00426700`; continue through its plan update/resource helpers.

- The script scheduler plan-state pair implements `SetRuntimePlansInactive` (`0x0040a7a0`) and `ClearRuntimePlansInactive` (`0x0040a800`). The runtime plan node is confirmed through `+0x28` with next pointer `+0x24` and flags `+0x28`; script-root plan head and inclusive terminal are at `+0xf8c/+0xfa8`.
- Tests cover root flag `0x20`, mixed preexisting node bits, changed/no-change results, inclusive terminal stopping, nodes after the terminal remaining untouched, and null-terminal behavior. Ghidra type/names/prototypes/comments are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 151/846 baseline functions, with 10 bundled library functions delegated and 685 baseline functions remaining.
- These functions are the direct state mutations behind script-thread transition helper `0x00421130`; its remaining dependency is the substantial refresh routine `0x00426700`, which is being decomposed through its plan/resource helpers rather than replaced with a test-only callback.

- The runtime-display shutdown tranche implements `GetOrCreateRuntimeNamedNode` (`0x00407690`) and `ShutdownRuntimeDisplay` (`0x00420130`). Runtime named nodes are confirmed as exact zeroed 0x50-byte allocations with self identity at `+0x20`, next at `+0x2c`, status at `+0x40`, child fields at `+0x44/+0x48`, and runtime-root heap/list fields at `+0x81c/+0xf84`.
- Shutdown preserves the `0x600`/`0x400` gates, exact `MMediaObjectsList` and `OpenMemoryFilesList` status checks, termination-bit timing, wait/close ordering, boolean combination of scene and host teardown, unconditional surface teardown after a successful close, and success-only clearing of 10 backend DWORDs, 8 format DWORDs, pointers, and flags `0x600`. Tests cover inactive, partial-active, busy-list, close failure, downstream failure, and complete cleanup.
- Ghidra contains the expanded runtime node, names/prototypes/comments, and is saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 149/846 baseline functions, with 10 bundled library functions delegated and 687 baseline functions remaining. The shutdown half of the runtime display lifecycle is complete; continue with initializer `0x0041fea0`, using a declared script-thread entry while the full `0x00421530` gameplay interpreter remains its own later milestone.

- The runtime scene/reset tranche implements `FindRuntimeNamedChild` (`0x00407810`), `AcquireRuntimeLockRecord` (`0x00425f10`), `ReleaseRuntimeLockRecord` (`0x00425fa0`), `SwitchRuntimeScene` (`0x004242c0`), and `ResetRuntimeDisplayState` (`0x004262b0`). Confirmed types include the 0x4c runtime named node, 0x1c recursive lock prefix, and 0x50 runtime scene record; the script root's runtime-node chain is at `+0xf84`.
- Tests cover parent/child/sentinel lookup, first/reentrant/contended/missing lock acquisition, non-underflowing release, old-scene parking at `(10000,10000)`, both `0x1000`/`0x2000` selection modes and context flags, relative offsets, release ordering, exact graphics mask `0xff7c3e43`, script/queue reset ordering, owner-wide scene release, and the exact 21-DWORD clear. Ghidra types/names/prototypes/comments are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass.
- Current source recovery count is 147/846 baseline functions, with 10 bundled library functions delegated and 689 baseline functions remaining. The surface and runtime-reset dependencies of `0x0041fea0` are complete; next resolve the script execution thread at `0x00421530` and the remaining runtime context globals, then implement the full runtime display initialize/shutdown pair.

- `CreateDisplaySurface` (`0x004139b0`) is implemented with both original backends. Confirmed types are the 0x18-byte source pixel format and 0x6c-byte DirectDraw surface descriptor with pixel format at `+0x48` and caps at `+0x68`. DirectDraw preserves flip-chain attempt, attached back-buffer acquisition, cooperative fallback, primary/offscreen descriptors, sentinel `-1` result, and mode bits `0x10/0x110`. GDI preserves the top-down 0x428-byte bitmap-info path, 8/16/32-bit compression/color-count decisions, masks, logical palette, DIB colors, pixel result, and asymmetric failure cleanup.
- Tests cover offscreen DirectDraw success, flip success, flip failure with offscreen fallback, exact descriptors, 8-bit GDI palette/DIB creation, RGB mask publication, 5-6-5 and 5-5-5 16-bit headers, and 32-bit bitfields. Ghidra contains all three exact structures, the fastcall prototype, and behavior comment and is saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 142/846 baseline functions, with 10 bundled library functions delegated and 694 baseline functions remaining.
- The surface backend needed by runtime display initialization is now source-complete. Continue through `0x004262b0` and the script-thread/reset dependencies before implementing the full `0x0041fea0`/`0x00420130` lifecycle pair.

- `OperateDisplaySurface` (`0x004140b0`) is implemented for both original backends. DirectDraw mode 1 uses the `+0x1c` BltFast slot with inclusive bounds and flag `0x10`; mode 2 uses the `+0x14` Blt slot with the same source/destination RECT, flag `0x400`, and an exactly zeroed 100-byte effects block whose size field is 100. GDI mode 1 uses `BitBlt(..., SRCCOPY)` and mode 2 uses `PatBlt(..., BLACKNESS)`; other modes only acquire/release the display lock.
- Tests cover both DirectDraw modes, effects zeroing, inclusive rectangle construction, both GDI modes, and ignored modes. Ghidra name/prototype/comment are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 141/846 baseline functions, with 10 bundled library functions delegated and 695 baseline functions remaining. `CreateDisplaySurface` (`0x004139b0`) is the next function and now has all surrounding cooperative, teardown, accessor, and operation contracts recovered.

- The DirectDraw cooperative-mode dependency tranche implements `FindTopLevelDisplayWindow` (`0x00413340`) and `SetDisplayCooperativeMode` (`0x00413590`). Parent traversal follows `WS_CHILD` through `GetParent`; cooperative transitions call the original IDirectDraw vtable slot `+0x50` with flags `8` or `0x15`, mutate state bit `0x1000` only on HRESULT zero, return `0x200000` only for a missing display window, and otherwise return zero even when the COM transition fails.
- Tests cover nested/top-level windows, the missing-window gate, enter/exit success, enter no-op, and failed exit without flag mutation. Ghidra names/prototypes/comments are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 140/846 baseline functions, with 10 bundled library functions delegated and 696 baseline functions remaining. Continue implementing `CreateDisplaySurface` (`0x004139b0`) with both its DirectDraw and GDI branches, using the now-recovered cooperative helper and palette teardown.

- The runtime-display dependency tranche implements `TeardownDisplayPaletteSurface` (`0x00413f80`) and `ResetScriptRuntimeTransientIndices` (`0x0040c370`). Palette teardown preserves the presentation-busy wait/recheck loop, critical-section ownership, conditional palette and DIB cleanup order, exact global clearing, and unconditional final unlock. The script helper confirms runtime-root DWORD fields at `+0x80c/+0x810` and clears them only when the global runtime exists.
- Tests cover the busy wait, initialized GDI cleanup and object identity/order, cleared handles/flags, nullable script runtime, and both transient fields. Ghidra names/prototypes/comments and the expanded script-runtime structure evidence are synchronized and saved. Win32 Debug build, CTest, and `check-format` pass. Current source recovery count is 138/846 baseline functions, with 10 bundled library functions delegated and 698 baseline functions remaining.
- `0x0041fea0` and `0x00420130` are now fully traced at the call-graph level. Their remaining unresolved production dependencies include the display-surface creator/clear path (`0x004139b0`/`0x004140b0`), runtime reset `0x004262b0`, and the script execution thread. Continue resolving those exact ABIs rather than installing placeholder lifecycle behavior.

- The display-resource accessor tranche implements `GetDisplayPaletteDc` (`0x00413480`), `GetDisplayPaletteDibDc` (`0x00413490`), `GetDisplayPaletteBitmap` (`0x004134a0`), `GetDisplayPaletteHandle` (`0x004134b0`), and `GetDisplayPaletteEntries` (`0x004134c0`). These exact global accessors are dependencies of the runtime display initializer at `0x0041fea0`; tests verify identity without substituting derived resources. Ghidra names/prototypes/comments are synchronized and saved. Current source recovery count is 136/846 baseline functions, with 10 bundled library functions delegated and 700 baseline functions remaining.

- The display-host lifecycle tranche implements `InitializeDisplaySceneHost` (`0x004192b0`), `ShutdownDisplaySceneHost` (`0x004194b0`), and `RunDisplaySceneWorker` (`0x0041b3f0`). Initialization preserves the 0x21-DWORD state reset, manual-reset event creation and failure ordering, root acquisition, optional worker creation, clip bounds, and pending-rectangle sentinel. Shutdown preserves the original active-bit clear before root-release call, including the observable interaction with `ReleaseDisplaySceneNode`'s active gate.
- The worker preserves dirty-rectangle synchronization, conditional root/child callbacks, publish/release ordering, palette mode-2 then frame mode-1 notification, interval pacing, and once-per-second dirty-frame accounting. Narrow host and worker seams cover initialization failure/success, optional thread ABI/parameter, shutdown/wait behavior, clean worker release, and exact sleep remainder. Win32 Debug build, CTest, and `check-format` pass.
- Ghidra contains verified boundaries, names, calling conventions, prototypes, and behavior comments for all three functions and is saved. Current source recovery count is 131/846 baseline functions, with 10 bundled library functions delegated and 705 baseline functions remaining. Continue from the lifecycle callers into the remaining display/platform wrappers and startup initializer dependencies.

- The scene allocation/format tranche implements `BuildIndexedTo16Palette` (`0x0041c8c0`), `BuildIndexedToIndexedPalette` (`0x0041ca00`), `ConfigureDisplayScenePalette` (`0x0041aa10`), `ConfigureDisplaySceneFormat` (`0x00418ee0`), and the complete `AcquireDisplaySceneNode` (`0x00419bc0`). The node layout is now confirmed at its full `0xa9c` allocation size, with a 256-DWORD palette source at `+0x29c` and 256-DWORD mapping at `+0x69c`; the exact eight-DWORD `DisplayPixelFormatDescriptor` is also defined.
- Palette tests cover format gates, 8-to-16 channel masks and rounding, indexed threshold matching, direct palette-table installation, root/child 8-bit and 16-bit callback selection, palette publication, dependent child conversion, and palette clearing. Allocator tests cover inactive and descriptor gates, node/pixel allocation failures, `HEAP_ZERO_MEMORY`, sorted new-node insertion, pixel zeroing, owner descriptors, shared owners, duplicate rejection, ownerless reference acquisition, destructive resize success, resize allocation failure and removal, root sentinel setup, and default root fill selection.
- Ghidra contains the full node/format types, scene index, renamed palette builders/configurators/allocator, exact fastcall prototypes, behavior comments, scene-count and palette-state globals, and a saved re-decompilation of the allocator with named fields. Current source recovery count is 128/846 baseline functions, with 10 bundled library functions delegated and 708 baseline functions remaining. Continue through remaining display-scene wrappers/worker lifecycle and then return to the startup initializer callers now that scene allocation, update, rendering, and teardown are source-complete.

- The compositor/scene-release tranche implements `CompositeTransparent8To8` (`0x0041b9d0`), `CompositeOpaque8To8` (`0x0041bc40`), `CompositeTransparentIndexedTo8` (`0x0041bee0`), `CompositeOpaqueIndexedTo8` (`0x0041c180`), `CompositeTransparentIndexedTo16` (`0x0041c400`), `CompositeOpaqueIndexedTo16` (`0x0041c660`), and `ReleaseDisplaySceneNode` (`0x0041a480`). The six original compositor entry points retain a shared non-original internal clipping/loop helper while preserving their distinct transparency and conversion policies.
- Compositor tests cover null/flag gates, local and global-coordinate clipping, destination rectangle mutation only in local mode, zero-index transparency versus opaque copying, direct 8-bit copying, palette low-byte conversion, palette low-word conversion, byte strides, and clipped pixel outputs. Release tests cover invalid/inactive gates, swap-removal from the owner array, primary-owner clearing, last-owner callback selection and metadata clearing, reference decrement, root-sentinel preservation, shutdown release, dirty bounds, unlinking, and exact callback/alternate/current/primary/node free order.
- Ghidra contains synchronized names, seven-argument fastcall prototypes, behavior comments, `g_nDisplaySceneCount`, and the fully re-decompiled teardown showing the named callback assignments; the program is saved. Current source recovery count is 123/846 baseline functions, with 10 bundled library functions delegated and 713 baseline functions remaining. The next lifecycle target is the large scene allocation/reuse routine at `0x00419bc0`, whose teardown, callback, heap, metadata, and list contracts are now established.

- The scene query/update/callback tranche implements `QueryDisplaySceneByIndex` (`0x0041ae60`), `OffsetDisplaySceneNode` (`0x0041af20`), `BeginDisplaySceneUpdate` (`0x0041b280`), `EndDisplaySceneUpdate` (`0x0041b360`), `AddDisplaySceneCallback` (`0x0041a830`), `FillDisplaySceneRectangle8` (`0x0041b950`), and `FillDisplaySceneRectangle16` (`0x0041be60`). Tests cover found/missing query output and exact zeroing, metadata copying, movable/immovable offsets, begin/end busy accounting and event transitions, first/second/already-present surface buffers, inline callback contexts, `0x10000` buffer bypass, callback/buffer allocation failures with rollback, and clipped byte/word surface fills including null and empty inputs.
- `DisplaySceneNode` is confirmed through `+0x29b`: the eight-DWORD callback metadata begins at `+0x27c`, so the source slice is now 0x29c bytes. `DisplaySceneDescriptor` is confirmed as 0x10 bytes with signed 16-bit x/y/width/height, 16-bit present/reserved fields, and a 32-bit pixel position. A narrow heap seam preserves production `GetProcessHeap`/`HeapAlloc`/`HeapFree` while allowing exact allocation-order and failure tests.
- Ghidra contains the expanded node type, descriptor, names, prototypes, and comments for this tranche and is saved. Current source recovery count is 116/846 baseline functions, with 10 bundled library functions delegated and 720 baseline functions remaining. Immediate work is mapping the remaining format-specific blitters selected by the release/teardown routine at `0x0041a480`, then recovering that routine using the confirmed reference, owner, callback-list, buffer, busy-gate, and heap contracts.

- The adjacent display-scene control tranche implements `FindAvailableDisplaySceneIndex` (`0x00419550`), `WaitForDisplaySceneReady` (`0x00419600`), `SetDisplayClipRectangle` (`0x00419660`), `ReleaseDisplayLockMode1000` (`0x00419b60`), `LockDisplaySceneNode` (`0x0041acc0`), `UnlockDisplaySceneNode` (`0x0041ad50`), and `SetDisplayScenePrimaryOwner` (`0x0041adc0`). Tests cover inactive/owned gates, consecutive-index skipping, ready publication, clip validation/clamping/clearing, mode-`0x1000` recursion behavior, recursive same-thread node locks, missing nodes, owner membership, replacement, and clearing.
- `DisplaySceneNode` now has confirmed reference count `+0x08`, lock count `+0x0c`, lock-owner thread `+0x10`, owner count `+0x68`, primary owner `+0x6c`, and 128 owner identifiers at `+0x70`. These fields, all seven names/prototypes/comments, and the earlier synchronization tranche are saved in Ghidra. Current source recovery count is 109/846 baseline functions, with 10 bundled library functions delegated and 727 baseline functions remaining. The large allocation/reuse routine at `0x00419bc0` remains unresolved and must be recovered as its own tested cluster rather than simplified from the surrounding helpers.

- The display-scene synchronization tranche implements `MergeDisplayRectangle` (`0x0041b6f0`), `QueueDisplayRectangle` (`0x004195b0`), `SynchronizeDisplaySceneNode` (`0x004190d0`), `PublishDisplaySceneNode` (`0x00419230`), and `DispatchDisplaySceneUpdate` (`0x00419710`). Inspection of the preceding `INT3` alignment and complete prologue corrected Ghidra's malformed overlapping entry at `0x0041971b`; the true dispatcher begins at `0x00419710`.
- Confirmed binary-facing additions are the 0x08-byte `DisplayRectangleTransform`, the 0x10-byte `DisplaySyncRequest`, and a 0x280-byte `DisplaySceneNode` with secondary position `+0x28`, callbacks `+0x270/+0x274`, callback position `+0x278`, and callback state `+0x27c`. The global synchronization callback uses a mode-dependent payload: `DisplaySyncRequest *` for modes `0x10000`/`0x20000`, and raw `DisplayRectangle *` for mode 1.
- Tests cover rectangle union, signed low-word transforms and clipping; queue gates and locking; synchronization failure, unchanged, position-only, and dimension-change paths; root/node request identity; callback modes and ordering; registered-node accumulation; invalid rectangles; mode-1 suppression; and the exact ten-failure retry limit with nine 5 ms sleeps. Win32 Debug builds of `gag` and `gag_startup_test`, both CTest tests, and `check-format` pass.
- Ghidra contains synchronized boundaries, names, prototypes, structures, globals, and comments for this tranche. Current source recovery count is 102/846 baseline functions, with 10 bundled library functions delegated and 734 baseline functions remaining. Continue through adjacent display-scene registration/teardown and callers while preserving the mode-dependent callback ABI.

- The runtime display/target tranche now implements `EnableDisplayPaletteMode` (`0x00414590`), `DisableDisplayPaletteMode` (`0x004145d0`), `ApplyDisplayPalette` (`0x00414610`), `EndDisplayTarget` (`0x00414540`), `AcquireDisplayLock` (`0x004198e0`), `ReleaseDisplayLock` (`0x00419af0`), and `UpdateRuntimeTarget` (`0x004280d0`). Production `ProcessRuntimeMessage` now uses the recovered acquisition, update, and release sequence.
- The acquisition dependency cluster implements `ProcessSceneNodeCallbacks` (`0x0041b560`), `ConstrainDisplayRectangleToSurface` (`0x0041b640`), `ClipDisplayRectangle` (`0x0041b690`), `TrimDisplayRectangleOverlap` (`0x0041b790`), `AccumulateSceneNodeRectangle` (`0x0041b860`), and `ContainsDisplaySceneNode` (`0x0041ac70`). Confirmed structures are the 0x10-byte rectangle, 0x24-byte traversal state, 0x14-byte callback node, and the recovered 0x27c-byte scene-node slice.
- Tests cover disabled, first, recursive, busy-gate, foreign-owner 0x3000, and 0x2000 acquisition paths; event reset/wait/sleep ordering; pending-rectangle consumption; primary/secondary flags; callback one/two-pass behavior and 0x10 termination; signed clipping; overlap trimming; geometry accumulation; and locked scene lookup. Palette tests retain the distinct 236-entry logical-palette and 256-entry DIB behavior.
- Ghidra contains synchronized names, prototypes, structures, comments, and confirmed globals for this tranche, and the program is saved. Current source recovery count is 97/846 baseline functions, with 10 bundled library functions delegated and 739 baseline functions remaining. The malformed function boundary currently shown at `0x0041971b` must be resolved from its preceding instructions before recovering the adjacent display-scene update dispatcher; do not trust its current phantom ZF/stack inputs.

- The runtime-session tranche now implements `InitializeRuntimeInputSession` (`0x00420790`) with its confirmed ECX/EDX plus five-stack-argument ABI and `RET 0x14`. It preserves the active-object no-op gate, byte-ring reset, record/status/value/width/time seeding, nullable selector lookup, preparation gate, `0x80000` resource allocation, context flag `0x04000000` alternate-resource selection, descriptor `+0x27c`, activation-result cleanup split, release ordering, and final graphics flag `0x100`.
- `RuntimeInputContext` is confirmed as 0x20 bytes with context value `+0x04`, flags `+0x0c`, and resource context `+0x1c`. Tests cover already-active, preparation failure with and without a context, missing descriptor, activation success, activation-zero cleanup/configuration, normal/alternate resource selection, default character width `0x20`, and exact callback ordering.
- `RunRuntimeCommandLoop` (`0x00420ce0`) is implemented with state flags at `+0x930`: its entry mask `0x03000040`, saved/restored `0x100000` bit, startup message `0x60000000`, repeated script-state message `0x70000000`, 100 ms polling, cancellation message `0x80000000`, completion message `0x90000000`, cleanup ordering, and graphics-host `0x02000000` clear. Tests cover the inactive return, cancellation, immediate completion, and multi-iteration completion paths.
- `ProcessRuntimeMessage` (`0x00421230`) is implemented and bound as the production loop's processor. It consumes one message, handles only `0x30f`/`0x311` by clearing/setting state flag `0x40000`, calls their distinct handlers, and conditionally updates target `+0x458` with exact bounds `{0,0,width,height}` only when the three-zero-argument query returns zero. Tests cover empty, ignored, both handled values, busy-query suppression, and both flag directions.
- Ghidra contains synchronized names, prototypes, comments, and the confirmed `RuntimeInputContext`, 0x934-byte `RuntimeCommandLoopState`, and `RuntimeCommandBounds` types, and is saved. Current source recovery count is 84/846 baseline functions, with 10 bundled library functions delegated and 752 baseline functions remaining. Continue through the two message-specific handlers at `0x00414590`/`0x004145d0` and their query/update dependencies, then return to unresolved shutdown/script callees and the complete primary window procedure `0x0041d560`.

- Runtime input transport recovery now implements three distinct original rings: byte input `EnqueueRuntimeByte`/`DequeueRuntimeByte`/`ResetRuntimeByteQueue` (`0x00420640`, `0x004206d0`, `0x00420750`), two-DWORD input `EnqueueRuntimePair`/`DequeueRuntimePair`/`ResetRuntimePairQueue` (`0x00420910`, `0x004209b0`, `0x00420a50`), and DWORD message `EnqueueRuntimeMessage`/`DequeueRuntimeMessage` (`0x00420a90`, `0x00420b50`). `ClearCreditsRuntimeFlag` (`0x00420cb0`) is also implemented and is now the production default used by `FinishCreditsState`.
- All rings have exact 32-slot storage with one slot sacrificed to distinguish full from empty. Byte and pair rings require graphics-host flags `0x100400`; the message ring requires `0x400`, suppresses consecutive duplicates, and treats `0x30f` as an index reset before enqueue. Reset functions intentionally clear indices without clearing the corresponding availability flag. Tests cover inactive gates, FIFO behavior, duplicate suppression, special reset, overflow/wraparound, retained queued state while flags are inactive, and the reset/availability asymmetry.
- `CopyRuntimeInputSessionRecord` (`0x004208e0`) is also implemented: it copies exactly eight DWORDs from the persistent global record, returns the separate status word, and clears only that status. Its 0x20-byte type and prototype are synchronized in Ghidra, and tests verify repeated copies retain the record while consuming status once.
- Ghidra now contains synchronized names, calling conventions, prototypes, comments, `RuntimeMessagePair`, and `RuntimeInputSessionRecord` for this cluster, and the program is saved. Current source recovery count is 81/846 baseline functions, with 10 bundled library functions delegated and 755 baseline functions remaining. The next primary-window dependency targets are the input-session initializer at `0x00420790` and the remaining shutdown/script helpers called by `0x0041d560`.

- The screenshot/bitmap-capture tranche now implements and tests `SaveGameScreenshot` (`0x0041cbe0`), `CaptureGameBitmap` (`0x0041cb90`), `CaptureBitmapIfRuntimeActive` (`0x0041f8b0`), and `CreateIndexedBitmap` (`0x00417790`). The capture source is an exact 0x28-byte x86 structure with width/height/pixels at `+0x20/+0x22/+0x24`; the game context supplies those values at `+0x478/+0x47a/+0x488` and the palette at `+0x48c`.
- The BMP constructor emits an exact 0x436-byte 8-bit header/palette prefix, reverses the palette source's RGB byte order into BGR, flips source scanlines vertically, and implements the mode-1 half-resolution path by sampling every other row and column. Tests cover null palette, both resolutions, inactive-host gating, context adaptation, dialog cancellation, capture failure, file-create failure, successful write/close, and cleanup.
- Primary-window dependency work also implements `SetScriptRuntimeFlags` (`0x004068c0`) and `CopyFileNameFromPath` (`0x0040d030`), with null-runtime, set/clear-mask, rooted, rootless, and trailing-backslash tests. Ghidra names, prototypes, comments, and `BitmapCaptureSource` are synchronized and saved.
- Current source recovery count is 71/846 baseline functions, with 10 bundled library functions delegated and 765 baseline functions remaining. The complete primary window procedure at `0x0041d560` has been re-traced; immediate work remains its unresolved shutdown/render/script callees before implementing its full dispatcher without placeholders.

- The primary-window dependency tranche now implements `FixedDwordMemoryEqual` (`0x0040d070`), `FindScriptObjectByName` (`0x00408380`), and `ResolveStateFieldReference` (`0x00408660`). Confirmed script layouts include 32-byte object/field names, direct object chain at runtime-root `+0xf7c`, container chain at `+0xf98`, 12-byte container slots, field count `+0x428`, active mask `+0x480`, 32 integers at `+0x484`, and 32 fixed strings at `+0x504`. Tests cover direct/nested/missing lookup and boolean/integer/string field transitions.
- Source also implements `HandleCurrentStateActivation` (`0x0041d380`), `SaveApplicationStateInteractive` (`0x0041d280`), `OpenApplicationStateInteractive` (`0x0041d1c0`), synchronized wrappers `0x0041f7c0`/`0x0041f830`/`0x0041f8f0`, and `SetRuntimePathsOnce` (`0x00420c30`). Assembly review corrected `0x0041d280` to ECX state plus EDX dialog context and corrected the synchronized wrappers to mixed register/stack ABIs (4, 6, and 4 total arguments respectively).
- Confirmed additional application fields: saved flags `+0x30`, saved memory `+0x34`, script state `+0x3c`, and 260-byte startup configuration name `+0x50`; successful open-state selection writes exact `START.CFG`. Tests cover activation gates/cursor/credits/status flags, owned vs temporary save snapshots, open success/cancel and 8-bit restoration, synchronized result/fatal-message handling, and one-time path publication.
- Current source recovery count is 65/846 baseline functions, with 10 bundled library functions delegated and 771 baseline functions remaining. Immediate next work is to bind the newly recovered wrappers as production defaults, recover remaining primary-window command helpers, then implement/test the complete `0x0041d560` procedure.

- Continued startup/platform recovery implements `UpdateApplicationWindowLayout` (`0x0041ce60`) with both exact branches: optional 0x1c-byte secondary-layout filling and full desktop/main/capture-window recentering. Confirmed application-state fields include capture HWND `+0x20`, content bounds `+0x40..+0x4c`, top adjustment `+0x464`, and adjusted desktop RECT `+0x468`. Tests cover both branches and the null-window gate, including publication to game-context `+0x490/+0x494`.
- Source now also implements `RestoreApplicationDisplay` (`0x0041d120`), `DisableRuntimeSubsystem` (`0x00420be0`), `SetCreditsRuntimeFlag` (`0x00420c90`), `EnterRuntimeState1000` (`0x00424260`), and `LeaveRuntimeState1000` (`0x00424290`). Tests cover display-mode direction and flag transitions, offscreen capture positioning, idempotent runtime toggles, and the `0x4000` leave-state gate. All names/prototypes/comments and the new `GagSecondaryWindowLayout` type are saved in Ghidra.
- Current source recovery count is 55/846 baseline functions, with 10 bundled library functions delegated and 781 baseline functions remaining. The next platform target is the primary window procedure `0x0041d560`; its full branch tree is now decompiled, but several command handlers (`0x0041d1c0`, `0x0041d280`, `0x0041d380`, and downstream state/reference routines) should be recovered first so the dispatcher can be implemented without placeholder behavior.

- The custom bitmap-control cluster is now source-implemented and synchronized in Ghidra: `InitializeCustomControlGdi` (`0x00417ab0`), `SetCustomControlBitmap` (`0x00417b60`), `RealizeAndPresentCustomControl` (`0x00417cb0`), `DestroyCustomControlGdi` (`0x00417d10`), and `GagCustomControlWindowProc` (`0x00417d90`). `GagCustomControlState` is confirmed through `+0x84` and has exact size `0x88` for this recovered slice.
- The bitmap uploader ABI was verified from assembly: ECX control state, EDX `BITMAPINFO`, stack present flag. It accepts only 8-bit images, copies `width*height` bytes from bitmap-info offset `+0x428`, updates 236 `PC_NOCOLLAPSE` palette entries on 8-bit displays, installs 256 DIB colors, and optionally presents.
- The custom window procedure's private `0x7ff0` commands are recovered: attach/init (1), get state (2), load `COMMENT.TXT`/`COMMENT.BMP` from a CDF (4), GDI cleanup (8), RT_BITMAP load (`0x10`), and direct BITMAPFILEHEADER block load (`0x20`). Production custom-class registration now uses this original procedure instead of the diagnostic `DefWindowProcA` fallback.
- Focused GDI tests cover 8-bit and high-color initialization, exact palette/presentation order, bitmap replacement and pixel copy, non-8-bit rejection, and teardown ordering. The full custom-window dispatcher still needs an injectable WinAPI/resource/archive seam to cover each message/command failure branch deterministically; its source follows the complete verified assembly but is not counted as fully branch-tested acceptance yet.
- Current source recovery count after this cluster is 49 original functions out of the milestone's 846-function baseline, with 10 bundled CRT/library/DEFLATE functions intentionally delegated. Remaining baseline functions: 787. Ghidra additionally recognizes two procedures analysis originally missed (`0x0041e680` and `0x00417d90`), so a later ledger audit should decide whether reporting should revise the denominator to 848 while retaining the original baseline for continuity.

- The user wants work grouped into substantial subsystem milestones rather than a few functions per handoff. Continue autonomously through the active milestone unless binary evidence is ambiguous, runtime observation is required, or approval is needed.
- Every milestone handoff must report function statistics against Ghidra's current recognized-function count. Report at least: source-implemented original functions, Ghidra-identified/documented functions, CRT/library functions intentionally delegated, and remaining unresolved functions. Do not count test seams, diagnostic fallbacks, or new non-original helpers as decompiled functions.
- Main-game implementation must be decompiled function-by-function from Ghidra. Existing repository utilities, including `tools/cdf_extractor`, were written from file-format observations and must not be analyzed or matched as implementations of original functions. They may describe data content but are not source evidence for GAG code.
- The only implementation-reuse exception is the user's historical FLIC decoder in `data/flic_player.cc/.hh`, because the original optimized decoder has recursion transformations and jumps across nominal function boundaries. Even for that exception, first identify each corresponding `GAG.EXE` routine in Ghidra and preserve its confirmed ABI, state transitions, and original call boundaries.
- Main-game recovery has started from `/GAG.EXE` in Ghidra. The program is a 32-bit x86 PE with 846 recognized functions; behavioral parity is the selected acceptance target.
- Ghidra confirms the CRT entry at `0x0042c130` calls the ANSI WinMain body at `0x0041cae0`. `GagWinMain` initializes a fixed 640-by-480 application, calls `SetRuntimeFlag40`, dispatches messages through `GetMessageA`, `TranslateMessage`, and `DispatchMessageA` until `MSG.message == WM_QUIT`, conditionally shows the internal-error message for state flag `0x2000`, and returns `MSG.wParam`.
- `InitializeGagApplication` at `0x0041f4f0` has a custom x86 `__fastcall` ABI: width in ECX, height in EDX, followed by `HINSTANCE`, command line, and show command on the stack. It allocates an exact `0x48c`-byte state. Confirmed fields currently include `message_table` at `+0x14` and flags at `+0x478`; all other bytes remain explicitly unresolved.
- Ghidra now contains `GagApplicationState`, confirmed prototypes/comments, and names for `GagWinMain`, `InitializeGagApplication`, `RegisterGagWindowClasses`, and `SetRuntimeFlag40`; the program was saved after synchronization.
- Root CMake now requires a 32-bit generator and builds `gag.exe` plus `gag_startup_test`. Source under `src/` reproduces the confirmed startup loop through a narrow WinAPI test seam, with original-address comments on recovered functions and layout assertions for the state.
- The production initializer is currently an explicitly labeled inert diagnostic fallback returning null. It is not recovered behavior and keeps `gag.exe` from entering an incomplete runtime. The immediate next step is to recover the transitive initializer graph rooted at `0x0041f4f0`, beginning with registration/configuration/display checks and then graphics/window initialization, replacing the fallback only when the complete required chain is evidence-backed.
- Win32 Debug configuration and compilation succeed for the new targets and existing XTET targets. CTest passes both `gag_startup_test` and `xtet_sfs`; `check-format` and `git diff --check` also pass.
- The next recovered startup tranche implements `LoadInstallationRegistrySettings` (`0x0041edf0`), `StringsEqual` (`0x0040cf90`), and `CopyDirectoryFromPath` (`0x0040cff0`). The registry routine uses `HKLM\\SOFTWARE\\ZES't Corp.\\GAG`, requires exact version `Russian Edition Version 2.51`, reads `path` and DWORD `set`, masks settings with `0x02001020`, and preserves staged returns `0x10000`, `0x20000`, `0x40000`, and `2`.
- `GagApplicationState` now exposes additional confirmed fields: instance `+0x00`, dimensions `+0x04/+0x08`, validation flags `+0x0c`, archive context `+0x10`, installed version `+0x154`, installation path `+0x258`, and executable directory `+0x35c`. Its size remains exactly `0x48c`; unrecovered regions remain byte arrays.
- Focused registry tests cover success, missing key, missing version, wrong version, path normalization, settings masking, and the archive-context branch. Ghidra names/prototypes/comments are synchronized for these routines plus `RegisterCustomControlClass` (`0x004174b0`) and the single-RET `InitializeApplicationStateNoOp` (`0x0041f4e0`). Win32 Debug build, CTest, and `check-format` pass.
- The next startup dependency is the validation dispatcher at `0x0041f040`, followed by its CD/path discovery and display-capability branches. The production initializer remains gated until those paths and the graphics host are complete.
- Stable-message-loop milestone analysis now covers `ValidateStartupEnvironment` (`0x0041f040`). Confirmed flag-controlled stages are single-instance rejection (`0x1`), registry plus installed-file checks (`0x2`), game-data drive discovery (`0x4`), archive read-speed measurement (`0x8`), display depth/capability checks (`0x10`), optional error UI (`0x20`/`0x40`), executable-relative path construction (`0x80`), alternate 640x480 display-mode detection (`0x200`), and forced high-color state (`0x400`).
- `LocateGameDataDrive` (`0x0041ebd0`) does not merely inspect files: it enumerates eligible logical drives, mounts candidate CDF archives through the original archive subsystem, reads `Version.txt`, requires the exact edition string, and checks the requested archive name case-insensitively. Do not replace this with a plain directory search.
- `MeasureArchiveReadSpeed` (`0x00417990`) measures reads through the archive layer in 0x8000-byte chunks using `timeGetTime`; its archive primitives remain under analysis. `DetectAlternateDisplayMode` (`0x0041efa0`) traverses the initialized graphics host's internal display-mode records rather than calling `EnumDisplaySettingsA` directly.
- Source now also implements exact `CopyString` (`0x0040cf50`) and `AppendString` (`0x0040cfd0`) primitives with focused tests. Ghidra names are synchronized for those functions, the validation dispatcher, drive discovery, read-speed measurement, alternate-mode detection, and its three display-mode iterator wrappers. The stable-message-loop milestone is still in progress; this is not a milestone handoff.
- Continued milestone work established the graphics mode record as an exact 0x40-byte linked-list node with flags `+0x00`, width `+0x18`, height `+0x1c`, bits-per-pixel `+0x28`, and next `+0x3c`. Source now implements the filtered list start/advance routines at `0x00413650` and `0x004136a0`; focused tests cover flag filtering and termination. Ghidra contains the synchronized `DisplayModeRecord` type.
- The CD discovery archive chain is now identified as `OpenCdfArchive` (`0x004282a0`), `ReadCdfEntry` (`0x004284e0`), and `CloseCdfArchive` (`0x00428590`). Opening allocates an exact 0x207c-byte archive state, accepts three seven-byte archive signatures, and delegates index construction to `0x004287e0`. Entry reads choose direct or chunk-decoded paths based on entry flag `0x10`. Full archive structures and decompression are the active recovery point.
- Continued stable-message-loop work corrected the display-wrapper ABI from instruction-level evidence and implemented `FindCurrentDisplayMode` (`0x004136f0`), `GetCurrentDisplayMode` (`0x0041f960`), `BeginDisplayModeEnumeration` (`0x0041f980`), `GetNextDisplayMode` (`0x0041f9a0`), and `DetectAlternateDisplayMode` (`0x0041efa0`). The detector always enumerates mask `0x10000`, searches for a distinct 640x480 node at the active bits-per-pixel, repeats the scan at most twice exactly as compiled, and sets state flag `0x4000`.
- Source also implements the initializer-adjacent state helpers `SwitchDisplayModeIfEnabled` (`0x0041d010`), `EnableRuntimeSubsystem` (`0x00420bc0`), `SetActiveObjectField0824` (`0x00404980`), runtime flag setters/clearers (`0x00420c00`, `0x00420c10`), runtime-command clear (`0x00420cd0`), confirmed no-op hooks (`0x0041ce40`, `0x0041ce50`), and application flag helpers (`0x0041cdc0`, `0x0041cd30`). Ghidra names, prototypes, comments, and the program save are synchronized. Focused tests and Win32 Debug build/CTest/check-format pass. The milestone remains in progress; archive/index recovery and the graphics-host constructor still gate the production initializer.
- The CDF tranche now has a dedicated `src/cdf_archive.*` implementation derived only from Ghidra. Confirmed `CdfEntry` size is `0x2c` with flags `+0`, name `+1`, file offset `+0x24`, and uncompressed size `+0x28`; confirmed `CdfArchive` size is `0x207c` with storage `+0x11c`, count `+0x120`, handle `+0x124`, alternate-stream flag `+0x128`, second handle `+0x12c`, error `+0x130`, and the 2000-pointer table at `+0x13c`. Ghidra contains synchronized versions of both structures.
- Source implements `GetCdfEntryFlags` (`0x00428630`), `GetCdfEntrySize` (`0x004286a0`), `ReadCdfEntry` (`0x004284e0`), `CloseCdfArchive` (`0x00428590`), and the direct reader `ReadUncompressedCdfEntry` (`0x00429320`). Tests cover null/missing/case-insensitive lookups, selector filtering, direct and alternate I/O, short-read errors, compressed dispatch, and exact cleanup ordering. The alternate stream and compressed-reader test bindings remain explicitly unresolved production seams until their original callees are recovered. Win32 Debug build, CTest, check-format, and whitespace checks pass.
- `ReadCompressedCdfEntry` (`0x004293d0`) is now source-implemented. It preserves the exact offset-table cardinality, 0x10000 temporary compressed buffer, sequential direct/alternate reads, 0x8000 destination stepping, cleanup, and staged archive errors `0x20000`, `1`, and `0`. Its focused test uses two chunks with unequal compressed sizes and confirms destination offsets. The production DEFLATE callback remains unresolved pending a library dependency decision; the original bundled implementation is now positively classified and documented as library code.
- Ghidra names/prototypes/comments now cover the bundled raw-DEFLATE boundary and nine internal routines from `0x0040f8d0` through `0x00410b50`. These are intentionally delegated library functions rather than game functions to reconstruct. Direct Ghidra memory confirms signatures `CDF96a` at `0x00442154`, `CDF97a` at `0x00442178`, and `CDF96b` at `0x00442180`. `CdfArchive` now additionally exposes signature `+0x04`, path `+0x14`, index size `+0x118`, and index-data size `+0x138`, synchronized in Ghidra and source.
- `InitializeCdfIndex` (`0x004287e0`) is now implemented for all three formats. `CDF96a` reads a 16-bit compressed index size, fixes 256 entries, allocates `0x2c08`, and points records at storage `+4`; `CDF96b` additionally reads a 16-bit count and 32-bit index-data size and uses direct `0x2c` records; `CDF97a` reads three 32-bit header values and expands independently compressed index chunks through a trailing offset table. Tests cover all three layouts and pointer-table construction.
- `OpenCdfArchive` (`0x004282a0`) is now implemented with its exact direct-file parameters, two-handle alternate lifecycle, zeroed `0x207c` allocation, three signature checks, signature/path copies, index-constructor call, cleanup, and global error stages. Instruction review corrected the alternate close ABI to receive each handle in ECX. Direct open plus index construction is tested end-to-end with a synthetic `CDF96b` byte stream. Ghidra is synchronized and saved. The CDF production path now only lacks binding of the delegated DEFLATE library and recovery of the alternate-stream implementation.
- `LocateGameDataDrive` (`0x0041ebd0`) is now source-implemented with exact four-byte drive-root iteration, drive-type exclusions, `*.cdf` enumeration, selector-zero `Version.txt` read, exact edition comparison, case-insensitive requested filename check, archive cleanup, and `(required+8)&~3` drive-list allocation. Instruction evidence confirms state `+0x154` receives the complete matching archive path. A focused test covers a skipped removable drive followed by a successful CD-ROM candidate without using `tools/cdf_extractor` as evidence.
- Source now also implements `ClearRuntimeActiveFlag` (`0x0041cd50`), `ClearApplicationLockFlag` (`0x0041cdd0`), and `FreeHeapMemory` (`0x00417970`). Cursor tests cover skip gates, strict threshold equality, threshold crossing, and flag results; the exact `GetSystemMetrics` indices are 4, 8, and 15. Ghidra is synchronized and saved. Win32 Debug build, CTest, check-format, and whitespace checks pass.
- `ValidateStartupEnvironment` (`0x0041f040`) is now source-implemented with its corrected custom ABI: ECX state, EDX requested archive path/name, and stage flags on the stack. Tests cover single-instance rejection, all three registry error messages, installed-file flags, drive discovery success/failure, executable-relative and embedded-archive paths, speed failure/slow-media rejection, display depths below 8/above 16, accepted 16-bit caps, validation-flag latching, alternate-mode dispatch, and the fact that force bit `0x400` acts only inside stage `0x200`. Confirmed state fields now include HWND `+0x18` and display caps `+0x47c/+0x480/+0x484` in source and Ghidra.
- The initializer registration cluster is now implemented: `InitializeApplicationStateNoOp` (`0x0041f4e0`), `RegisterGagWindowClasses` (`0x0041f3d0`), and `RegisterCustomControlClass` (`0x004174b0`). Tests assert exact ANSI class names, `WNDCLASSEX` size, extra-byte counts, icon slots/resource 105, `CS_OWNDC`, cached custom registration, and failure UI offset `0xe38`. Production callback slots remain pointed at explicit diagnostic procedures until the original window procedures are reconstructed. Ghidra is synchronized and saved; all automated checks pass.

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

## GAG display-mode change milestone

- Recovered and tested `SetActiveDisplayMode` (`0x00413780`), `RestoreActiveDisplayMode`
  (`0x004138D0`), and their graphics-ready wrappers at `0x0041F9C0` and `0x0041F9E0`.
- DirectDraw changes preserve temporary cooperative-mode bracketing. The Windows
  path builds the exact 0x94-byte ANSI `DEVMODEA` and updates the current-mode
  pointer only after success.
- Corrected `SwitchDisplayModeIfEnabled` (`0x0041D010`): nonzero EDX selects the
  stored mode and zero restores the current mode.
- Ghidra is synchronized and saved. Win32 Debug build and both CTest tests pass.
- Recovered `ShutdownDisplayModeHost` (`0x004134D0`): guarded no-op, busy-bit
  wait/recheck, palette/surface teardown, ordered display-mode list frees,
  critical-section deletion, and display-state clearing are tested. Ghidra now
  types/names the display-mode-list head and is saved.
- Recovered the shutdown aggregation cluster: `ShutdownRuntimeGenericBackend`
  (`0x00410BD0`), `ShutdownAsyncFileSubsystem` (`0x00414E40`),
  `AcquireFirstRuntimeMediaBackend` (`0x0042B290`),
  `ShutdownRuntimeMediaBackend` (`0x00429E50`), and `ShutdownGraphicsHost`
  (`0x00420230`). Tests cover disabled, busy, raced insertion, subordinate failure,
  heap failure, and full-success ordering.
- Recovered `ClearRuntimeDisplay` (`0x00427880`), a window-procedure dependency:
  it clears and publishes the full runtime-host rectangle only after successful
  display-lock acquisition. Both lock branches and exact call order are tested.
- Mapped the full `0x0041D560` window-procedure decompile into message families;
  it remains unresolved until register-passed arguments in its custom-command
  branches are verified from assembly.
- Recovered generic-backend dependencies `ClearRuntimeGenericBackendReady`
  (`0x00410D40`) and `FindAvailableRuntimeGenericChild` (`0x00410DE0`), including
  ordered two-level scanning, threshold comparison, filtering, and mutex release.
- Recovered `FindRuntimeGenericTextEntry` (`0x00410E50`) and
  `CreateRuntimeGenericBackendChild` (`0x004110B0`). Tests cover category/name
  scanning, entry versus block return offsets, numeric and named selectors,
  missing parents, allocation failure, child initialization, list insertion, and
  call ordering. The assembly-proven missing-entry uninitialized default slot is
  deliberately preserved.
- Recovered `ProcessAvailableRuntimeGenericChildren` (`0x004212E0`) from its
  instruction-level stack layout. Tests cover normal publication, busy-scene
  suppression, pointer-hidden positioning, stale-child destruction/release, and
  mode-0x200 fallback. Ghidra is synchronized and saved.
- Recovered `RunPendingRuntimeExternalCommand` (`0x00421010`). The runtime command
  context now has confirmed fields at `+0x454` (message context) and `+0x928`
  (pending flag). Tests cover the inactive gate, rejected synchronous message,
  repeated message/command processing, result accumulation, 10 ms sleeps, and
  unconditional flag cleanup. Ghidra is synchronized and saved.
- Recovered `RefreshRuntimePointerRegion` (`0x004236C0`) and
  `SetRuntimeResourceLoopCount` (`0x004258D0`). Tests verify active-region clearing,
  current-coordinate forwarding and tail-call return propagation, missing-resource
  behavior, the sound loop path, non-sound backend flag `0x400`, unsigned
  `count - 1` storage at resource offsets `+0x50/+0x54`, and lock release ordering.
  Ghidra is synchronized and saved.
- Recovered the runtime-tree query cluster:
  `FindRuntimeTreeIdentityByNameRecursive` (`0x00406640`),
  `FindRuntimeTreeDescendantIdentityByName` (`0x004066C0`),
  `FindRuntimeTreeRootIdentityByName` (`0x00406720`), and
  `HasRuntimePointerTreeFlag1000` (`0x004237B0`). Tests distinguish recursive
  child-first search, descendants-only search, top-level sibling search, absent
  runtime roots, exact 0x20-byte names, empty pointer enumeration, and early
  flag-0x1000 success. Ghidra is synchronized and saved.
- Recovery statistics: 366/855 implemented, 10 library delegated, 479 remaining.
- Next: recover the adjacent plan-inactive transition (`0x00421130`) after its
  refresh dependency `0x00426700`, then the queued pointer dispatcher at
  `0x004211A0` with its three pointer-action callees.
- Next: continue through the sole caller at `0x0041D560` and its application-level
  shutdown orchestration.
# 2026-08-16 - Runtime resource constructor implementation accepted

- `0x00424EC0` now has its confirmed eight-formal-argument `__fastcall` source ABI and production implementation in `src/startup.cpp`; the old null production stub has been removed from resource selection and both rebuild APIs.
- Raw `RET 0x18`, prologue stack arithmetic, and all five callers confirm ECX/EDX plus six cleaned stack arguments. In the animation branch, formal width/height control X/Y scaling and formal scale-or-loop controls flag `0x400` plus the frame limit; there is no caller-frame constructor argument.
- Implemented branches cover bitmap type 1, sound type 2, animation type 3, script/tree type 4, generic type 0/type-4 fallthrough, CDF type 5, and shared resource registration/callback publication.
- Ghidra prototype and plate comment at `0x00424EC0` were corrected. `CFG` at `0x00442168` and `Start.cfg` at `0x0043F124` were confirmed directly from program bytes.
- Focused injected tests cover every dispatch family and the observed success/failure cleanup classes, including allocation failures, failed backends, scene failures, memory-versus-stream animation release, tree suppression, generic fallthrough, archive failure, registration, and visibility callbacks.
- Win32 Debug build, CTest, `check-format`, and whitespace checks pass. The function is accepted and included in the 553/853 recovery count.

# 2026-08-16 - Adjacent runtime-root head mutation audit

- Re-audited `RemoveRuntimeGenericResource` (`0x00405000`), `RemoveRuntimeVisualObject` (`0x004091B0`), `PurgeDisabledRuntimeNamedNodes` (`0x00407EE0`), `DestroyRuntimeFixedNameListNodes` (`0x00407440`), and `DestroyScriptObjectStates` (`0x00408D80`) against the current Ghidra decompilation and corrected `ScriptRuntimeRoot` layout.
- Their source implementations already target the correct heads at `+0xF74`, `+0xF80`, `+0xF84`, `+0xF88`, and `+0xF7C`; no implementation correction was required.
- Added focused cross-field tests proving those operations preserve the adjacent `runtime_tree`, `visual_objects`, `runtime_nodes`, `fixed_name_nodes`, and `plan_nodes` fields. This directly guards the same offset-confusion class that previously wrote `runtime_nodes` instead of `runtime_tree`.
- Added the verified adjacent-field facts to all five Ghidra functions and saved `/GAG.EXE`.
- Win32 Debug build, CTest (2/2), `check-format`, and tab/whitespace checks pass.
- Current accounting baseline remains: 559/559 recognized game functions represented in source; 136/136 internal library/runtime functions delegated; zero recognized game-function addresses absent. This is address coverage, while the function-by-function fidelity audit remains in progress.

# 2026-08-16 - Seven-family runtime-tree routing audit

- Reverified `PublishRuntimeTreeGlobalLinks` (`0x00406190`), `UpdateRuntimeTreeGlobalLinks` (`0x00406360`), and all seven inclusive range removers: scene `0x00409920`, secondary resource `0x00409CB0`, primary resource `0x0040A650`, link-84 `0x0040B130`, link-8C `0x0040B6B0`, link-7C `0x0040BE20`, and containers `0x0040C9F0`.
- Source implementations and existing focused tests match the Ghidra control flow, including separate null-parent and `0xffffffff`-parent routing, family-specific next offsets (`+0x40`, `+0x48`, or `+0x24`), ancestor propagation, and the asymmetric global-tail repair behavior. No source correction was required.
- Completed the Ghidra type model for confirmed `RuntimeTreeNode` fields `+0x70..+0xB8` and `ScriptRuntimeRoot` heads/tails `+0xF78..+0xFC0`, replacing unnamed/`-BAD-` entries with the established visual, scene, primary, secondary, link-84, container, child, auxiliary, next, and previous types/names.
- Forced all nine functions to re-decompile with the corrected structures, added verification comments, and saved `/GAG.EXE`.
- Win32 Debug build, CTest (2/2), `check-format`, and tab/whitespace checks pass.
- Accounting remains 559/559 recognized game functions represented; 136/136 internal library/runtime functions delegated; fidelity audit continues.

# 2026-08-16 - Central runtime-tree destruction re-audit

- Reverified `DestroyRuntimeTreeNode` (`0x00405E50`) against raw assembly after completing the tree/root types. Exact order is recursive children; scene, secondary, primary, link-7C, link-84, link-8C, and container ranges; auxiliary nodes; parser contexts; flag-driven property notifications; optional replacement parser redispatch; global-link publication; sibling/root unlink; final node free.
- Raw assembly confirms property operations `0x40`, `0x0E`, and `0x20` use ECX/EDX plus one stack value through the root `+0x814` fastcall callback. Replacement is retained from the second tree lookup and returned after optional redispatch.
- Corrected Ghidra's `FindRuntimeTreeNode` (`0x0040CD60`) prototype to `RuntimeTreeNode *__fastcall(RuntimeTreeNode *root, void *identity)`, eliminating integer-typed tree/parent results in callers.
- Extended the destructor test with two owned nodes plus a successor for every link family and containers. The exact 26-event sequence proves each family frees/destroys head through its recorded tail inclusively and leaves `tail->next` untouched.
- Ghidra comments were updated and `/GAG.EXE` saved. Win32 Debug build, CTest (2/2), `check-format`, and whitespace checks pass.

# 2026-08-16 - Parser-context and auxiliary ownership audit

- Reverified `FindOrCreateRuntimeTreeParserContext` (`0x00405210`), `ReleaseRuntimeTreeParserContexts` (`0x004052F0`), `FindExistingRuntimeTreeParserContext` (`0x00405350`), and `ReleaseRuntimeTreeAuxiliaryNodes` (`0x004071E0`) against decompilation and raw assembly.
- Corrected all four Ghidra prototypes to `__fastcall` with typed owners/returns, and typed `RuntimeTreeParserContext +0x250` as its self-pointer next field. Completed `RuntimeTreeAuxiliaryNode` as name `[0x20]`, identity `+0x20`, and typed next `+0x24`.
- Parser-context release deliberately leaves owner `+0x6C` unchanged. Per context it saves next, conditionally decrements resource active references, removes the resource if the resulting count is zero, and only then frees the context.
- Auxiliary release unlinks the head before callback operation 7 and then frees that node. Existing tests already cover exact callback/free traversal; parser release tests now additionally verify the complete remove-before-free operation sequence.
- Ghidra comments were updated and `/GAG.EXE` saved. Win32 Debug build, CTest (2/2), `check-format`, and whitespace checks pass.
- `DispatchRuntimeTreeParser` (`0x004056C0`) was inspected but is not accepted by this tranche: its large branch family currently has only narrow end-to-end coverage and needs a dedicated property-by-property audit/test harness.

# 2026-08-16 - Parser reset, section dispatch, and jump audit

- Reverified `DispatchRuntimeTreeSection` (`0x00405380`), `FindAndCreateRuntimeTreeJump` (`0x00405D00`), `ResetRuntimeTreeParserContexts` (`0x00405DC0`), and `ResetRuntimeTreeParserContextRecursive` (`0x00405E00`). Existing focused tests already cover their observed branch families, so no source change was required.
- Section dispatch retains the resource on section miss but removes the caller-provided resource identity when parser-context creation fails. Successful creation returns the parser dispatch result directly.
- Jump scanning saves the original cursor; only a matching property `0x70` synchronizes the owner. Missing third name copies the complete 32-byte second name and reuses the parser's resource; explicit third name loads the second as an external resource. Failure restores the old cursor and success publishes the supplied success cursor.
- Recursive reset publishes `start_offset` to `cursor` before parsing. Only property 10 resolves an included tree; every included context is recursively reset before scanning of the current context continues.
- Corrected all four Ghidra prototypes/calling conventions, added verification comments, and saved `/GAG.EXE`.
