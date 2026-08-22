# Learned patterns

# 2026-08-20 - Match zlib framing to the recovered stream format

- Replacing a bundled inflater with zlib requires selecting the original framing explicitly: raw DEFLATE uses `inflateInit2(..., -MAX_WBITS)`, while ordinary `inflateInit` expects a zlib wrapper. Preserve recovered boundary behavior separately from framing; XTET accepts a logically complete 32 KiB SFS block when the fixed output buffer fills before the inflater consumes its end marker, but partial blocks still require `Z_STREAM_END`.
- A static library that calls fetched zlib should link the exported zlib target publicly so its final consumers receive the required link dependency. Independently compiled test executables using the same source must link that target directly.

# 2026-08-20 - Pin FetchContent archives by cryptographic hash

- For a small upstream dependency with working CMake support, prefer an official immutable release archive plus `URL_HASH` over a floating Git branch/tag or a hand-maintained source manifest. Disable the dependency's unrelated shared-library, test, and install targets before `FetchContent_MakeAvailable`, then consume its exported target so generated headers and usage requirements propagate correctly.
- Do not delete an existing untracked vendored source tree merely because the build no longer needs it; confirm ownership or leave it as user-local data.

# 2026-08-20 - Portable resource precedence belongs at each recovered resolver boundary

- When a legacy application resolves archive packs through startup/drive validation but resolves plugin DLLs through an active resource directory, adding one process-wide search-path mutation does not reliably cover both and can affect unrelated file access. Under a compatibility guard, prepend the executable-directory candidate independently at each recovered resolver and fall back through the untouched original path.
- Build sibling candidates from `GetModuleFileName`, use only the requested DLL's filename for the executable-directory probe, reject truncated or overflowing `MAX_PATH` results, and load DLLs by absolute path. Opposing fixes-on/fixes-off tests should prove both precedence and original fallback behavior.

# 2026-08-19 - Decode a save before diagnosing a missing conditional scene object

- GAG save slots are CDF archives whose embedded `START.CFG` records object fields, inventory membership, and the active scene. When an original binary and the reconstruction both hide an object from the same save, extract that state and evaluate the scene's `local` condition conjunction before changing runtime code.
- A missing inventory object can remain intentionally gated by a separate story flag even while its own `Pick_Up` field is OFF. Trace both the pickup object's state and every prerequisite field used by the containing scene.

# 2026-08-17 - Scope focus-loss compatibility to Window mode

- Legacy fullscreen applications may intentionally minimize on `WM_ACTIVATE` focus loss. For a modern fixed-frame Window mode, suppress only the automatic minimize post while the recovered Window-mode flag is active; preserve explicit user minimization, fullscreen focus-loss behavior, and the original fixes-OFF path.

# 2026-08-17 - Legacy window mode may still assume a desktop-sized game resolution

- A recovered Window/Full Screen toggle does not guarantee useful modern window geometry. Preserve its state machine, but under the compatibility guard size Window mode from the native framebuffer client dimensions with menu-aware `AdjustWindowRect`, while leaving Full Screen and fixes-OFF behavior unchanged.

# 2026-08-17 - Emulate legacy color depth at the framebuffer boundary

- On a modern true-color desktop, keep the original renderer in a supported 8- or 16-bit top-down DIB and let GDI convert during presentation. This preserves original asset selection, palette construction, pixel compositing, and renderer assumptions while avoiding unsupported physical display-mode changes.

# 2026-08-17 - Validate delegated codecs through production bindings and original containers

- Keep library replacements behind the recovered callback ABI, test their framing and failure contracts directly, and also open an original container through the default production binding. Synthetic injected callbacks alone cannot prove that the executable target is wired to a functioning codec.

# 2026-08-17 - Injected callback tests do not validate production library bindings

- A function can pass complete branch tests while its production callback remains a failure stub. For every delegated compression or runtime-library boundary, add at least one test through the default production binding using original executable data; otherwise ignored callback return values can publish successfully allocated but zero-filled state.

# 2026-08-17 - Keep compatibility behavior compile-time isolated from recovered behavior

- Modern-Windows fixes must be explicit conditional branches that retain the original path when disabled. Test both compiled variants independently, label compatibility branches non-original in source, and leave Ghidra's representation describing only the executable's original behavior.
- Registry access should request the narrow operation-specific right: `KEY_QUERY_VALUE` for the recovered read path and `KEY_SET_VALUE` for the settings write path. A Windows 95-era `KEY_ALL_ACCESS` request can fail for a normal user on modern HKLM even when all values exist.

# 2026-08-17 - Inspect every fixed-stride data slot before classifying a block

- A zero-filled tail in the first fixed-size string slot does not imply that the following slots are empty or dynamically populated. For a table referenced through base-plus-offset arithmetic, read each stride directly from the executable and verify both payload and padding before reproducing its initialized image.

# 2026-08-17 - Keep original copyrighted PE payloads opt-in and outside source

- Recovered numeric resource identifiers and templates can remain represented by an optional local bundle without placing original binary payloads in version control. Keep that bundle in an ignored evidence/data directory, make the RC input an explicit CMake cache path, and enable byte-parity tests only when the bundle is supplied.
- This repository's `.gitignore` is user-managed and must not be modified. Place local-only evidence beneath paths covered by its existing rules.

# 2026-08-16 - Recompile PE resources and verify their payloads, not just identifiers

- Referencing an original numeric icon, dialog, or bitmap identifier in recovered WinAPI code is insufficient if the reconstructed target does not embed the corresponding resource. Inventory the original `.rsrc` tree by type/name/language and include every required payload in the GUI target.
- RC source can preserve binary dialog templates exactly, but extended dialogs may carry per-control `EXSTYLE` values that are easy to miss in a visual transcription. Compare the compiled `FindResourceEx` payload byte-for-byte against the original executable; equal dimensions and item counts do not prove equality.
- A multi-language icon set must be ordered so RC assigns the original RT_ICON component identifiers before constructing RT_GROUP_ICON records. Verify component icons and group records independently.

# 2026-08-16 - Unknown tokens and extraction failures are separate parser outcomes

- Table-driven decoders return zero for a successfully extracted but unrecognized/case-mismatched name, while extraction failure returns `0xffffffff`. Tests must cover both outcomes and cursor positions.
- Delimiter cursor conventions differ by extractor: property-name extraction advances past `=`, while slash-scope extraction stops on the terminating delimiter (for example the colon). Do not normalize these positions in wrappers.

# 2026-08-16 - Optional parser lookahead may affect serialization only

- `ParseScriptFileValue` serializes one token of lookahead, restores the original cursor, then reparses that position as an integer qualifier. With no lookahead it still appends a trailing space and returns the integer parser's `0x7fffffff` sentinel while retaining the suffixed file name.
- Test serialized bytes, final cursor, and return sentinel together. Any one alone is insufficient to prove the original lookahead/restoration sequence.

# 2026-08-16 - Separate allocation publication from partial parse publication

- A parser can have two distinct failure boundaries. `ParseScriptObjectContainer` publishes nothing when the name or 0x1B4-byte allocation fails, but inserts the container into both local and flattened lists before reading the first condition triple; later token failure deliberately retains a partial container.
- Preserve an original uninitialized byte read through object-representation copying when checked scalar evaluation would trigger MSVC runtime diagnostics. Do not replace the unknown stack bytes with a guessed constant.

# 2026-08-16 - Preserve the return register across parser case bodies

- A parser's loop condition may test EAX after a case-specific callee rather than the original scope code. Link-8C parsing deliberately leaves the last integer result in EAX; a valid integer `-1` therefore terminates the scope loop even though the ordinary parse-failure sentinel is `0x7fffffff`.
- Test this with a valid `-1` followed by another recognized token. Verifying only final field values at EOF cannot distinguish preserved return-register control flow from a conventional scope loop.

# 2026-08-16 - Resolve parser-token semantics at branch destinations

- When adjacent parser cases differ only in coordinate interpretation, follow each raw compare directly to its destination block instead of trusting decompiler case presentation. In link-84 parsing, token `0x02000000` reaches four direct stores (POS), while `0x0B000000` reaches x/y stores plus two additions (RECT right/bottom conversion).
- Allocation-failure tests for flattened records must assert both publication layers: the owning node's local head/tail and the runtime root's shared flattened head/tail. A null return alone does not prove list integrity.

# 2026-08-16 - Convert absolute stores to offsets from the correct embedded base

- Nearby low offsets can be dangerously plausible. `0x00480124/0x00480128` looked like root `+4/+8` when viewed without the real base, but the embedded root begins at `0x0047F910`, making them `+0x814/+0x818`. Always subtract the confirmed absolute base before translating a store into a structure field.
- Validate initializer stores with cross-view tests: the property callbacks must land at root `+0x814/+0x818`, while root `+4/+8` remain zero flag words after initialization.
- Adjacent same-typed root pointers require independent preservation tests. Runtime tree head `+0xF78` and named-node head `+0xF84` are both pointer fields, so an incorrect write compiles cleanly; seed both and verify which one changes during root unlink.

# 2026-08-16 - Bulk reset ranges must be mapped against the complete owning allocation

- A standalone-looking `REP STOSD` target may begin in unnamed padding and cross later typed arrays. Convert its absolute start/end to offsets from the proven allocation base before modeling it.
- `ResetRuntimeSession` clears graphics-host `+0x11D8..+0x1924`; this crosses the scene-slot array at `+0x1444` and intentionally stops eight bytes into its final slot. Boundary tests should verify both the final cleared byte and the first preserved byte.
- Named records can overlap at both ends: pointer-event body DWORDs 14/15 at allocation `+0x9B4/+0x9B8` are also the embedded script root's flags and palette flags. Prefer the semantic field names in source while retaining a cross-view test that proves the shared addresses.
- Test a bulk operation through the production embedded view, not only through an independently allocated fixture. `ClearRuntimeCommandDefinitions` is locally a root `+0xA70` clear but physically a graphics-host `+0x1420..+0x1924` clear, so only an embedded-root fixture exposes its scene-slot overlap.
- Equal-stride arrays can be deliberately phase-shifted. Command definitions begin 0x20 bytes before scene slots with the same 0x28-byte stride, making definition N visual/flags the slot N prefix and definition N+1 name the slot N name. Verify writes through both views before assigning independent storage.
- Compatible list specializations can share the same root field. Graphics-host `+0x1944` is both the pointer-region head and the embedded script root's link-84 head; model the specialized pointer view as a reference to the semantic root field and prove bidirectional writes.

# 2026-08-16 - Backend ABI blocks may overlap named high-level fields

- Runtime graphics uses a ten-DWORD backend block beginning at `g_RuntimeDisplayContext +0x458`. Its first eight DWORDs are also the command-target buffer; the final two DWORDs overlap width/height and the display-surface pointer. Preserve the physical overlay because shutdown zeros all ten DWORDs in one loop.
- The runtime scene host (`+0x214`), pixel format (`+0x228`), root scene (`+0x248`), surface (`+0x47C`), callback positions (`+0x480..+0x488`), palette entries (`+0x48C`), and script thread (`+0x8FC`) are all fields of the shared context. Bootstrap and shutdown test helpers must expose views of that storage instead of parallel variables.

# 2026-08-16 - Bulk clears can intentionally cross semantic record boundaries

- `ResetRuntimeDisplayState` clears 21 DWORDs beginning at `g_RuntimeDisplayContext +0x95C`. This spans current/loading/tree/script fields and the full pointer-event record at `+0x970`; separate source arrays silently break the original clearing behavior.
- Treat an original `REP STOSD` range as one physical storage range even when later routines give subranges different semantic structures. Preserve aliasing first, then expose typed views at the confirmed offsets.
- Whole-state test setters must initialize every field relevant to the subsequent operation. Once previously detached scene X/Y values were correctly embedded at `+0x93C/+0x940`, assigning a zeroed `RuntimeCommandLoopState` correctly cleared them and exposed a stale-fixture dependency.

# 2026-08-16 - One original global may have several subsystem views

- A whole-block `REP STOSD` is decisive allocation-base evidence: `InitializeGraphicsHost` clears 0x75f DWORDs beginning at `0x0047EF60`, so `g_RuntimeDisplayContext` starts at allocation offset zero. `RuntimeGameHostContext` is a distinct subview at `+0x458`, callbacks at `+0x498`, and `ScriptRuntimeRoot` at `+0x9b0`. Model confirmed subviews over one backing allocation, but do not infer that equal field-relative offsets in different subviews alias; here the parent window at base `+0` and child window at subview `+0` are distinct.
- When correcting a global allocation base, audit semantically duplicated source globals even if tests already pass. In GAG, scene-control flags and graphics-host flags were one DWORD at allocation `+0x930`, while pointer coordinates were the scene X/Y DWORDs at `+0x93c/+0x940`; keeping detached copies hid cross-subsystem state propagation.
- Prefer direct structure-member access for embedded fixed-size arrays after proving an allocation alias. MSVC reference-to-array behavior complicated observation of GAG's result buffer; using `runtime_display_context.game_result_data` directly preserves the confirmed `+0x538` address and makes size/cross-view tests unambiguous.
- Do not treat a field's earlier semantic label as stronger evidence than its absolute xrefs. GAG state `+0x960` had been called a loading-scene Boolean, but references at absolute `0x0047F8C0` prove it is the current runtime-resource pointer; Boolean gates merely test that pointer for non-null.
- Reconcile a recovered allocation by enumerating every named/xrefed Ghidra data symbol across its complete address range. This exposed GAG arrays far beyond the small command-state prefix, including scene slots at `+0x1444`, pointer-list storage at `+0x1944`, and a palette whose exact end confirms the allocation boundary.
- Globals referenced as standalone symbols by early recovery can later prove to be fields of a larger object. In this case runtime flags at `+0x930` and command-pending state at `+0x928` must alias the corresponding `RuntimeCommandLoopState` fields. Re-audit every standalone source global whose Ghidra address falls inside a newly confirmed aggregate.
- When a wrapper reads an embedded window directly in the executable, a test seam should obtain the current value at the call site. Capturing a null/default window in a production API table disconnects later initialization from the wrapper.

# 2026-08-16 - Runtime queues and their locks are embedded in the main state object

- The queue counters, ring storage, indices, and five critical sections at `RuntimeCommandLoopState +0x6b0..+0x8f7` are one contiguous state region. Do not represent queue storage as separate globals or production locking as injectable no-op hooks once these offsets are confirmed.
- `InitializeGraphicsHost` and `ShutdownGraphicsHost` initialize/delete the five embedded critical sections in address order. Their roles are byte queue (`+0x880`), pair queue (`+0x898`), message queue (`+0x8b0`), synchronized resource/dialog operations (`+0x8c8`), and path/resource-record operations (`+0x8e0`). Palette presentation uses a different display-host critical section.
- CDF alternate handles are `AsyncFileRecord *` values opened from the caller-supplied async host. Bind seek/read/size/close to that recovered subsystem; do not invent a parallel stream abstraction. Compression callbacks remain explicit library boundaries until the corresponding bundled inflate/gzip implementation is linked.

## 2026-08-16 - Pointer rebuilds synchronize named-list slots before resource lifecycles

- `0x00407A80` starts at a runtime named node's circular cursor but iterates its configured count at `+0x28`, not its current child count at `+0x40`. Once the cursor wraps, remaining matching link-84 slots are disabled by clearing command mask `+0x40` and setting primary flag `0x80000000`.
- A populated slot publishes the child script object and its command mask. Ordinary objects use mouse name `+0x430`; natural-mouse objects use their visual's file at visual `+0x28`. If a natural-mouse object has no visual, the original forwards uninitialized local pointer bytes; preserve that representation without a checked uninitialized scalar read.
- `0x00426700` treats `(QueryRuntimeSceneFlags(identity) & 0x3000) != 0` as resource-counted. Retiring or disabling one decrements the local target count, while constructing one increments it. Primary flag `0x01000000` changes counted teardown from immediate finalization to requested destruction; non-counted resources always use requested destruction.
- `RuntimePointerRegion` is the full 0x68-byte link-84 overlay: primary resource `+0x5c`, previous owner `+0x60`, and previous primary identity `+0x64`. Rebuild clears only previous owner `+0x60` during region traversal.

## 2026-08-16 - Right-button command slots use a packed scene-slot DWORD

- `0x00423CA0` tests the DWORD at scene-slot `+0x04` against `0x00200000`; with the confirmed layout this bit is byte flag `0x20` at `+0x06`. Clear slots emit the ordinary pointer event, while set slots enter the state-object command-mask path.
- The lazy mouse-visual call to `ConstructRuntimeResourceObject` pushes the constructor's complete six stack arguments. Its final pushed value is the resource flag word; saved EDI is not an additional constructor argument.
- Right-button mode `0x30000` starts with event flags 3, adds 8 when a region resolves, and adds 4 plus the state-object pointer only when that object differs from the current pointer-state owner, producing exact flags 3, 11, or 15.

## 2026-08-16 - Auxiliary names are resolved before list admission

- Runtime-tree auxiliary nodes are 0x28 bytes: name `+0x00[32]`, self identity `+0x20`, and next `+0x24`. They are prepended at tree `+0xb0` after exact duplicate-name suppression.
- Creation temporarily places the caller name pointer at `+0x20`, invokes runtime operation 6 with its address, and rejects/frees the node if the callback clears it. Only then is the name copied and identity replaced with self.
- The default auxiliary-name source is a root string at `+0x96c`; it is split by ASCII space only. Parser-driven lists and the default string both preserve first-admission uniqueness through the shared constructor.

## 2026-08-16 - Conditional tree creation ignores incomplete predicates

- `0x00406CB0` and `0x00406EA0` use `/V` (`0x00010000`) for typed object-field comparison, `/C` (`0x0e000000`) for container-state comparison, and `/GLOBAL` (`0x00200000`) for parent sentinel `-1`.
- Missing pieces of a condition do not make it false. The update form still records that a condition was seen; the direct-create form simply continues. Only a complete comparison returning false rejects/destroys.
- The update form performs no mutation without an observed condition. Existing nodes with flag `0x800` are treated as an observed false condition and destroyed. The direct-create form always creates at the scope terminator if no complete predicate rejected it.

## 2026-08-16 - Integer expressions combine recursive values and runtime queries

- `0x0040F4F0` recognizes special forms by the first four token bytes. `RAND` recursively parses two bounds with `-10000`/`10000` defaults; `RELZ`, `RELI`, and `RELM` recursively parse an optional offset that defaults to zero.
- Token byte 4 selects X only when it is exactly uppercase `X`; every other byte selects Y. `RELZ` reads link-84 `+0x2c/+0x30`, `RELI` reads primary-link `+0x5c/+0x60`, and `RELM` issues script runtime operation 9/10 with a null source.
- `PHASE` issues operation 11 with EDX pointing to the primary-link field at `+0x4c` and a stack output pointer. `VALUE` resolves an object and field through the existing integer-field lookup. Missing link/object results restore the expression's entry cursor.

## 2026-08-16 - Parameter evaluation is positional and type-gated

- Parser scratch text at `+0x0c` is a whitespace-separated parameter-name list; creation text at `+0x08` is the parallel value list. `0x0040F070` finds the requested name's zero-based index in the first and parses that index from the second.
- `0x0040EEB0` treats only tab, LF, CR, and space as separators and copies the selected token into an unchecked 260-byte stack buffer. A missing requested type 1 defaults the value to `0x07000000`; missing types 2 and 4 default to zero. All defaults still return failure.
- `0x00408AA0` tries integer expression, image flag, then string, restoring the original cursor before each fallback. Its image branch tests only for nonzero, so even the `0xffffffff` unknown/failure sentinel is accepted and published as type 1. Preserve this nonconventional behavior.

## 2026-08-16 - Image flags split across root and tree state

- `0x0040E580` recognizes `BVALUE` and `PARAM` by their first four bytes, while ordinary image-flag names use exact string comparison. The otherwise ambiguous data strings at `0x0043E370`, `0x0043E374`, and `0x0043E378` are `OFF`, `ON`, and `DUAL`, mapping to `0x07000000`, `0x03000000`, and `0x00200000` respectively.
- `0x00406B40` consumes flags through the `0xffffffff` terminator. Bit 1 is global script-root flags `+4`, bit `0x04000000` is global script-root palette flags `+8`, and every other returned flag is ORed into the parser owner tree at `+0x2c`.
- `0x0040F2C0` is the shared recursive value-token reader: `PARAM` evaluates a recursively parsed name as type 4, while `SVALUE` recursively parses object and field names and reads a 32-byte string. Both substitutions normalize success to `0x20` and failure to `0xFFFFFFFF`.

## 2026-08-16 - Runtime configuration serializers preserve sparse-list output

- `0x004069D0` emits its leading CRLF whenever the runtime tree root exists, even if no node has a matching parser context. Context lookup uses the tree's own name at node `+0x00`; successful entries serialize resource name, tree name, and GLOBAL only for parent sentinel `-1`.
- `0x004068F0` emits recognized flags in fixed `PAL_NOADJUST`, `NOCOMMENT`, `NOSAVE` order. Any nonzero word starts and terminates a flags statement, so unrecognized-only bits deliberately serialize as `flags=;\r\n`.
- Fixed-name nodes are 0x58 bytes with identity `+0x20`, flags `+0x24`, serialized value `+0x28[44]`, and next `+0x54`. `0x004073D0` emits one CRLF before the whole list, not before every item.

## 2026-08-16 - Top-level tree removal routes seven heads through active selectors

- `0x00406360` is gated by removed node parent `+0x24 == NULL`; descendant removal cannot change the runtime-global/override heads.
- Scene and secondary selectors publish at override `+0x40` and `+0x48`. Primary, link-84, link-8c, link-7c, and container selectors publish at override `+0x24`. Without a selector, destinations are runtime root `+0xfa4`, `+0xfa0`, `+0xf8c`, `+0xf94`, `+0xf9c`, `+0xf90`, and `+0xf98` respectively.
- Replacement source heads are node `+0x74`, `+0xa4`, `+0x9c`, `+0x84`, `+0x8c`, `+0x7c`, and `+0x94`; a null replacement clears all seven selected destinations.

## 2026-08-16 - Runtime-tree destruction owns six inclusive link ranges

- `0x00405E50` removes scene, secondary-resource, primary-resource, `+0x7c`, `+0x84`, and `+0x8c` ranges only when their tail is non-null, then walks head through tail inclusively. Containers follow the same inclusive rule but use their destructor rather than direct `HeapFree`.
- Children are recursively destroyed using a saved next pointer. Final unlink chooses previous sibling first, otherwise parent child head, otherwise the runtime root; a surviving next sibling always receives the removed node's previous pointer.
- Flags `0x8000`, `0x2000`, and `0x4000` invoke runtime operations `0x40` with the node, `0x0e` with null, and `0x20` with null respectively. Replacement flag `0x200` triggers parser lookup using the replacement node address as both owner and name pointer, and successful dispatch replaces the function's return value.

## 2026-08-16 - Tree auxiliary teardown unlinks before callback and free

- `RuntimeTreeNode +0xb0` is the head of a singly linked auxiliary-node list whose entries are 0x28 bytes and link at `+0x24`.
- `0x004071E0` publishes the next head before invoking the runtime callback as operation 7 with EDX zero and the node as its stack argument, then frees that same node from the runtime heap. Neither result affects traversal.

## 2026-08-16 - Section dispatch releases resources only after context creation failure

- `0x00405380` uses a four-argument fastcall ABI: resource identity in ECX, node identity in EDX, then section and creation strings on the stack.
- Missing nodes, missing already-loaded resources, and missing script sections return null without releasing anything. A valid section followed by parser-context creation failure alone calls resource removal; successful creation returns the parser dispatcher's result unchanged.

## 2026-08-16 - Parser reset follows property-10 include edges recursively

- `0x00405E00` always restores a parser context's cursor from its start offset before scanning properties through the `0xffffffff` terminator.
- Property code 10 resolves another runtime-tree node and recursively resets every context linked at node `+0x6c`; a failed resolution simply continues scanning. `0x00405DC0` is the identity-to-node wrapper over the same traversal.

## 2026-08-16 - Tree deactivation has a hidden second fastcall argument

- `0x00426600` saves incoming EDX at entry and later forwards it with the original ECX identity to `0x00405E50`; callers such as `0x00423740` explicitly clear EDX. Its ABI is therefore two-register fastcall despite the initial one-parameter decompiler display.
- Root-node deactivation walks primary script objects. Object flag `+0x42c` bit `0x10000` protects its visual; otherwise visual `+0x470` is destroyed via scene identity `+0x158`, removed, and cleared regardless of the removal result.
- Private message `0x7FFD/0x30000000` precedes cleanup and `0x7FFD/0xE0000000` follows downstream tree destruction. Flag `0x1000` or an empty node name clears script flags 2 then 4; flag `0x800` tears down comment mode.

## 2026-08-16 - Pending tree switches accumulate flags at context +0x90c

- Runtime-display flag `0x04000000` gates `0x004210A0`. The routine clears context DWORD `+0x90c` before destroying/activating trees, then ORs any flags accumulated there into the activated node at `+0x2c`.
- Null activation still rebuilds plans and refreshes the pointer. Same-identity activation additionally finalizes the current tree and returns false; different identity skips that finalizer and returns true. Every processed path clears the pending flag.

## 2026-08-16 - Comment-tree cleanup finalizes once after enumeration

- `0x00423740` visits the complete pointer-root enumeration and handles every node carrying flag `0x800`, calling resource destruction before node deactivation for each match.
- Its two global finalizers run once after enumeration only when at least one match was handled; no-match and empty enumerations return zero without finalization.

## 2026-08-16 - Runtime plan mode uses desired and applied bits

- Display-context flag `0x40000000` is the desired inactive-plan mode, while `0x80000000` records that the transition has been applied. `0x00421130` calls set only for desired=1/applied=0 and clear only for desired=0/applied=1.
- The applied bit changes even when the set/clear helper returns false. Runtime-plan rebuilding at `0x00426700` occurs only when that helper returns true.

## 2026-08-16 - Runtime pair dispatch consumes before suppression

- `0x004211A0` dequeues one pair before checking display flag 4, so a suppressed message is consumed rather than retained.
- Pair codes are Win32 mouse messages: `0x200` forwards unsigned low/high 16-bit x/y values; `0x201`, `0x202`, and `0x204` call their distinct handlers. Other codes return zero.

## 2026-08-16 - Comment CDF packages have fixed optional slots

- `0x004176A0` always creates a writer with capacity three, then conditionally emits `COMMENT.TXT`, `COMMENT.BMP`, and `START.CFG` in that order. Text size includes its NUL terminator; bitmap/config entries request compression.
- A bitmap is accepted only when its unaligned bit-count word at `+0x1c` is 8. Its serialized size is the unaligned width at `+0x12` times height at `+0x16`, plus the fixed `0x436` BMP prefix.
- On append failure, the archive-local error must be read before finalization destroys the archive. Writer construction failure returns fixed error `0x20000` directly.

## 2026-08-15 — Recover register-plus-stack startup ABIs from call sites

- On 32-bit optimized Win32 code, a function reached from WinMain may use a custom `__fastcall` interface even when decompiler parameter recovery initially resembles an ordinary C signature. Verify register setup, pushed argument order, and callee `RET` size together before assigning a prototype.
- Keep partially known aggregate state as an exact-size byte layout with only evidence-backed fields exposed. Offset and size assertions preserve confirmed ABI facts without inventing semantics for untouched regions.

## 2026-08-15 — Preserve staged legacy configuration failures

- A legacy registry loader can intentionally return the stage reached rather than the raw Win32 error. Preserve those staged values and perform later flag normalization even on failure paths.
- When multiple adjacent `MAX_PATH` buffers become evident, expose them at their confirmed offsets while leaving intervening state opaque. This enables exact path and registry tests without prematurely interpreting the whole owner object.

This file records verified, reusable architectural patterns. Append new patterns;
do not replace prior entries without correcting a demonstrated error.

## 2026-08-16 - ScriptParserState overlays the parser-context header

- The 0x28-byte `ScriptParserState` is not a separate opaque object: it exactly overlays the header of `RuntimeTreeParserContext`. Offsets `+0x00/+0x14/+0x18/+0x1c/+0x20/+0x24` are owner, generic resource, text, text length, start offset, and cursor.
- `0x004052F0` frees every context and balances resource references but intentionally does not clear the owner's context-head field. Callers destroy or otherwise invalidate the owner immediately afterward.

## 2026-08-16 - Resource token parsing retains semicolons but consumes delimiters

- `0x00405110` skips only CR/LF before inspecting a token. Encountering the caller delimiter or `;` returns `0x7fffffff` after clearing output but without committing even the skipped local cursor.
- Successful copying permits exactly `capacity` payload bytes and then writes the terminator at `output[capacity]`. A delimiter is consumed after the token, while a semicolon is retained as the current position for the caller's command loop.

## 2026-08-16 - Tree admission applies class uniqueness by current scope

- `0x00405410` reuses same-name nodes before parsing. A declared `class=TEMPLATE` is never instantiated; other declared class names must be unique across top-level siblings when no current node exists, or across the current node's complete ancestor-root enumeration otherwise.
- Parent selector `0xFFFFFFFF` creates a sentinel-parent root and prepends it to the global root chain. Ordinary creation appends to the current node's child chain or the top-level chain. Parser dispatch may return a node different from the allocated node; callback operation `0x30` applies to that returned node unless flag `0x8000` is set.

## 2026-08-16 - Generic resources separate identity, data, metadata, and position

- `0x00404EE0` invokes resource callback operation 6 with an in/out data pointer initially pointing at the extracted basename plus a separate metadata output. Its 0x3c-byte list node stores self identity at `+0x20`, data at `+0x24`, current position at `+0x28`, metadata at `+0x2c`, active references at `+0x30`, and next at `+0x38`.
- `0x00405210` creates 0x254-byte per-tree parser contexts keyed by a 32-byte name at `+0x230`. Creation updates the generic resource position/reference fields only after allocation succeeds and initializes pointers to its 260-byte inline creation/scratch buffers.

## 2026-08-16 - Wrapper calls can intentionally stage a callee's stack arguments

- `0x00426560` pushes its third and second inputs before calling one-register `0x00404EE0`; because that callee returns without stack cleanup, those pushes become the later `0x00405410` stack arguments. The wrapper also reloads its third input into EDX, so the third input reaches the constructor twice while the wrapper's fourth input is unused.
- `0x004210A0` passes adjacent 0x104-byte globals `0x0047F1AC` and `0x0047F2B0` in ECX/EDX and the pending node in both stack slots. Preserve this register-plus-staged-stack mapping rather than simplifying duplicated or apparently unused arguments.

## 2026-08-15 — Preserve loader-local output reuse

- Optimized resource loaders may reuse an output-count local as a later type discriminator. When the original read API writes that local and no explicit default assignment intervenes, preserve the resulting value flow instead of replacing it with a convenient inferred type.
- A legacy streamed-resource lifecycle can couple the zero-to-one and one-to-zero count transitions to a script-engine flag. Keep those transitions inside the same resource lock as open/close accounting.

## 2026-08-15 — Circular child lists retain three parent cursors

- Runtime named-child lists maintain head, inclusive tail/sentinel, and iteration cursor independently. Removal must update each matching parent field before splicing neighbors, and removing the sole self-linked entry clears all three fields.
- Cache metadata combines a high-word type/flags value with a low-word reference count. Decrement the full DWORD as the executable does, then test only the low word for final payload destruction.

## 2026-08-15 — Distinct backend families use distinct ownership locks

- Media backends use thread ownership plus a recursion count under one mutex, while generic parent/child backends use a single bit `0x10000` under another mutex. Preserve each contention protocol independently even when both retry with `Sleep(0)`.
- Destruction routines may acquire and permanently unlink a record without balancing its ownership marker first because the record is immediately freed. Do not insert a conventional unlock step absent from the original assembly.

## 2026-08-15 — Resource destruction has nonuniform success conventions

- The display-scene release result is intentionally inverted when folded into resource destruction: only a zero scene-release result preserves success. Verify comparison and `SBB`/`NEG` sequences instead of normalizing return conventions.
- Sound handles index a fixed 1024-entry table with slot zero reserved. Destroying the maximum handle scans backward through active flags and updates the maximum; destroying a lower handle leaves the maximum unchanged.

## 2026-08-15 — Stream constructor failure can intentionally skip restoration

- The animation stream constructor snapshots an async-record position and reads its header before allocation, but allocation failure returns immediately without restoring that position. Preserve this asymmetric failure side effect rather than adding cleanup symmetry.
- Proprietary animation signatures can share a constructor while supporting different storage modes asymmetrically. AF11 initializes memory offsets but is merely accepted with zeroed offsets in the streamed branch; AF12 initializes both.

## 2026-08-15 — Sound slots separate active and playable state

- Start and stop validate only the enabled subsystem and handle range; they do not require the slot's active flag. Queueing data and setting loop values do require active state, while direct slot lookup uses the fixed 1024-slot bound rather than the current maximum handle.
- Queued PCM descriptors retain caller-owned sample pointers. Replace mode frees descriptor nodes only, clears scheduling state, and installs a new descriptor without copying or freeing sample bytes.

## 2026-08-15 — Preserve legacy lock leaks and callback deferral

- A valid-looking setter can intentionally or accidentally return from its invalid-handle branch without releasing an acquired mutex. Preserve the assembly-visible leak and cover it as behavior instead of silently repairing it.
- The waveOut callback does not mix audio directly. It forwards WOM messages to a dedicated sound window, timestamps WOM_DONE, and lets the window thread schedule mixing; callback-side work stays minimal.

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

- Use the repository's current architecture target for routine development and
  runtime testing. Architecture-specific constraints from earlier recovery
  phases are historical and must not override the active build configuration.
- Release builds are optional unless release-specific behavior is under
  investigation.
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

## 2026-08-15 — Verify register-only wrapper arguments in assembly

- Tiny tail-call wrappers can mislead the decompiler about which register is the wrapper's own argument versus caller state retained in another register. Confirm the call-site register setup and wrapper instructions before assigning a prototype.
- Preserve redundant bounded scans when they are present in the original control flow. Even when two iterations currently traverse the same list, collapsing them requires evidence that no invoked routine can change observable state between scans.

## 2026-08-15 — Model archive pointer tables separately from entry storage

- A fixed archive context can own one contiguous entry allocation while exposing a separately located fixed-capacity table of pointers into that allocation. Preserve both fields and their offsets; replacing the table with a vector changes the binary-facing layout and index semantics.
- Entry-selection bytes may combine identity and storage flags. Compare the complete byte where the original requires selector equality, then independently test individual bits such as compression flag `0x10` when choosing the read path.

## 2026-08-15 — Separate container errors from delegated codec results

- A container reader may establish success from exact input cardinality and chunk traversal while ignoring a bundled codec's integer return because codec failures escape through a separate exception boundary. Preserve the caller's observed decision contract rather than inventing return-value checks.
- For chunk tables, distinguish the number of output chunks from the number of offsets. The GAG CDF reader stores one more offset than chunks and derives each compressed size from adjacent entries.

## 2026-08-15 — Preserve per-version archive layouts explicitly

- Shared signatures do not imply one normalized index representation. Model each confirmed version branch independently when allocation sizes, record bases, count widths, or compression chunking differ.
- A legacy index may include leading bookkeeping bytes before its first logical record. Preserve pointer-table bases such as `storage+4` instead of reshaping the allocation around the visible records.

## 2026-08-15 — Trace path buffers through every in-place helper

- A field initially labeled from one use may later prove to hold a path. Track the complete sequence of copy, directory truncation, append, and comparison operations before assigning semantics to a fixed application-state buffer.
- Apparently pointless string operations can be intentional clearing idioms. In GAG, copying the directory portion of `*.cdf` produces an empty string that is then appended while constructing the selected archive path; preserve the operation rather than folding it away without evidence.

## 2026-08-15 — Recover mixed register/stack ABIs from prologue spills

- A large function may spill an incoming register into its local frame before saving nonvolatile registers, while its apparent decompiler prototype exposes only a stack argument. Track the pre-push stack address through the complete prologue to recover hidden register parameters.
- Nested stage bits must remain nested when assembly branches establish that relationship. GAG's validation bit `0x400` has no effect unless bit `0x200` first enters alternate-mode detection; treating them as independent flags changes behavior.

## 2026-08-15 — Legacy bitmap blocks and palette ownership

- A `BITMAPINFO *` passed into an old Win32 renderer may point 14 bytes into a complete `BITMAPFILEHEADER` block. Confirm pixel offsets in assembly: GAG's custom control copies indexed pixels from `BITMAPINFO+0x428`, the 40-byte header plus 256 four-byte color entries.
- Preserve separate palette and bitmap ownership pairs. The custom control stores both its created object and the object previously selected into each DC; teardown restores the previous object before deleting the created one.
- For 8-bit system-palette controls, initialization and image upload have distinct responsibilities: initialization creates 236 `PC_NOCOLLAPSE` entries, while each upload replaces their RGB values, realizes the palette, and separately installs all 256 DIB color-table entries.

## 2026-08-15 — Fixed-width script identifiers

- Script names can be compared as complete fixed-width DWORD blocks rather than null-terminated strings. Preserve padding bytes and the original comparator's `byte_count >> 2` behavior; it intentionally ignores a trailing partial DWORD.
- A message payload can overlay different semantic records. GAG's private state-reference messages use separate 32-byte object and field names, while credits lifecycle handlers interpret offsets `+0x24` and `+0x2c` as activity and flags. Do not force both uses into one guessed structure.
- Verify wrapper arity from `RET n` and pre-call pushes. Several GAG synchronized wrappers looked like two-argument functions in decompilation but actually use ECX/EDX plus two or four stack arguments.

## 2026-08-15 — Legacy screenshot capture adapters

- Trace stack-local capture descriptors at instruction level when a wrapper decompiles to a misleading tail call. GAG builds a 0x28-byte descriptor from noncontiguous game-context fields and forwards retained ECX/EDX plus two stack arguments.
- Preserve the complete serialized bitmap layout rather than modeling only `BITMAPINFO`: the screenshot path returns a heap-owned BMP file image with a 14-byte file header, 40-byte info header, 256-entry palette, and pixels beginning at exact offset `0x436`.
- Half-resolution capture can be sampling rather than rescaling. GAG halves each dimension and advances through the original indexed surface with a two-pixel horizontal step and alternating source rows.

## 2026-08-15 — Availability flags and ring indices are independent

- Do not infer that a queue reset clears its availability flag. GAG's byte and pair reset helpers zero only read/write indices; a subsequent dequeue can still take the locked path, observe equal indices, and leave availability set.
- A fixed 32-slot ring that advances the read index on write collision has an effective capacity of 31. Preserve this overwrite policy and its exact wrap point rather than substituting a standard container with capacity 32.
- Similar adjacent rings can have materially different admission rules. GAG's byte and pair queues require combined flags `0x100400`, while its DWORD message ring requires only `0x400`, suppresses consecutive duplicates, and gives message `0x30f` destructive index-reset semantics.

## 2026-08-15 — Preserve resource cleanup splits around nullable setup

- A nullable lookup result does not imply every later branch tolerates null. GAG passes a null-derived context value into preparation, but a successful preparation then dereferences the original context unconditionally; retain that distinction instead of adding a broad safety guard.
- Track acquisition and release operands separately. The input-session initializer acquires a descriptor from a selected resource context, uses `descriptor+0x27c`, but releases the selected context rather than the returned descriptor.
- Polling loops can preserve a bit captured before initialization only on one terminal path. GAG restores original flag `0x100000` on cancellation, while completion clears `0x01000040` without restoring it.

## 2026-08-15 — Palette and display-target state are coupled but distinct

- Legacy 8-bit presentation can update 236 logical system-palette entries while always publishing all 256 colors to the DIB table. Preserve both counts and their separate conditions.
- Palette handlers may deliberately re-enter the same display lock through the palette application routine. Test seams must model reentrancy and exact enter/leave ordering.
- A target dispatcher can gate begin/end through one global while the target backend itself uses a separate palette/display-state global. Do not merge similarly named flag words without address evidence.

## 2026-08-15 — Display lock acquisition owns dirty-region publication

- A legacy recursive display lock can combine ownership arbitration with scene callback traversal and dirty-rectangle consumption. Preserve the busy-event loop, mode-specific wait behavior, recursion transition, and rectangle side effects inside one acquisition transaction.
- An empty pending rectangle may be represented as `{display_width, display_height, 0, 0}` after consumption. Globals used as reset sentinels can therefore also be the actual surface dimensions; check their other callers before naming them.
- Callback lists can run a probe pass and only perform a second state-changing pass when any probe returns zero. Preserve the first pass's aggregate-zero decision, the second pass's exact last return value, and early `0x10` termination.

## 2026-08-15 — Display-scene boundaries and mode-dependent callbacks

- When analysis creates an entry inside a prologue, verify the preceding alignment bytes, stack allocation, saved registers, and incoming call targets before trusting decompiler parameters. Deleting an overlapping mid-prologue function can expose the true existing function boundary.
- One callback address can intentionally accept different payload layouts selected by a mode argument. Preserve the mode-dependent ABI explicitly instead of assigning one over-specific pointer type to every invocation.
- A packed rectangle transform can add signed values only to coordinate low words while retaining each high word. Replacing that operation with ordinary 32-bit addition changes carry and wrap behavior.
- Preserve retry terminal ordering exactly: the display synchronizer sleeps after failures one through nine and returns failure immediately on the tenth, with no final sleep.

## 2026-08-15 — Scene-node locking and owner lists are distinct

- Do not merge a scene node's recursive thread lock with its logical owner list. GAG stores the lock count/thread at `+0x0c/+0x10`, while owner count, selected owner, and the 128-entry membership array begin at `+0x68`.
- An index allocator over a sorted node list can advance through consecutive occupied values while returning a lower gap unchanged. Preserve the original previous/current traversal condition, including its equal-value increment, instead of replacing it with an unordered membership search.
- A nullable clip-rectangle setter can mean clear the active clip only after verifying the caller owns the required display-lock mode. Null handling does not bypass the ownership gate.

## 2026-08-15 — Scene callbacks own lazy alternate buffers

- A callback-list node can be allocated together with an inline copy of opaque caller context. Preserve the exact `record_size + context_size` allocation and point the callback context immediately past the fixed record; a null source leaves the stored context null even when the requested size is nonzero.
- Non-passive scene callbacks lazily acquire one of two full-surface buffers: current first, alternate second, and no additional buffer once both exist. A passive `0x10000` callback bypasses buffer allocation entirely.
- If lazy buffer allocation fails after the callback record was created, decrement the node's callback-state count and free the new record without linking it. Existing callbacks and previously allocated buffers remain unchanged.
- A metadata query can return a compact 0x10-byte surface descriptor and a separate eight-DWORD callback block. On lookup failure, the original clears each requested output independently rather than leaving partial data intact.
- Pixel-stride fields must remain byte strides even in a 16-bit fill routine. GAG computes the row base with `y * stride + x * 2`, writes words, then advances each row by the original byte stride.

## 2026-08-15 — Format-specific compositors share clipping, not pixel policy

- Preserve separate original compositor entry points even when their coordinate clipping is identical. GAG selects among direct indexed copy, palette conversion, zero-index transparency, and opaque copy through stored function pointers; combining their externally visible identities would break teardown and format dispatch.
- Local compositor mode rewrites the caller's rectangle to destination coordinates, while global mode intersects scene positions without modifying that rectangle. This difference is observable even when both paths touch the same pixels.
- Palette conversion uses the mapping pointer stored at callback-state offset `+0x1c`. The 8-bit destination takes each mapped DWORD's low byte; the 16-bit destination takes its low word.
- Last-owner removal and object destruction are separate transitions. GAG first swap-removes the owner, clears the selected owner and format metadata, selects a fallback compositor, then decrements a reference only when `owner_count < reference_count`; destruction occurs only if that reference reaches zero.
- The root scene's sentinel index `0x7fffffff` blocks ordinary reference release. Shutdown flag `0x40000000` enables its destruction, clears the root global, and intentionally does not free the root's primary pixel pointer.

## 2026-08-15 — Scene reuse can destructively resize before allocation succeeds

- Preserve allocation flags separately from pixel-buffer flags. GAG allocates the fixed 0xA9C scene node with `HEAP_ZERO_MEMORY` but allocates its primary/current/alternate pixel buffers with flags zero.
- A resize path can free all old pixel buffers before attempting replacements. If any required replacement fails, GAG unlinks and frees the complete scene node rather than restoring the old buffers; do not make this transaction atomic.
- Scene references and logical owners advance independently. A successful owner attachment writes an owner-relative descriptor and increments both counts, while ownerless acquisition can return the existing node and increment only its reference count.
- Indexed palette matching does not choose the mathematically nearest color. GAG scans destination indices from zero repeatedly at maximum-channel-difference thresholds 0, 10, 20, through 260, selecting the first entry admitted at the first successful threshold.
- The fixed scene allocation contains two distinct 256-DWORD palette areas: source colors at `+0x29C` and derived mappings at `+0x69C`. Callback-state pointers at `+0x294/+0x298` identify those areas without changing the node's fixed ABI.

## 2026-08-15 — Preserve lifecycle flag transitions across helper gates

- A caller can intentionally clear an active bit immediately before invoking a helper whose public gate requires that bit. Preserve both functions independently even when the call then appears ineffective; do not silently reorder the transition or broaden the callee gate to make cleanup look conventional.
- Worker-thread arguments can point at the base of a contiguous global state block. Array-style offsets in decompilation may therefore denote globals such as callback, interval, rate, root, and list head rather than a separately allocated runtime object.
- Frame pacing advances its target timestamp by the requested sleep remainder, not by re-reading the clock after sleeping. This distinction prevents accumulated drift and is observable with a deterministic time seam.

## 2026-08-15 — Legacy GDI teardown restores selected objects first

- A DIB/palette teardown can be guarded by both an initialization bit and a backend-mode mask. When the backend resources exist, restore the previously selected palette and bitmap before deleting the active objects, then delete/release their owning DCs in the original order.
- A presentation-busy flag may be checked before and immediately after entering the display critical section. Preserve the leave-and-retry race path rather than treating the initial check as sufficient synchronization.

## 2026-08-15 — Cooperative-mode HRESULT and API result can differ

- A wrapper around a DirectDraw cooperative-level transition can return success for every attempted transition while using the HRESULT only to decide whether to mutate its internal mode bit. Do not propagate the COM failure when the original wrapper deliberately suppresses it.
- Child-window display ownership is normalized by repeatedly following `GetParent` while `WS_CHILD` is present. Apply this traversal before both normal and exclusive cooperative-level calls.

## 2026-08-15 — DirectDraw and GDI rectangles can use different edge conventions

- GAG's DirectDraw copy helper constructs inclusive right/bottom coordinates (`x + width - 1`, `y + height - 1`) for both BltFast and Blt, while its GDI calls pass width and height directly. Preserve the backend-specific convention.
- A legacy DirectDraw effects structure may be represented as exactly 25 DWORDs: zero all 100 bytes, then set only the first DWORD to 100 before the Blt call.

## 2026-08-15 — Surface creation can return a backend sentinel

- GAG's DirectDraw surface creator returns pointer sentinel `0xFFFFFFFF` rather than a mapped pixel address; the GDI branch returns the DIB section's real pixel pointer. Callers use only zero/nonzero at this layer and publish backend-specific resources separately.
- Flip-chain failure is not terminal: restore cooperative mode only when it was not already exclusive before the call, release any primary surface, clear only option bit `0x100`, then retry with a primary surface plus an offscreen surface.
- Preserve legacy bitmap-header values even when they look unconventional. The GDI path sets `biClrUsed` to `0x10000` for 5-6-5, `0x8000` for 5-5-5, and `0x1000000` for 32-bit bitfields.

## 2026-08-15 — Runtime identities can double as lock records

- A named runtime node's identity pointer can point directly to a larger scene record whose prefix contains thread owner and recursion count. Lookup, logical identity, locking, and scene state are therefore layered over the same address rather than separate maps.
- Contended acquisition releases the global critical section before sleeping 5 ms and retries the lookup from the beginning. Same-thread acquisition increments recursion; release decrements only the count and intentionally retains the owner thread value.
- Scene switching parks the old scene at `(10000,10000)`, but publishes the requested identity only for recognized mode masks `0x1000` or `0x2000`. An acquired record with another mode is released while the current identity becomes null.

## 2026-08-15 — Shutdown success can require multiple independent zero returns

- Runtime shutdown begins only after two named runtime-list status fields are both zero. The lookup helper creates a missing list node, so the original assumes non-null identities after lookup rather than treating absence as a shutdown shortcut.
- After thread close succeeds, scene release and host shutdown each contribute an equality-to-zero boolean. Surface teardown still runs when either later boolean fails, but global state blocks and active flags are cleared only when their conjunction remains true.
- A zero/nonzero handle close result is part of application-level teardown success, not merely best-effort resource cleanup. Preserve its gating effect on all downstream teardown calls.

## 2026-08-15 — Plan lists can have an inclusive logical terminal

- A linked plan list may continue beyond the currently active logical range. GAG walks from the head and stops only after processing the explicit terminal node, so nodes linked after that terminal must remain untouched.
- Root activity bit `0x20` changes even when the terminal pointer is null. In that case no plan nodes are traversed and the function reports that no per-plan bit changed.

## 2026-08-15 — Runtime tree lookup is child-first depth-first

- The recursive runtime tree finder compares the current node, searches its entire child chain, and only then advances to the current node's sibling. A flat list lookup or sibling-first traversal can choose a different object when identities are reused.
- A tiny global wrapper can be a tail-call that supplies the tree root from a confirmed runtime offset; preserve it as its own original-address function rather than folding it into callers.

## 2026-08-15 — Root-local runtime tree traversal

- GAG stores traversal state inside the selected tree root rather than in a separate iterator: current identity at `+0x64` and an ascent flag at `+0x68`. Concurrent traversals of the same root therefore overwrite each other's state.
- The enumeration order is post-order below the selected root. It descends through children, returns leaves, ascends through parent links, and treats parent pointer `-1` as a terminal sentinel alongside the selected root.
- Scene position mutation first preserves the old coordinate pair in dedicated fields and passes the new-minus-old delta to the display scene. Tests should assert both the stored snapshot and the downstream delta, not just the final coordinates.

## 2026-08-15 — Default-comment scene activation

- Script root `+0xf80` is a separate visual-object list from state objects at `+0xf7c`. Its nodes share an inline name and `+0x24` link pattern but expose a scene identity at `+0x158`; do not collapse these two lists into one structure.
- Default-comment activation has three distinct results: gated/no action returns zero, missing named object returns `-1`, and a found object returns one even when the saved scene and active bit suppress the actual scene switch.
- A higher-level node activation bit is set only after the lower-level activation returns positive. It must not be inferred merely from the node having a parent or from the runtime-enabled flag.

## 2026-08-15 — Pointer-region selection and event publication

- Pointer regions use inclusive bounds and resolve overlap by the greatest unsigned priority at `+0x4c`. List order wins ties because replacement occurs only for strictly greater priority.
- Scene selection rotates a one-bit cursor for at most 32 attempts. A scene-slot flag `0x20` makes that slot conditional on the same bit being present in the region's state-object active mask; an unrestricted slot ignores that state mask.
- The runtime event queue stores 32 records of exactly 0x40 bytes inline at root `+0x0c`. When the write index catches the read index after wrapping, the read index advances, so the newest record is retained and the oldest is discarded.
- Pointer-region mode bits are not a simple enum setter. No-hit handling clears `0x38000` only outside mode `0x20000`; mode `0x30000` combines the region scene mask with its state-object mask to decide whether `0x8000` is cleared or set.

## 2026-08-15 — Runtime resource cache lists

- Runtime named-node child lists used for resource caching are circular and doubly linked. Parent `+0x44/+0x48` are head/tail, and cache entry `+0x2c/+0x30` are next/previous; lookup remains sentinel-bounded rather than relying on a null link.
- The first insertion self-links the entry and initializes parent head, tail, and `+0x4c` cursor. Later insertions splice between the old tail and head and update only the tail.
- Cache keys are copied as exactly 32 bytes, not as a variable-length C string. Callers must provide the full fixed-width key buffer even though lookup compares the stored key as zero-terminated text.
- Resource path construction retains a directory embedded in the requested path. The global current resource directory is used only when the requested path has no directory component.

## 2026-08-15 — Loose-resource compatibility opening

- The original deliberately varies `CreateFileA` flags by `dwPlatformId`: Windows 95/98 receives `0x28000000`, while other platforms receive `0x08000000`. Preserve this distinction even on modern NT systems.
- The loose-resource opener normalizes only `INVALID_HANDLE_VALUE` to null. It does not use `GetLastError` or reinterpret other handle values.
- Drive-prefix extraction is not a general path parser: it scans at most 12 bytes and considers only a colon a successful result. Backslash, NUL, or length exhaustion returns zero without guaranteeing a destination terminator.

## 2026-08-15 — Asynchronous file ownership bits

- Async host and file records use bit `0x10000` as a spin-acquired ownership marker protected by one global critical section. Contention releases the section and calls `Sleep(0)` before restarting the full list search.
- File flag `0x2` denotes shared-handle ownership: acquiring one such record marks every sibling with the same handle, and releasing it clears every matching sibling. Records with different handles remain untouched.
- The enable-gate asymmetry is original behavior: host acquisition and both file operations check it, while host release always enters the global critical section and attempts to clear the bit.

## 2026-08-15 — Async open-record lifetime

- An async file record owns a fixed 0x8000-byte VirtualAlloc buffer regardless of requested file range. A zero requested end substitutes the complete file size; nonzero ranges are stored without bounds checks, and remaining size uses unsigned subtraction.
- Open failures are asymmetric: invalid file performs no allocation cleanup, record-allocation failure closes only the file, and buffer-allocation failure frees the record and closes the file. Host ownership is released in all cases after host acquisition succeeds.
- Flag `0x2` makes multiple records share an OS handle lifetime. Removing a record does not free that record, buffer, or handle while any same-handle sibling remains; when exactly one remains, the survivor's `0x2` bit is cleared so its later close performs destruction.

## 2026-08-15 — Async circular-buffer accounting

- Producer and consumer movement account sectors asymmetrically. Producer completion subtracts every completed byte from available capacity and wraps at or beyond the end; consumer movement returns only newly crossed complete sectors to available capacity and wraps by subtracting one full buffer size.
- The async host allocation uses two consecutive truncations: first `requested / 0xffff * 0xffff`, then division/multiplication by bytes per sector. Preserve this exact expression rather than using a conventional alignment helper.
- A thread entry referenced by `CreateThread` may remain undefined data in Ghidra. Verify executable bytes and prologue/call target first, then create the function boundary before decompiling; do not substitute a placeholder thread routine.

## 2026-08-15 — Disk geometry and dual async read paths

- Confirm `GetDiskFreeSpaceA` output fields from argument order, not nearby provisional names. In GAG's async host, bytes per sector is `+0x44` and sectors per cluster is `+0x48`; sector-alignment code consistently reads `+0x44`.
- One async record reader has two distinct storage paths. The active/forced path consumes the host circular buffer under the secondary lock, while an inactive record uses its private 0x8000-byte buffer and OS file position without taking that host lock.
- A successful OS read that returns zero bytes is EOF and retains the reader's success result; a failed `ReadFile` returns failure. Preserve this distinction along with the partial byte count and current-offset update.
- Host destruction deliberately acquires the host without releasing its ownership bit, unlinks it, sets termination bit 1, and waits for the worker before deleting locks and storage. The ownership bit becomes irrelevant only because the object is destroyed.
# Sound readiness can report accepted initialization independently of readiness

- `EnsureRuntimeSoundReady` (`0x004010b0`) returns zero only when global sound support is disabled. After entering the enabled path it always releases the lifecycle mutex and returns one, even when thread creation fails or mixer initialization fails and tears the new thread back down. Callers must inspect the separate fault/ready globals where they need actual output status; do not normalize this result into a conventional success/failure boolean.

# The sound worker dispatches negative GetMessage results

- `RunRuntimeSoundThread` (`0x00402100`) tests `GetMessageA` only against zero, not against `-1`. Consequently a retrieval error is passed to `DispatchMessageA`, and the loop calls `GetMessageA` again. Preserve this literal loop in worker/message-thread recovery instead of applying the conventional `> 0` predicate.

# WOM_OPEN immediately enters the same two-buffer pump as WOM_DONE

- `RuntimeSoundWindowProcedure` (`0x004021e0`) prepares and marks both headers done for WOM_OPEN, then falls into the exact pump shared with WOM_DONE. This causes both buffers to be mixed and submitted during successful open. The output format copy covers fields through `wBitsPerSample` only; the adjacent `cbSize` word is not copied.

# PCM mixer variants differ in their exact buffer-clear granularity

- The four mixer callbacks share playback logic but do not clear arbitrary trailing bytes uniformly. 8-bit mono clears every byte to `0x80`; 8-bit stereo clears complete words to `0x8080`; 16-bit mono clears complete words to zero; and 16-bit stereo clears complete DWORDs to zero. Preserve untouched trailing bytes when the buffer length is not aligned to the variant's clear width.
- The fastcall mixer argument is the `HWAVEOUT` value forwarded from the hidden window procedure's `wParam`. It is recorded in per-slot playback/schedule marker fields; it is not an output-buffer index.

# Capacity scans may intentionally cross a global-table boundary

- `CreateSoundHandle` scans active fields only through slot 1023 but its final `JBE` accepts computed handle 1024. The ensuing 0x34-byte slot writes begin at `0x0043e048`, aliasing unrelated sound globals; do not normalize this to a conventional full-table failure or invent an independent legitimate slot without reproducing the alias side effects.
- A short Ghidra function immediately followed by a second function at fallthrough can be a false split. Verify the raw bytes and connected control flow; for `0x00401820`, clearing flow and repairing the complete verified range merged spurious `FUN_0040182b` and restored the real body through `0x00401ba1`.

# Resource palette publication has distinct global and scene paths

- Bitmap resources whose type bit 0 is clear publish 256 palette entries to their scene only after primary-owner assignment succeeds. With type bit 0 set, owner-assignment success is ignored: 236 entries are always published globally, then 256 may also be published to the scene.
- Backend/resource flag `0x04000000` suppresses the scene publication only at exactly 8-bpp. Display depths above 8 override that suppression; preserve the strict `> 8` comparison.

# Bitmap remapping and publication preserve legacy edge behavior

- `BuildPaletteIndexRemap` scans comparison colors in stable index order and increases an 8-bit tolerance by ten until a candidate matches. Candidate count is 236 at exactly 8-bpp and 256 otherwise; the tolerance deliberately wraps as a byte.
- BMP conversion uses the signed height literally: positive height writes bottom-up by beginning at the last destination row, while negative height writes top-down after negating the height. Source rows retain four-byte BMP padding independently of destination stride.
- Bitmap finalization clears pending bit `0x20` before conversion and reloads flags afterward. A converter may restore that bit, enabling the 236-entry palette realization path; do not simplify the apparently contradictory post-conversion test.
- Adjacent Ghidra functions separated at a fallthrough/prologue-like byte sequence can still be a false split. `0x0042b300` required merging the spurious `0x0042b326` body before its complete finalizer control flow became visible.

# Animation presentation uses callback handshake bits

- The default animation callback treats flags `0x10000000` and `0x20000000` as immediate acknowledgements: it clears only the observed bit and returns before palette or dirty-rectangle work. The worker brackets decoder dispatch with these callbacks.
- At 8-bpp, palette dirty bit `0x4000` selects `AnimatePalette` when bit `0x20` is clear, or `SetPaletteEntries` plus `RealizePalette` when it is set. Both paths then publish the 256-entry DIB color table unless suppression bit `0x10` is set.
- Dirty rectangle bit `0x8000` presents either exact-size pixels or a precise 2x destination rectangle when bit `0x200000` is set. Origin words are added to source coordinates; the scaled destination doubles only the dirty offsets and extents before adding the origin.

# Generic text parsing preserves mode-dependent delimiters and position side effects

- The generic parser treats `:` as the normal delimiter and `#` when flag `0x1000` is set. Its integer scanner advances to the next digit before accumulating, so an apparent leading minus sign is skipped rather than applied; preserve this even though the subsequent sign branch becomes effectively unreachable for ordinary input.
- Generic-text destination coordinates are loaded by zero-extending their 16-bit storage. Do not infer signed screen coordinates from the source field declaration; newline reset repeats the same zero-extension.
- Runtime media backend identities normally point to their owning backend and are also the tokens passed to recursive lock-release routines. Callers release the stored identity, not a temporary pointer returned by acquisition.

# Resource playback queries normalize unlike backends into a shared flag vocabulary

- Bitmap and animation resources return their media backend flags unchanged. Sound resources instead synthesize `0x01000000` as the type marker, bit 0 from `playing`, `0x2000` when playback state is nonzero or both schedule and playing are zero, and `0x400` when the first loop value is `-1`.
- Resource destruction request `0x00425bd0` deliberately does not release the acquired resource lock before immediate destruction; animation is the only recognized type that sets backend flag `0x10000` and then explicitly releases the record.
- The game-DLL unload path ignores `FreeLibrary`'s return value. Loaded flag `0x10` is always replaced by `0x20`, and state flag `0x1000` controls an additional pre-flag-update state-exit call.
- Game-DLL loading retains the module and every resolved ordinal even when one ordinal is missing; it does not call `FreeLibrary` on this failure path. Once the module loads, all three ordinals are queried regardless of earlier lookup failures. The loading scene and both queue resets run after every attempted load, including LoadLibrary failure, but are skipped when flag `0x10` already indicates a loaded DLL.
- Ordinal 1 is an x86 `__fastcall` boundary: the 0x40-byte host context is in ECX and the adjacent 35-pointer callback table is in EDX. A generic Ghidra function-definition type without a fastcall convention breaks the indirect-call decompile; retain the global as a pointer with an explicit ABI comment unless the function-definition data type can encode the convention.
- Ordinal 3 is also fastcall and receives commands exclusively in ECX: 1 stops, 2 pauses, and 4 resumes. The stop wrapper timestamps only after issuing command 1, then tests the loaded bit before calling `timeGetTime` on every poll; elapsed time uses wrapping unsigned DWORD subtraction and sleeps only while elapsed is strictly below 5000.

# Historical FLIC logic must still be reconciled with executable-specific branches

- The historical FLIC player is a decoder reference only after mapping the worker's chunk switch to original addresses. GAG maps types 4/5/7/8/15/16 to `0x00415e60`/`0x00416420`/`0x00416da0`/`0x00415ee0`/`0x00416ad0`/`0x00416900`.
- GAG's COLOR_256 assembly applies the skip byte only when the count byte is nonzero. A zero count expands to 256 colors without advancing either palette destination first, unlike the historical player's unconditional skip accumulation.
- Chunk handlers 11, 12, and 13 at `0x0042b820`, `0x0042b830`, and `0x0042b840` are each literally one `RET`. In particular, do not import the historical BLACK clear implementation for chunk type 13.
- LITERAL and BYTE_RUN apply scale factors directly while decoding: source pixels are palette-remapped unless flag `0x04000000` is set, horizontal pixels are repeated by scale X, compressed rows are re-decoded for each scale-Y repetition, and the published dirty rectangle covers the full scaled frame.
- MVZ short and long references copy from the already decoded destination surface, not from encoded input. Reference coordinates are relative to the MVZ area's origin; scaling multiplies both reference coordinates and copy lengths, and forward bytewise copying preserves overlapping-reference expansion.
- MVZ8 terminates rows by its explicit packet count and applies an x skip before every packet. MVZ5 has neither field and terminates when decoded logical x reaches the area width. Both always publish the complete declared MVZ area as their dirty rectangle, even where MVZ8 skips pixels.
- DELTA_FLC differs from the full-frame decoders by expanding an existing dirty rectangle. It updates the left bound after each packet's scaled x skip, the right bound after packet output, the top bound for every physical scaled row, and the bottom bound only when all encoded logical lines finish. An EOL-pixel command assigns the full scaled width to the right bound before later packet maxima.
- For scale Y greater than one, DELTA_FLC replays the complete saved logical-line command stream for every physical destination row. Leading line-skip controls are consumed before that saved replay point, so their displacement is multiplied by scale Y only once.
- The animation backend's `+0x9b4` field is frame duration, not framebuffer stride. Destination stride is the 16-bit field at `+0x928`; `+0x9b4` participates exclusively in worker timing and AF11/AF12 header-speed conversion.
- Global animation pause is a single shared bit (`0x01000000`) at `0x00442198`. The setter and clearer are independent one-instruction-style functions that preserve every other global control bit; worker pause logic ORs this global condition with per-backend flag bit 1.
## 2026-08-16 — Recover large workers through exact control-flow phases

- For very large worker functions, exact non-original phase helpers are useful only when their boundaries follow contiguous original control-flow regions and the original entry remains unresolved until all phases are integrated. Test the original signed/unsigned comparisons and persistent flag side effects at each phase boundary.

## 2026-08-16 — Repair Ghidra structures without shifting confirmed offsets

- Removing a defined Ghidra structure component collapses later components toward the deletion point. When replacing a large opaque range, remove every later component first, then rebuild the entire suffix at explicit offsets and verify the final layout and size before saving. For some imported arrays, inserting an equal-sized byte array and then changing its type is more reliable than inserting the imported array directly.

## 2026-08-16 — Verify switch-join constants in assembly

- Optimized x86 switch joins can leave Ghidra associating code addresses or stale registers with `HeapAlloc` flags and sizes, and can hide stack-slot assignments from one case. Verify each call's actual pushes and each supposedly uninitialized local's defining writes in disassembly before accepting decompiler constants or dataflow.

## 2026-08-16 — Palette-remapped scene blits use function-local mapping storage

- `BlitBitmapWithOptionalPaletteRemap` copies the source's eight-word callback state and replaces word 7 with a pointer to a function-local 256-entry mapping table before building a non-root palette conversion. The compositor consumes that temporary state before the function returns; do not redirect word 7 to persistent destination palette storage.
- The blit acquires the source scene before the destination and releases the destination before the source. Even unsupported pixel-format combinations still perform both matching end-update calls after successful acquisition.

## 2026-08-16 — Validate tiny prologue functions as possible false splits

- A short block ending immediately before register saves can be the leading tests of the following apparent function. At `0x004231e0`, a `TEST`/branch prologue and the body Ghidra had placed at `0x004231eb` share one stack frame and one return convention. Verify fallthrough, incoming flags, stack references, callers, and the combined decompile before counting either boundary.
- Runtime DLL window callbacks use a sentinel return (`0x10000`) to consume host messages. The sentinel does not imply one uniform return: the host still translates and forwards mouse coordinates and returns 1 specifically for message `0x30f`.

## 2026-08-16 — Recover hidden custom-ABI initializer arguments from pushes and registers

- Ghidra can omit arguments for custom fastcall callees and register-based string copies. At `0x0041f4f0`, assembly proves `InitializeGraphicsHost` receives instance in ECX, window in EDX, then x/y/width/height/`0x300000` on the stack; `CopyString` receives destination in ECX and source in EDX. Read the pointer globals to recover exact strings rather than inferring them from nearby resources.
- The initializer centers content with logical `SHR`, not signed division. Cast the potentially negative rectangle difference to the original 32-bit unsigned value before shifting, then convert back to the stored signed field.

## 2026-08-16 — Preserve partial subsystem initialization leaks

- Startup subsystem initializers often use an initialized flag only after every setup call succeeds. `InitializeRuntimeMediaBackend` deliberately leaves its newly created heap live when mutex creation fails, because the flag remains clear and a later call creates and overwrites another heap. Do not introduce rollback absent from the executable.
- The runtime-node enable helper changes only bit 0 at node `+0x24`; it does not use the separate status word at `+0x40`. Keep both fields distinct even though both describe node state.

## 2026-08-16 — Legacy display discovery merges Windows and DirectDraw evidence

- DirectDraw mode descriptors are accepted only when descriptor flags contain all bits `0x1006`. Duplicate identity is exactly width, height, bits per pixel, and green mask; duplicate updates replace pixel-format flags and set availability bit `0x20000` without changing the count or other stored fields.
- DirectDraw initialization uses staged non-HRESULT status values: `0x10000` for version-query failure, `1` for unsupported platform or missing DLL, `0x100000` for a missing export, `0x200000` for creation/enumeration failure, and `0` for success. The display coordinator treats `1` and `0x100000` as successful Windows-mode fallbacks, but not `0x10000` or `0x200000`.
# Callback tables can reveal missed function boundaries

- Treat every executable callback pointer as a boundary candidate even when Ghidra initially shows it inside padding or adjacent code. The `InitializeGraphicsHost` table exposed six independently padded original functions and changed the authoritative recognized-function denominator.
- Tiny CDF accessors are not interchangeable with format-tool behavior: `0x00428710` returns the archive-wide field at `+0x138`, despite an initially misleading size-by-index name. Use the executable instruction offsets as the semantic authority.
- Returned state views may overlap later ABI structures rather than being standalone allocations. For graphics host `0x0041FA00`, the game-DLL context begins at state `+0x458`; therefore its bpp field at context `+8` is the same storage the application reads at result `+0x460`. Model the shared backing storage explicitly.
- When a typed Ghidra decompile and assembly disagree on a structure field, use the instruction displacement. Runtime bootstrap 0x0041FEA0 reads scene-node +0x1c three times; the existing structure identifies that as callback_first_position, while the decompiler incorrectly rendered the later +0x278 callback field.

## 2026-08-16 — Count fastcall stack arguments from raw offsets and RET cleanup

- `ConstructRuntimeResourceObject` has ECX/EDX plus six stack arguments and `RET 0x18`. Saved caller registers beneath those arguments are not constructor inputs. In the animation branch, raw offsets after the four callee pushes map `[ESP+0x268/+0x26c/+0x270]` to formal width, height, and scale-or-loop.
- Callback signatures installed into binary-owned tables must include ignored register parameters. The script setter is property in ECX, ignored context in EDX, and value on the stack; omitting context changes the ABI even though the implementation never reads it.

## 2026-08-16 — Reverse tree scans require the previous-sibling field

- The runtime tree's three “last link” searches descend to the last child through `next +0xb4`, then scan siblings backward through `previous +0xb8`, recursively preferring the deepest non-null tail before the current node's own tail. A structure ending at `+0xb8` silently loses this traversal; the confirmed node size is 0xbc.
- Preserve executable use of uninitialized stack slots as an explicit edge fact. `0x00425C40` initializes only its scene identifier before acquisition; missing or non-bitmap/non-animation records still pass the other four existing stack words to the region updater. Initializing them for cleanliness changes observable behavior, while ordinary C++ tests cannot make their values deterministic.

## 2026-08-16 — A NOP does not justify a function split

- `0x0041B1F0` was split immediately after a NOP at `0x0041B1FA`, but the following pushes continue the same allocation and both exits restore the same frame with `RET 4`. Delete the inner function, disassemble the contiguous bytes, then recreate the outer entry so Ghidra follows the full flow; a clear-flow pass alone may leave the short outer body unchanged.
- The 8-bit bitmap region copier uses the literal source-row adjustment `2*width - aligned_stride - copy_width`, not the conventional `aligned_stride-copy_width`. Preserve the assembly expression even though it looks like an alignment-sign error.

## 2026-08-16 — Runtime transition selection preserves sparse-mask edge cases

- `SelectRuntimeSceneTransition` intersects available, requested, and `0xfff` masks. A combined selected value of 3 is not decomposed and performs no transition. Random fallback is entered only when selection is zero, availability is nonzero, requested low bits are not exactly one, and the high override bit is clear; it chooses one of bits 1/2/4 and cycles until available.
- `ApplyImmediateRuntimeSceneTransition` intentionally returns without releasing an acquired animation record when its type bits `0x3000` are both clear. Preserve this lock leak. Animation state transition augmentation is selected only when the 16-bit frame number is exactly one, as proven by the `DEC`/`CMP`/`SBB` sequence.

## 2026-08-16 — Palette transition stack bytes have distinct roles

- In `ApplyPaletteRuntimeSceneTransition` (`0x00426F40`), `[ESP+0x12]` after saved registers is an activation byte initialized to zero and incremented only by a supported transition setup. The caller's low ECX byte survives separately at `[ESP+0x13]` and controls RGB subtraction or the fade-in pass count. Do not merge these overlapping decompiler locals: missing resources and failed display acquisition never animate merely because the caller step is nonzero.
- The transition copies 257 DWORD-sized palette entries from backend `+0x1c`, but passes only the first 256 to `ApplyDisplayPalette`. Both fade loops mutate entries 1 through 236, preserving entry zero. Fade-in uses wrapping byte increments: a temporary channel initialized to original+1 advances until zero, after which the destination channel advances; the temporary flags byte counts 255 passes.

## 2026-08-16 — Rectangle transitions use asymmetric clocks and operations

- `ApplyRectangleRuntimeSceneTransition` quantizes a non-`0xff` byte to horizontal step `(value & 0xfc)+4` and vertical step `3/4` of that. Closing uses `timeGetTime` and clears four strips with surface operation mode 2; opening uses `GetTickCount` and dispatches four newly exposed rectangles. Preserve this clock asymmetry and the exact strip partitioning.
- An acquired record without type bits `0x3000` falls through the immediate-transition path without release, matching the neighboring immediate transition's leak. Size `0xff` means full host dimensions and causes opening to publish the full rectangle without entering its expansion loop.
- `SynchronizeDisplayRegion` differs from `OperateDisplaySurface`: its DirectDraw calls receive the rectangle's right/bottom bounds literally, whereas the surface operation constructs inclusive `right-1/bottom-1` bounds from x/y/width/height. Its mode-2 effects block is exactly 25 DWORDs with the first DWORD set to 100.

## 2026-08-16 — Runtime target bounds are a mode-dependent ABI union

- `UpdateRuntimeTarget` mode 1 treats its four-DWORD block as a `DisplayRectangle`. Mode `0x10000` instead passes DWORD `+0x0c` in ECX as a pixel-pointer output address, `+0x04` in EDX as a rectangle output address, and `+0x08` on the stack as a pitch output address. Keep the storage generic and use an explicit adapter; field names such as width/height are not valid across modes.
- `BeginDisplayTarget` sets active bit `0x40000000` before surface access. A DirectDraw Lock failure or missing GDI pixel buffer clears it, but a failed Restore after `IsLost == 0x887601c2` returns `0x200000` without clearing the bit. Preserve this asymmetric failure state.

## 2026-08-16 — Display mode changes preserve backend-specific state

- DirectDraw mode changes are temporarily bracketed by cooperative mode `0x1000`
  only when that bit was not already set. Cleanup is skipped if entry fails, and
  its return value is ignored after the display operation.
- The Win32 path copies mode offsets `+0x08`, `+0x0c`, `+0x10`, `+0x18`, `+0x1c`,
  and `+0x28` into the corresponding 0x94-byte ANSI `DEVMODEA` fields.
- In `SwitchDisplayModeIfEnabled`, nonzero EDX selects the stored alternate mode
  and zero restores current mode. Name parameters from branch evidence.

## 2026-08-16 — Display host shutdown owns the mode list

- `ShutdownDisplayModeHost` frees the display-mode list itself, following the
  confirmed `+0x3c` next pointer. It calls palette/surface teardown first, deletes
  the critical section after all list nodes are freed, and finally zeroes the
  original contiguous 0x488-byte display-state region.

## 2026-08-16 — Graphics shutdown aggregates with bitwise AND

- `ShutdownGraphicsHost` calls generic-backend, async-file, and media-backend
  shutdowns unconditionally after display shutdown succeeds, combining their
  results with bitwise AND rather than short-circuit logic. Display-mode teardown
  also runs even when that aggregate becomes zero.
- Five critical sections, the resource heap, and the child window are touched only
  when all four subsystem results are nonzero. The child window is destroyed even
  when `HeapDestroy` fails; the 0x1d7c-byte host reset and scene-bit `0x800` clear
  happen only after heap destruction succeeds.

## 2026-08-16 — Full display clear publishes after unlocking

- `ClearRuntimeDisplay` performs clip reset and mode-2 surface clearing while the
  display lock is held, releases that lock, and only then calls
  `UpdateDisplayRootRegion` with the full runtime host rectangle.

## 2026-08-16 — Generic child availability is list-order dependent

- `FindAvailableRuntimeGenericChild` scans parent backends and children in list
  order under one mutex acquisition. A child is eligible only when flag mask
  `0x300` is clear and its `+0x8c` end position is no greater than the caller's
  threshold; it returns the child's identity, not the child record.

## 2026-08-16 — Generic child selectors are numeric-or-name unions

- `CreateRuntimeGenericBackendChild` treats a selector with a zero high word as a
  16-bit numeric default and adds flag `0x1000`. Otherwise the same DWORD is a
  text-name pointer used to locate entry, control, and text offsets.
- If named entry lookup returns -1, the original function still checks an
  uninitialized default-selection stack slot after locating control/text offsets.
  Preserve that edge rather than assigning a defensive failure sentinel.

## 2026-08-16 - Generic child state arrays overlap scene inputs by fixed indices

- `BuildRuntimeGenericBackendChildState` writes fifteen DWORDs. Scene processing
  uses DWORDs 2/3 as x/y and reinterprets DWORDs 11-14 as a rectangle.
- The descriptor and two-DWORD context are separate outputs. When context scene
  index 1 is zero, scene processing obtains an index with flag `0x80000` before
  acquiring the scene.

## 2026-08-16 - External command completion is synchronously polled

- Runtime context `+0x454` is the LPARAM sent with message `0x7FFD` and WPARAM
  `0x02000000`; `+0x928` is the pending flag polled after a nonzero SendMessage
  result.
- Each poll processes one runtime message, OR-accumulates the command-loop return,
  then sleeps 10 ms. Both the pending flag and scene-control bit `0x200000` are
  cleared even when the synchronous message returns zero.

## 2026-08-16 - Resource loop counts are type-dependent

- `SetRuntimeResourceLoopCount` recognizes sound resources only when
  `(type_flags & 0xFF000) == 0x8000`; those forward the original count to the
  sound backend.
- Every other resource sets backend media flag `0x400` and stores unsigned
  `count - 1` in both frame-limit DWORDs at `+0x50` and `+0x54`, including the
  intentional `0xFFFFFFFF` result for a zero count.

## 2026-08-16 - Runtime-tree name searches have three distinct scopes

- `0x00406640` searches a selected node and sibling chain child-first and
  recursively; `0x004066C0` searches descendants of a selected root but never
  compares that root; `0x00406720` compares only global-root siblings.
- All three compare exactly the first 0x20 bytes of each node and return the
  identity stored at `+0x20`, not the node address unless those happen to match.

## 2026-08-16 - Script object identifiers and fields are fixed-width blobs

- Script-object name and field lookup compares exactly 0x20 bytes; callers and tests must provide fully padded 32-byte identifiers rather than ordinary short C string literals.
- Identity lookup checks the root's primary object list first, then each container's 0x0c-byte slots. Object identity is the pointer at `+0x20`; container count and slots begin at `+0x30` and `+0x34`.
- Integer and string fields share the field-name index: integer values begin at `+0x484`, string values use 0x20-byte entries at `+0x504`, and a missing integer returns `0x7fffffff`.
- Field creation returns a one-bit field mask through the caller's value buffer. For type 4, the same buffer initially contains the 0x20-byte string: the function copies those bytes into object storage first, then overwrites the caller buffer's first DWORD with the bit.
- The exported field snapshot is exactly 0x68 bytes: object name `+0x00`, field name `+0x20`, normalized active DWORD `+0x40`, integer `+0x44`, and string `+0x48`. It is fully zeroed even when lookup fails.

## 2026-08-16 - Script and visual teardown use the runtime heap directly

- Primary script objects and visual objects both link through `+0x24` and are freed in forward list order with the script root heap at `+0x81c`, flags zero.
- Primary-object destruction tolerates a null runtime root; visual-object destruction and removal assume it is valid. Visual removal matches identity at `+0x20`, unlinks before freeing, and returns the exact `HeapFree` result.

## 2026-08-16 - Runtime root lists use separate traversal offsets

- The runtime-tree root list begins at script root `+0xf78` and follows root siblings through `RuntimeTreeNode +0xb4`; the tail helper does not descend into children.
- The fixed-name list at script root `+0xf88` uses 0x58-byte nodes with a fixed 0x20-byte name and next pointer at `+0x54`. Its lookup tolerates a missing runtime root, and its destructor frees in forward order then clears the head.
- Tree ancestry follows parent pointers at `+0x24` and stops before both NULL and the `-1` sentinel. The node whose parent is the sentinel is itself the returned root.
- Global secondary-resource and scene-link heads live at script-root `+0xfa0` and `+0xfa4`; they use their native link offsets `+0x48` and `+0x40` respectively. Scene global lookup assumes a valid runtime root, while secondary lookup checks it.

## 2026-08-16 - Runtime-tree link insertion propagates through ancestor lists

- Scene and secondary-resource predecessor lookup returns pointer sentinel `-1` when the node parent is NULL or `-1`. With a real parent, it scans preceding siblings backward through `+0xb8`, asks each sibling subtree for its last link, then falls back to the parent tail at `+0x78` or `+0xa8`.
- When no local predecessor exists, insertion prepends the same link to the parent head and repeats at the parent, causing that link to be represented in every applicable ancestor ordering. A real predecessor terminates propagation with an ordinary splice.
- At the global boundary, a NULL parent appends through the global tail without changing `link->next`; a `-1` parent inserts after the global tail while preserving its successor, or prepends to the global head when no tail exists. Global secondary and scene tails are at script-root `+0xfbc` and `+0xfc0`.

## 2026-08-16 - Runtime-tree link removal distinguishes two root sentinels

- Scene and secondary removal consume an inclusive head/tail range stored on the removed node. A null range tail is an immediate no-op. Interior parent ranges are found by walking from the parent head until `next == removed_head`, then spliced to `removed_tail->next`.
- When the removed head equals a real parent's head, the function either clears that head if the parent's recursively computed last link equals the removed tail, or advances it to the removed tail's successor, then repeats at the next ancestor.
- A NULL parent rewrites through the global tail when present, otherwise the global head, and deliberately does not repair the global-tail pointer. The `-1` parent path removes from the global head/interior and repairs the global tail only when it equals the removed tail.

## 2026-08-16 - Primary-resource ordering shares the runtime plan-list prefix

- Primary-resource links use `next` at `+0x24`; tree-node head/tail fields are `+0x9c/+0xa0`. Their predecessor, insertion, and removal algorithms mirror the scene/secondary ordering family with those native offsets.
- The global primary-resource head/tail are script-root `+0xf8c/+0xfa8`. Other recovered functions interpret the same storage as `RuntimePlanNode` and consume `next +0x24` plus flags `+0x28`; keep this confirmed prefix-layout overlap explicit rather than inventing separate root fields.

## 2026-08-16 - Primary-link updates retain prior mutable state

- Primary links are at least 0x8c bytes. Identity is `+0x20`, next `+0x24`, flags `+0x28`, fixed name `+0x2c`, resource identity `+0x4c`, value `+0x54`, coordinates `+0x5c/+0x60`, and prior resource/x/y snapshots `+0x80/+0x84/+0x88`.
- Replacing a changed 32-byte name snapshots `+0x4c` into `+0x80` and clears `+0x4c`. Nonzero coordinate deltas snapshot then add; zero deltas do nothing. The `+0x54` value is replaced only by a nonzero argument.
- The suffix helper at `0x0040A920` always emits three arithmetic digit bytes after a prefix. It does not constrain the input to 0..999, so the hundreds quotient can produce a non-decimal byte for larger values.

## 2026-08-16 - Node +0x84/+0x88 is a separate opaque ordered-link family

- Opaque 0x68-byte records with `next +0x24` are ordered through node head/tail `+0x84/+0x88` and script-root global head/tail `+0xf94/+0xfb0`. Their semantic role is not yet proven; retain offset-neutral naming until constructors/callers establish it.
- Tail search, predecessor selection, insertion propagation, and inclusive-range removal use the same sentinel architecture as the primary/scene/secondary families, with their own confirmed offsets.
- The global primary-resource lookup at `0x0040A990` is separate: it scans script-root `+0xf8c`, compares the first 0x20 record bytes, and follows primary `next +0x24`.

## 2026-08-16 - Link84 mutation and Link8C ordering

- Link84 identity is `+0x20`; rectangle is `+0x2c..+0x38`; conditional scalars are `+0x40/+0x4c/+0x50/+0x54`; current identities are `+0x58/+0x5c`; prior snapshots are `+0x60/+0x64`.
- `+0x4c` uses sentinel `0x7fffffff`; the other scalar arguments ignore zero. Nonzero changed identities snapshot their former values. A rectangle of four zero DWORDs is ignored; otherwise the function first forwards x/y deltas to the current `+0x5c` primary link, then replaces all four rectangle fields.
- A distinct opaque 0x54-byte record family links at `+0x24` through node head/tail `+0x8c/+0x90` and script-root globals `+0xf9c/+0xfb8`. Its semantic role remains unproven, so retain offset-neutral naming.

## 2026-08-16 - Node +0x7c/+0x80 is another opaque ordered-link family

- This family uses 0xb4-byte records with `next +0x24`, node head/tail `+0x7c/+0x80`, and script-root global head/tail `+0xf90/+0xfac`.
- Its recursive tail, predecessor, insertion-propagation, and inclusive-range removal functions use the same two-sentinel ordering architecture as the other runtime-tree link families. Its semantic role is not yet proven; retain offset-neutral Link7C naming until parser constructors and consumers establish it.

## 2026-08-16 - Script condition containers combine ordering and field-state masks

- Script condition containers are fixed 0x1b4-byte records: name `+0x00`, identity `+0x20`, next `+0x24`, rebuilt current mask `+0x28`, required mask `+0x2c`, slot count `+0x30`, and 32 slots from `+0x34`. Each 0x0c-byte slot contains an owned script-object pointer, a pointer to an active-field mask, and the tested field bit.
- Container ordering uses tree-node head/tail `+0x94/+0x98` and script-root global head/tail `+0xf98/+0xfb4`, following the same NULL/`-1` sentinel insertion and range-removal architecture as other ordered tree families.
- State queries rebuild the current mask from every slot before comparison. A missing runtime returns false, while a valid runtime with no matching container returns true. Container destruction attempts every non-NULL owned object and the container itself, bitwise-AND accumulating all `HeapFree` results without short-circuiting.

## 2026-08-16 - Runtime event acknowledgement is two-stage

- The 32-entry ring stores 0x40-byte records. Reading clears output word `+0x38` before checking runtime/availability, copies the entire record when present, and advances only when requested.
- Acknowledging an unmarked current record sets flag `0x20000`; acknowledging it again consumes it without copying. Both operations are no-ops for an empty ring, and indices wrap after 31.

## 2026-08-16 - Script text serialization uses a fixed VirtualAlloc arena

- The serializer allocates exactly 64,000 bytes with `MEM_COMMIT | MEM_RESERVE` and `PAGE_READWRITE`. Its 12-byte header is length `+0x00`, usable capacity `0xf9f4` at `+0x04`, and an inline data pointer to `+0x0c`; script-root `+0xfc4` owns the pointer.
- Append helpers advance only the length and write directly without capacity checks. Copied string terminators are deliberately overwritten by delimiters or CR/LF; final document text leaves the copied terminator at the current length. Statement termination reads `data[length - 1]` without guarding zero length.
- The integer writer treats the input bits as signed only for sign detection, negates in unsigned arithmetic, emits the magnitude through the original divisor loop, and always appends the caller delimiter.

## 2026-08-16 - Bounded script random selection clamps asymmetrically

- The helper seeds the CRT generator exactly once from `GetTickCount`, clamps only a minimum below `-10000` and a maximum above `10000`, and does not symmetrically clamp the opposite endpoints.
- When the resulting maximum is not greater than the minimum it returns the minimum without calling `rand`; otherwise it returns `minimum + rand() % (maximum - minimum)`.

## 2026-08-16 - Script serializer directives use sparse exact-code mappings

- Property code emission recognizes 20 sparse byte values and writes the mapped lowercase name followed by `=`. It writes both the value and trailing space only when the value pointer is non-null; a null value leaves the output at `name=`.
- General scope emission recognizes eight sparse DWORD values and writes `/TOKEN:`. Preload is deliberately handled by a separate function that recognizes only `0x50000000` and writes `/PRELOAD:`. All three emitters leave the buffer untouched for unknown codes or a null buffer.

## 2026-08-16 - Bounded script-text lookup returns parser continuation offsets

- The property scanner returns one past the delimiter that stopped value capture, not the value end itself. When the output pointer is null, it skips capture and returns one past the first value byte. Both the property-name scratch buffer and caller output are fixed at 32 bytes with at most 31 copied characters.
- The section scanner decrements its remaining count before testing the compared byte; consequently an otherwise valid closing `]` in the final supplied byte is rejected. Preserve this boundary order rather than normalizing it into conventional substring-search behavior.

## 2026-08-16 - Parser token extractors own cursor advancement

- The parser state is proven through 0x28 with text at `+0x18`, bounded length at `+0x1c`, and cursor at `+0x24`; keep the earlier 24 bytes and `+0x20` semantically unresolved.
- Property-name extraction uses separators to move a candidate start and commits the cursor only after finding `=`. Scope extraction searches for `/`, aborts on `;` or `[`, and leaves the cursor on the terminating delimiter. Both use fixed 32-byte outputs and a 31-byte copy ceiling, but their cursor behavior at that ceiling differs.
- Property and scope classifiers distinguish extraction failure (`0xffffffff`) from a successfully extracted unknown token (`0`). Their sparse codes must be mapped from exact, case-sensitive executable strings.
- Executable opcode classification uses the same slash-token extraction primitive as scope classification, but has an independent 52-entry sparse mapping spanning low-byte, nibble, word, and high-bit command families; do not infer codes from textual similarity.

## 2026-08-16 - Generic token and integer parsing share skip grammar

- Both generic token extraction and integer-literal parsing skip tab/LF/CR/space/comma/colon plus complete parenthesized groups, while `/`, `;`, and `[` are hard failures before a value. A `[` inside a skipped parenthesized group also fails.
- Generic tokens stop on `(` as well as the normal separators and use the caller's capacity minus one. Integer parsing permits exactly one leading sign and commits the parser cursor only after consuming at least one digit; its failure sentinel is `0x7fffffff`.

## 2026-08-16 - Standalone generic text uses a fixed measured state

- Standalone text state is exactly 0x3c bytes: text `+0x0c`, font identity `+0x10`, two caller values `+0x14/+0x18`, colors `+0x1c/+0x20`, and four measured bounds DWORDs at `+0x2c`. Initialization requires media backend type `0xac` and measures through `strlen+1`, including the terminator in the supplied end bound.
- Wrong backend type and empty text return zero without clearing the caller state. Drawing recomputes `strlen+1` and forwards the entire state prefix to the generic text renderer with flags zero.
- The scoped-token serializer at `0x0040CE90` is not a general tokenizer: it assumes colon is followed by decimal digits and a delimiter, uses unchecked 32-byte scratch storage, and emits only the scope for a terminal token lacking space or colon.

## 2026-08-16 - Path numeric identification concatenates digits after the first backslash

- `0x00418230` returns an integer despite the original untyped decompiler view. With a backslash it ignores all characters before the first backslash, then concatenates every later digit; without a backslash it scans the entire nonempty string. Nondigits do not terminate parsing, and no digits yields `-1`.

## 2026-08-16 - Media signatures and the embedded gzip boundary

- `0x004299B0` checks the word at `+4` first, then `BM`, seven bytes of `WAVEfmt` at `+8`, and five bytes of `[CFG]`. Its zero-padded `strncpy` scratch locals mean the source bytes immediately after the seven- and five-byte signatures are deliberately irrelevant.
- The regions `0x00403650..0x00404910` and `0x004124F0..0x00412D8A` contain the recognizable legacy GNU gzip `trees.c`, `bits.c`, and `deflate.c` implementation. Delegate canonical functions such as `ct_init`, `build_tree`, `flush_block`, `bi_reverse`, `lm_init`, and `deflate`; retain `0x00404920` separately because it is GAG's memory-buffer `file_read` adapter.
- A secondary Ghidra function at `0x00412B9A` was flow inside `deflate_fast`, not a real entry. Function statistics must follow corrected boundaries rather than preserving an earlier analyzer count.

## 2026-08-16 - CDF writer payloads use fixed two-megabyte chunks

- `CdfArchive +0x134` is the current writer-entry index. Uncompressed output seeks to that entry's `+0x24` offset and writes its `+0x28` size in chunks capped at `0x200000` bytes.
- The writer ignores `WriteFile`'s Boolean result and checks only the returned byte count. A short count sets archive error 2. A zero-sized payload still calls `GetProcessHeap`, clears the error, and seeks before succeeding without a write.

## 2026-08-16 - CDF finalization is failure-insensitive ownership teardown

- `0x004298E0` first publishes `write_entry_index` as `entry_count`, calls index serialization, then patches three DWORDs at file offset 7 in order: index size, writer entry count, and index-data size.
- It ignores serialization, seek, write, close, and free results. Entry storage is freed before the 0x207c-byte archive object, and the exact return is zero for both null and non-null inputs.

## 2026-08-16 - CDF append publishes metadata even when payload writing fails

- `0x004297E0` uses `entry_count` as writer capacity and `write_entry_index` as the active slot. It performs an unchecked name copy, classifies the payload, records uncompressed size and current file offset, then dispatches raw/compressed output.
- Compression adds rather than ORs `0x10` into the byte flag. The index and accumulated uncompressed size advance after either payload writer returns, even when that result is zero.

## 2026-08-16 - CDF97a writer capacity is allocated wide but published narrow

- `0x00429630` allocates entry storage using the full 32-bit requested capacity times 0x2c, but stores only `capacity & 0xffff` as the usable count and builds only that many table pointers.
- The constructor writes a `CDF97a\0` signature followed by three zero/initial DWORD placeholders in separate 7/4/4/4 calls. It ignores all header-write results and clears the global error only after those calls.
# 2026-08-16 - Fixed-name records split ordinary and palette flags

- `0x00407240` uses exact 32-byte names and 0x58-byte zeroed records. Identity is self at `+0x20`, ordinary image flags accumulate at `+0x24`, the file string begins at `+0x28`, and palette selection lives at `+0x48` with initial bit `0x04000000`.
- `/F PRIMARY` clears palette bit `0x04000000` at `+0x48` and also ORs bit 1 into ordinary flags. `/F NOPAL` updates only `+0x48`; all other parsed image flags accumulate at `+0x24`.
- `0x0040CDA0` suffixes an unqualified file token with runtime string `+0x828`, serializes an optional lookahead without consuming it, and treats integer sentinel `0x7fffffff` as acceptable while clearing the filename and returning `0xffffffff` for an explicit mismatch against runtime DWORD `+0x824`.
# 2026-08-16 - Runtime named-node children are circular and count-governed

- Runtime named nodes hold a circular doubly linked child list at head `+0x44`, tail `+0x48`, and cursor `+0x4c`; 0x34-byte children link next/previous at `+0x2c/+0x30`. The authoritative number of children is node DWORD `+0x40`.
- Cursor rotation occurs only when child count `+0x40` exceeds threshold `+0x28`. `0x00407C00` follows child previous `+0x30`, while `0x00407C60` follows next `+0x2c`, exactly the requested nonzero count.
- Named-node serialization walks children until it returns to the head, independent of the count, then emits `/ZONE` DWORDs in order `+0x30,+0x34,+0x28,+0x38,+0x3c`. Disabled-node pruning instead trusts the count and frees exactly that many children before freeing the node.
- Script-object membership entries store the object's identity at child `+0x20` and copy its exact 32-byte name. Node lookup uses fixed 32-byte equality, whereas object lookup uses the zero-terminated string comparator.
# 2026-08-16 - Runtime commands occupy a fixed 32-entry root table

- Root `+0xa70` is the command-definition count; `+0xa74` begins 32 entries of 0x28 bytes containing name `+0x00[32]`, visual-object pointer `+0x20`, and flags `+0x24`. `0x004095E0` clears the count and entire table as exactly 0x141 DWORDs.
- The parser rejects only when the entry count is already above 31. An exact 32-byte existing-name match reuses that slot but still increments the global count, so serialization can subsequently include a zeroed trailing entry; preserve this non-deduplicating count behavior.
- Command `/MOUSE` stores the matching runtime visual object itself, and serialization reads the object's inline name through that pointer. Command bit `0x00200000` serializes as `DUAL` from string address `0x0043E378`; `INVERT_NOPAL` begins separately at `0x0043E380` and must not be conflated with it.
- The object serializer's separate bit `0x00010000` helper emits `NATURALMOUSE` from `0x0043E360`. Both sparse helpers ignore every unrecognized flag bit rather than serializing a generic representation.

# 2026-08-16 - Runtime visuals retain source text separately from resolved files

- Runtime visual objects are 0x164 bytes: name `+0x00[32]`, identity `+0x20`, next `+0x24`, resolved file `+0x28[32]`, serialized FILE expression `+0x48[0x104]`, position `+0x14c/+0x150`, prior/current scene identities `+0x154/+0x158`, flags `+0x15c`, and palette flags `+0x160`.
- Parser-driven updates initialize palette flags from the runtime default, set dirty bit `0x00100000`, and preserve a current scene only for an unchanged valid FILE. A changed or absent FILE snapshots a non-null current scene before clearing it; an unchanged FILE clears the dirty bit.
- PRIMARY bit 1 is exclusive across the visual list and publishes the selected object at parser `+0x70`. NOPAL bit `0x04000000` lives in the separate palette word; `/INVERT_NOPAL` toggles it after parsing.
- Visual serialization emits POS only when serialized FILE text is nonempty and at least one coordinate is nonzero. Palette overrides are differences against the root default: NOPAL uses `/F:NOPAL`, while disabling a default NOPAL uses the deliberately colonless `/INVERT_NOPAL ` form.

# 2026-08-16 - Scene RECT values are inclusive while POS values are dimensions

- Scene links are 0x44-byte records: name `+0x00`, identity `+0x20`, Z `+0x24`, x/y `+0x28/+0x2c`, width/height `+0x30/+0x34`, flags `+0x38`, scene identifier `+0x3c`, and next `+0x40`.
- `/RECT left top right bottom` stores width and height as `right-left+1` and `bottom-top+1`; `/POS x y width height` stores all four values directly. The scene parser accepts only image flags 2, 0x20, 0x02000000, and 0x04000000.
- Secondary-resource links are 0x4c-byte records with fixed name `+0x00`, self identity `+0x20`, resolved FILE `+0x24[32]`, resource identity `+0x44`, and next `+0x48`. Owners with flag `0x400` register the resolved FILE, not the link name, as an auxiliary name.

# 2026-08-16 - Primary LIST parsing expands a disposable template into paired link families

- Primary-resource links are 0x8c bytes: identifier `+0x00`, identity/next/flags `+0x20/+0x24/+0x28`, file `+0x2c`, current resource `+0x4c`, image flags `+0x54`, loop `+0x58`, x/y `+0x5c/+0x60`, source `+0x64`, width/height `+0x68/+0x6c`, ratios `+0x70/+0x74`, secondary/fixed-name pointers `+0x78/+0x7c`, and resource/x/y snapshots `+0x80/+0x84/+0x88`.
- Direct parsing initializes ratios to one, inherits root palette flags, sets primary flag `0x80000000` only for root-flag `0x20` plus sentinel-parent nodes, and uses `/F DOUBLE` to force both ratios to two. `/F NOCLS` routes to primary flags; ordinary image flags route to `+0x54`.
- `/LIST` makes the allocated primary record a disposable template. Each output identifier is the template prefix plus an unchecked three-digit suffix. Circular list children supply an object pointer, file bytes at object `+0x430`, and DWORD `+0x47c`; after wrapping to the cursor, remaining count-governed slots behave as null children.
- Every expanded slot creates or updates a primary link and a link-84 record. Null-child primaries gain flag `0x80000000`. Link-84 receives the named-list pointer at `+0x54`, list `+0x3c` at its `+0x4c`, and coordinates `(x,y,x+list+0x30,y+list+0x34)`; x then advances by list `+0x30 + +0x38`.
- `0x0040A3C0` uses `0x7fffffff`, not zero, as the no-change sentinel for source and coordinate deltas; even a zero delta snapshots the prior coordinate. `0x0040AE40` initializes `+0x54` only on creation, sets `+0x3c` to `0xffffffff` on every call, and propagates changed existing x/y into the linked primary record.
# 2026-08-16 - Named-list parsing mixes fixed-width membership and sentinel defaults

- `0x00407490` uses exact 32-byte equality for both named-list nodes and script objects. Tests and callers must provide deterministic bytes through the entire fixed field; a short C string alone does not establish the compared padding.
- Named-list object children are the same 0x34-byte circular entries used by the generic runtime cache: object identity at `+0x20`, exact object name at `+0x00[32]`, and next/previous at `+0x2c/+0x30`. Allocation after an object match is intentionally unchecked.
- `/ZONE` parses five integers into node offsets `+0x30,+0x34,+0x28,+0x38,+0x3c`. Missing values preserve the first, second, fourth, and fifth fields, but the third is written first and then normalized from `0x7fffffff` to one.
- The parser returns zero on every path, including successful creation and mutation; callers must observe the node state rather than treating the return as success.
- `0x0040AAC0` reveals that the link-84 record is the runtime command/pointer association: `+0x40` is an accumulated command-definition mask, `+0x44` is the single PCOMM bit, `+0x4c` is `/P`, `+0x50` is the MOUSE visual identity, `+0x58` is the OWNER object, and `+0x5c` is the IMAGE primary-resource link. Its RECT syntax stores `x + third` and `y + fourth` without the inclusive `+1` used by scene-link RECT parsing.
- `0x0040B3E0` confirms that link-family scope meanings must be derived per parser: link-8C uses the general scope codes TIME (`0x00600000`), RAD (`0x00400000`), LINE (`0x00500000`), and RECT (`0x02000000`). Its RECT stores four direct values, unlike both scene-link endpoint conversion and link-84 x/y-relative storage.
- `0x0040B850` embeds a byte-for-byte `ScriptParserState` at link-7C `+0x38`, then replaces embedded `start_offset` and `cursor` with the cursor immediately after the record name. Later link-7C opcode scanners temporarily reset that embedded cursor to this saved start, so preserving both fields and the original text pointers is required rather than storing only parsed interaction fields.
- The link-7C navigation routines have intentionally asymmetric cursor ownership: `0x0040C1E0` and `0x0040C260` reset to the embedded start but may retain a successful position, whereas `0x0040C2F0` always scans from and mutates the current cursor. Its nesting grammar increments only for SWVALUE (`0x4000`), SWRAND (`0x40000`), and SWLOCK (`0x50000`); VALUE (`0x5000`) is deliberately not a nesting opener.
- `0x0040BF60` treats link-7C `+0x74` through `+0xb3` as a 16-DWORD criteria block matching the event-ring record layout. NOMATCHES recursion uses a copied event state, skips the current criteria block by exact address, prefilters alternate links with `(alternate.flags ^ event.flags) & 0xf0000fff`, and rejects when any alternate matcher succeeds; its recursive arguments are ECX=state copy and EDX=alternate `+0x74`.

# 2026-08-16 - Condition containers distinguish ownership from observation

- `0x0040C570` inserts its zeroed 0x1b4-byte container before parsing any member triple, so malformed trailing input deliberately leaves the container published.
- A slot stores its object pointer only when the parser created that object; an already-existing object leaves the slot pointer null while still storing the object's active-mask address and field bit. Container teardown therefore frees only parser-owned objects.
- Global-system slots observe root flags directly and use masks `0x10`, `1`, `2`, and `4` for DRIVE_BUSY, NOCOMMENT, INVENTORY_OPEN, and INVENTORY_CLOSE. Their required bit is set for every typed flag except OFF (`0x07000000`), unlike ordinary slots, which additionally require a signed-positive truth marker.
# 2026-08-16 - Archive speed measurement skips one fixed block

- `0x00417990` creates an async host with a 0x10000-byte buffer, performs one unmeasured 0x8000-byte read, then times the remaining selected bytes in chunks of at most 0x8000.
- A zero caller limit selects unsigned `file_size - 0x8000`; when the file is smaller than a nonzero limit the same subtraction replaces the limit. The timed loop tests the remaining DWORD as signed, and read return values/byte counts do not affect progression.
- Throughput is integer bytes per millisecond and remains zero unless the second `timeGetTime` value is strictly greater than the first. Record cleanup precedes host destruction on every opened-record path.

# 2026-08-16 - Preserve native uninitialized byte reads without checked scalar UB

- `0x0040C570` really loads the first DWORD of an uninitialized stack character buffer and carries it across typed entries. Expressing that as a direct uninitialized `uint32_t` read triggers MSVC Debug Run-Time Check #3 and leaves CTest waiting behind an interactive dialog.
- Copying the four-byte object representation from the character buffer with `memcpy` preserves the original native stack-byte dependency while avoiding a direct checked scalar read. This is a narrow fidelity shim, not permission to initialize or normalize the original value.
# 2026-08-16 - Runtime command targets use asymmetric flag probes

- `0x00421440` first probes an image flag at the entry cursor and proceeds only when that probe returns zero. A positive flag or `0xffffffff` returns zero immediately; only the latter is normalized to output flags zero.
- After the zero probe it restores the entry cursor, reads the first name, and probes again. A missing or positive second flag converts the first name into the tree name and copies the current resource's fixed 0x20-byte name into the resource output. A zero second probe instead restores the post-first-name cursor and reads an explicit tree name.
- The explicit-name form accepts a third flag only when interpreted as signed-positive. All completed target forms return one even when that third probe is zero or negative.

# 2026-08-16 - Function counts exclude flag-dependent analyzer fragments

- `0x004204BB` was not a callable function: it began with `JA` using flags established by `GetRuntimeScriptProperty` at `0x004204B0`, used that predecessor's 0x10c-byte stack frame, performed its epilogue, and had no callers. Delete such boundaries and reduce the recognized-function denominator rather than counting them as unresolved work.
# 2026-08-16 - Archive comment dialogs retain paths separately from displayed comments

- `0x004182A0` uses a 0x98-byte dialog context. It constructs the search as `directory + "*" + extension`, while each opened archive path is `directory + found filename` and occupies a fixed 0x104-byte slot in the retained array.
- The listbox receives the contents of `COMMENT.TXT`, but the retained slot contains the archive path. A path is copied before comment validation and is reused at the same count index when the comment is absent, oversized, or unreadable.
- Capacity starts at ten and grows by ten exactly when count reaches capacity. A failed `HeapReAlloc` closes the current archive and ends enumeration while retaining the old pointer, count, and capacity; it is still a successful nonempty result.
- Fatal CDF open error `0x10000` frees/reset storage, closes enumeration, deletes the failing archive path, and returns `0x10000`. Other open failures are skipped. No usable comments frees storage and returns two.
- Maximum selection first compares the stored value and `ParsePathNumericIdentifier(filename)+1` as signed values; only when that does not increase it does an unsigned count comparison apply.

# 2026-08-16 - Archive comment dialog callbacks use the custom control as state storage

- `0x00418560` publishes and retrieves its 0x98-byte state through control 1001 messages rather than window user data: initialization uses custom message `0x7ff0` with selector 1, and later commands query it with selector 2.
- List control 1000 selection changes forward the selected retained 0x104-byte archive-path slot to the custom control with selector 4. A double-click additionally sends selector 8, frees the retained array, and ends with result zero.
- Enumeration result two inserts the literal no-entry placeholder, still publishes the state, configures the custom control with selector `0x10`/value `0x70`, and leaves the dialog active. Result `0x10000` ends immediately with the same result.
- `0x00417550` launches resource 101 only when the custom control class is registered. It uses `_splitpath` only for the fixed filename and extension fields, supplies the same output buffer in both state output slots, and clears that output after every nonzero dialog result.
- Missing callback boundaries can increase the recognized-function denominator when repaired. `0x004188A0` is an independent 1471-byte dialog procedure, not continuation data from `0x00418560`; count it as unresolved until its complete branch set is recovered.

# 2026-08-16 - Archive save selection couples edit text to retained archive paths

- `0x004188A0` uses edit control 1009 and reacts to notifications `0x100` (EN_SETFOCUS) and `0x300` (EN_CHANGE). Do not substitute EN_UPDATE (`0x400`) based on an imprecise semantic label; preserve the numeric branch verified in assembly.
- The edit buffer is 0x104 bytes, is zeroed, and begins with WORD `0x0103` before EM_GETLINE, setting the original 259-character capacity. Exact list matches forward the corresponding retained 0x104-byte archive path through custom-control selector 4; misses restore selector 0x20 with the initial value or selector 0x10/value 0x70.
- IDOK with empty edit text cleans up and ends with result two. A new name copies edit text to output 2 and constructs output 1 as `directory + filename-prefix + three-digit maximum_identifier + extension`. An existing name prompts with `Replace \"name\" ?`, caption `SAVE`, and flags `0x34`; IDNO leaves the dialog and retained allocation active.
- Accepted nonempty IDOK sleeps 100 ms only when the state's initial value is non-null, then frees retained paths and ends zero. List double-click always prompts and, when accepted, frees and ends zero without the sleep.
- `0x004175F0` has a six-argument fastcall ABI (ECX/EDX plus four stack arguments, `RET 0x10`). Its raw assembly sets EAX to zero before the registration gate and otherwise returns DialogBoxParamA's EAX, even though the initially inferred decompiler signature was void.

# 2026-08-16 - Runtime sound pause and resume intentionally differ from full shutdown/readiness

- `0x004015D0` always writes the 32-bit mixing-suppression state after acquiring the lifecycle mutex. Its argument only controls whether output initialization is cleared and an already-ready waveOut/thread pair is torn down; zero does not mean a no-op.
- The pause teardown resets waveOut, unprepares exactly two headers, resets again, closes, then optionally posts WOM_CLOSE and joins/closes the thread. It releases the lifecycle mutex but does not destroy sound handles, format storage, or mutexes as full shutdown does.
- `0x004016D0` clears suppression and recreates output only when both initialized and callback-ready states are zero. Thread creation failure is nonfatal and reaches release/return one; waveOut-open failure shuts down the created thread and also returns one.
- While waiting for the sound window, a nonzero fault state returns zero directly before the common mutex release. Preserve this lock leak because it is explicit control flow, not a cleanup omission to modernize.
- `0x004010A0` uses a full DWORD bitwise NOT, not boolean negation. Its two callers use the same toggle during subsystem disable and enable.
# 2026-08-16 - Basic runtime-tree commands distinguish local and explicit resources by the second value token

- `0x00406A70` and `0x00406C00` parse the first value as both the local section/tree name and a possible external resource name. A missing second value copies all 32 bytes into the destination name and reuses the parser resource; a present second value loads the first name as a resource.
- `0x00406A70` extracts parenthesized creation text once before probing the second token and a second time only for the explicit-resource form. Preserve both cursor side effects.
- `0x00406C00` consumes scope codes to exhaustion. Scope `0x00200000` selects the `0xffffffff` parent sentinel, except an owner already carrying that sentinel rejects the command immediately.
- `0x00405080` saves a resource node's next pointer before calling the single-node remover. This permits traversal to continue even when the remover retains a node because its active-reference count is nonzero.

# 2026-08-16 - Script object parsing compares complete fixed-width names

- `0x00407FA0` compares object, field, and command names as complete 0x20-byte blocks. Test fixtures must therefore use zero-padded fixed-width storage; MSVC Debug `strcpy_s` may fill unused destination bytes and is not a faithful fixture constructor for these comparisons.
- The parser first attempts an integer expression, then an image flag, and only then a string token after restoring the cursor. ON/OFF are stored as active-mask changes, while ordinary signed-positive integers both store a value and activate the field.
- Natural-mouse flag `0x10000` changes finalization: both mouse names resolve to visual-object pointers. Without it, `/INVERT_NOPAL` conditionally toggles image bit `0x04000000` instead.

# 2026-08-16 - Pending tree switching routes identity through ECX

- `0x004210A0` passes null/null as the two stack arguments to tree activation. On success it passes the activated node in ECX to both parser-context reset and `0x004268B0`; on activation failure it passes the original node in ECX to `0x004268B0`.
- `0x004268B0` uses its incoming identity for resolution and root publication. Each resource constructor call supplies the complete formal argument list from the corresponding record; inherited EDI is not forwarded as a constructor argument.
- Fixed-name nodes are 0x58-byte runtime records, not only serialized strings: file text occupies `+0x28..+0x47`, followed by resource flags, current resource identity, previous resource identity, and next pointer at `+0x48/+0x4c/+0x50/+0x54`.

# 2026-08-16 - Backend-child attachment retains a destroyed return identity

- `0x00425D50` acquires resource records in main, secondary, fixed order but releases them in secondary, fixed, main order. Missing linked identities are inherited from main resource offsets `+0x68` and `+0x64` before the subordinate acquisitions.
- Once child creation succeeds, a missing display lock or failed 16x16 scene creation destroys the child without clearing the return register. The caller receives the original child pointer even though its backing child was destroyed.
- Successful attachment publishes secondary/fixed identities back to main `+0x68/+0x64`, sets main type flag `0x01000000`, and stores the child at `+0x74`. When the main resource cannot be acquired, the secondary identity becomes the display-scene owner and no back-publication occurs.
# 2026-08-16 - Runtime-tree publication uses parent state as a three-way routing mode

- `0x00406190` does not simply append every node. A normal non-null parent produces no publication; parent `0xffffffff` updates each global tail only when the corresponding local tail is non-null; parent zero publishes each local head through the existing global tail selector or establishes the global head when no selector exists.
- The seven routed families use different next offsets: scene `+0x40`, secondary resource `+0x48`, and primary/link-84/link-8C/link-7C/container `+0x24`.
- `0x00408B80` always emits an ON/OFF token for each declared script-object field, independently of whether that same field emitted an integer or string value. Integer zero and empty strings are omitted, but boolean state is not.
# 2026-08-16 - Pointer events are contiguous records beginning before the named event body

- The pointer event passed to `EnqueueRuntimeEventRecord` begins at `0x0047F8D0`, not at the named `g_abRuntimePointerEventRecord` symbol at `0x0047F8DC`. Overall DWORDs 0/1/2 are the state mask, state owner, and event state object; the named body begins at DWORD 3; flags at `0x0047F908` are overall DWORD 14.
- `0x00423BC0` always enqueues after passing its two top-level gates, even when mode, region, mask, selected scene, or slot eligibility prevents region metadata from being added. Its baseline release flags are `0x10000000`.
- A state object changes the eligible release flags from `0x10000009` to `0x1000000d` and publishes the object in overall DWORD 2; the selected region identity is overall DWORD 3.
# 2026-08-16 - Left-button scene rotation merges visual and state-command eligibility

- `0x004238B0` rotates from the current bit before testing eligibility, wraps `0x80000000` to one, and searches at most 32 positions in `scene_mask | state_object.command_mask`.
- A selected bit present in the state object's command mask bypasses scene-slot flag `0x20`; otherwise flagged slots are skipped. The scene-slot command name is a confirmed 32-byte string at `RuntimeSceneSlot +0x08`.
- Slot name `IView` suppresses both pending script flags and emits a pointer event. `Hide` suppresses flag 2, forces flag 4, and emits the same event family. Any ordinary rotation away from the original bit suppresses both initially derived script flags.
- The apparent `0x004238BE` function was the tail of `0x004238B0`: it depended on inherited ZF, had no callers, and disappeared cleanly when the false function boundary was deleted.
# 2026-08-16 - Decompiler stack names can shift across a large fastcall frame

- In `0x00424EC0`, the decompiler temporarily labeled formal stack values as an extra `in_stack_0000001c`. Raw prologue arithmetic and `RET 0x18` disprove an extra argument: width, height, scale-or-loop, and flags are all formal inputs.
- For animation resources, width and height select X/Y scaling (or receive half-size defaults under flag 2). Nonzero scale-or-loop adds flag `0x400` and sets frame-limit fields. Prefer raw stack offsets and call-site pushes over decompiler variable names when they conflict.

# 2026-08-16 - Private window messages reuse LPARAM with command-specific layouts

- `0x0041D560` message `0x7FFD` uses WPARAM as a private command selector. Boolean state-query commands interpret LPARAM as two consecutive 0x20-byte names, whereas lifecycle commands such as credits completion and current-state activation interpret the same LPARAM as the separate activity/flags record.
- A value left in EDX at a fastcall call site is not evidence of a second argument. `BeginScriptTextDocument` (`0x0040D0F0`) and `EndScriptTextDocument` (`0x0040D140`) overwrite EDX with embedded `[CFG]`/`[END]` pointers before their first string operation; raw callee assembly proves both are one-argument ECX-only functions despite the earlier decompiler rendering.
- Model these as distinct structures and cast only after selecting the command. A single decompiler-inferred LPARAM type can make valid `+0x20` field-name accesses appear to overlap unrelated lifecycle fields.

# 2026-08-16 - Preserve reversed-looking timeout branches

- `0x004263A0` checks each named-node status and exits immediately when the current tick is below `start + 5000`; it sleeps and retries only when the current tick is already at or beyond that threshold. Raw `CMP`/`JC` flow and focused boundary tests confirm this counterintuitive direction.
- Do not normalize a suspicious timeout into conventional “wait until deadline” behavior. Preserve the unsigned branch exactly and test values on both sides of the threshold.

# 2026-08-16 - Runtime text input applies case rules only to signed-positive bytes above `@`

- `0x00420E10` uses signed `JL` after comparing the dequeued byte with `0x40`. Consequently bytes `0x80..0xff` bypass ASCII case conversion even when text modes `0x10` or `0x20` are active.
- Its caret checks are strict unsigned comparisons: equality at `tick + 250` selects the pre-threshold branch, and equality at `tick + 500` does not reset the base tick. Preserve both boundaries in tests.

# 2026-08-16 - Re-type contiguous globals before accepting subsystem boundaries

- The addresses `0x0047F59C..0x0047F60C` initially looked like an independent input-session record/object subsystem, but they are `RuntimeCommandLoopState +0x63c..+0x6ac`. Once those state fields were defined, `0x00420790` and `0x004208E0` re-decompiled into text-scene initialization and text-buffer copying.
- When several adjacent globals fall inside a subsequently confirmed large state object, re-decompile every already-recovered function that references them. Passing tests around an injected abstraction do not prove that the abstraction corresponds to the binary.
# 2026-08-16 - Audit consumed EAX even when a recovered helper looks procedural

- `0x0040C4B0` was initially modeled as `void`, but its caller at `0x00421530` tests EAX immediately. Raw assembly proves deliberate status propagation: zero for null root/link and a failed criteria match, one for an already-active link and a successful new activation.
- A callee whose primary side effect is mutation may still have a meaningful implicit return. When any caller consumes EAX, audit every exit path and preserve that return in both source and Ghidra before reconstructing the caller.
# 2026-08-16 - Compare the Ghidra address set against source annotations

- An exact comparison between Ghidra function entry addresses and `// GAG.EXE: 0xXXXXXXXX` annotations quickly separates genuine source gaps from bundled compression/CRT routines and analyzer fragments. After the current audit, `0x00421530` is the only recognized non-library game entry missing from `src/`.
- `0x00429EC0` was a false split of `0x00429EB0`: no callers or references, inherited flags/registers, the predecessor's stack frame, and one shared epilogue. Deleting both boundaries and recreating the predecessor caused Ghidra to recover the complete 761-byte body; restore its prototype afterward because function recreation clears it.
# 2026-08-16 - Preserve opcode retry as an explicit disposition

- `0x00421530` has shared labels that distinguish an opcode completing normally from pausing on the same opcode. Backend-child opcode `0x100`, for example, pauses after successful creation and while child flag `0x200` remains set, but completes after parse/create failure or when the wait flag clears.
- A factored dispatcher must return an explicit pause disposition so the outer executor can restore the pre-opcode cursor. Treating every recognized opcode as ordinary completion would skip asynchronous retry behavior.
# 2026-08-16 - Script timed waits complete at deadline equality

- Opcode `0x000F0000` stores `script_clock + duration`, sets link owner flag `0x40000000`, and pauses immediately. Subsequent visits pause only while the unsigned script clock is strictly below the deadline; equality clears the flag and completes.
- Opcode `0x00000200` deliberately parses the same second 0x20-byte name buffer twice before initializing text input. Preserve both parser advances even though the first value is overwritten.
# 2026-08-16 - Resource-wait opcode polarity is intentionally asymmetric

- For `0xA0000000`, a missing primary resource pauses only when a runtime tree with that name exists. For `0xF0000000`, a missing primary resource pauses only when no such tree exists.
- GAME `0x0000C000` is a two-pass opcode: successful DLL initialization pauses without advancing; the later pass under state flag `0x20` consumes the command again, applies the captured type/data to a named field, and clears the full result block.
# 2026-08-16 - Conditional opcode families share one cursor-mutating scanner

- `SWVALUE` (`0x4000`), `SWRAND` (`0x40000`), and `SWLOCK` (`0x50000`) enter the same scanner. SWVALUE first retains two fixed-width names; the other two enter with the existing stack buffers and rely on nested RAND/COND records.
- RAND uses an inclusive signed comparison `minimum <= value <= maximum`. Failed VALUE/RAND/COND branches call the link-7C boundary scanner for `0x60000`; `CSEND` (`0x6000`) or EOF completes the conditional block with the parser left at its resulting cursor.
# 2026-08-16 - GOTO commits a new cursor; waits restore the old cursor

- Link-7C `GOTO` (`0xB0000`) seeks from the embedded parser start and then publishes the sought cursor as the saved cursor. It cannot share the ordinary completion or pause disposition: pause restores the cursor from before the opcode, while GOTO must commit its mutation.
- Tree-load opcodes `0x7000`, `0x8000`, and `0x60000000` share parsing but select parent `0xffffffff`, the current resolved tree, and the link parser owner respectively. A missing second token copies the first token to the tree name and replaces the resource name with the embedded parser resource's fixed name.

# 2026-08-16 - Movement commands keep retry state on the moved record

- MOVZ `0xE0000000` stores its deadline and retry bits on link-84 at `+0x3c/+0x28`; MOVI `0x90000` stores them on the primary resource at `+0x50/+0x28`. Bit one means the timed move has started and bit two blocks further path steps; termination clears both bits.
- Absolute MOVZ preserves the existing rectangle extent while replacing its origin. Timed MOVZ passes the resolved tree identity and link identity, whereas the absolute form passes the resolved tree/link records as observed in the original call sites.
- COPY `0x9000` resolves a non-primary source scene using the destination-name buffer, not the newly parsed source-name buffer. Preserve this apparent bug: it also controls the `BACKGND` source fallback.

# 2026-08-16 - The script executor has five distinct opcode exit dispositions

- `0x00421530` advances immediately after ordinary completion, restores the pre-opcode cursor for asynchronous retry, preserves the sought cursor for GOTO, resets to the parser start and clears link flag `0x80000000` when a link finishes, and jumps to the outer clock-update path after a tree/session restart.
- The outer scheduler services children, messages, text, pair messages, and the nested command loop before testing active flag `0x00100000`. Inside each link it adds external-command processing before pair/command processing and stops scanning links when any of those three reports work.
- Raw assembly initializes the prior-tick slot at `0x0042154F`, but the SWRAND input slot is not written until `0x0042319B`. A Debug reconstruction must not silently trigger MSVC RTC failure when passing the first value; any guard is a labeled portability shim because the original first value is indeterminate.

# 2026-08-16 - Tree-switch opcodes share parsing but not retirement behavior

- Opcodes `0x40000000..1` and `0x50000000..1` share target parsing and activation. A nonzero low opcode byte overrides parsed flags; any nonzero parsed flag also adds `0x10000000`. Only the `0x50000000` family deactivates the old tree when activation resolves a different tree.
- Exit opcodes `0xB0000000` and `0x70000000` reverse the resource/tree output buffers passed to the target parser. Root exits can finish the current link, publish a prior root directly, or force rebuild depending on the prior root's `0x200` flag and the opcode family; nested exits may continue the same script only when the parser owner's tree identity can be found again.

# 2026-08-16 - CDF compressed writers use the gzip wrapper's EAX result

- Raw callers at `0x00429070` and `0x00429B50` immediately retain EAX after calling `0x00418E90`; the gzip wrapper therefore returns the compressed byte count despite its former decompiler-inferred `void` prototype. Audit consumed return registers before accepting library-boundary prototypes.
- The compressed CDF format starts with `ceil(uncompressed_size / 0x8000) + 1` cumulative DWORD offsets. Each source step is exactly 0x8000 even for the final short block. The index writer adds compressed sizes to `CdfArchive +0x118`; the entry writer does not and uniquely passes flag 1 to both `HeapFree` calls.

# 2026-08-16 - Root-head tests must guard adjacent fields

- For tightly packed runtime-root list heads, testing only the resulting list is insufficient: a wrong neighboring offset can still produce plausible behavior while corrupting another subsystem. Seed the immediately adjacent head with a sentinel and verify it survives every head replacement or clear.
- The confirmed head sequence includes generic resources `+0xF74`, runtime tree `+0xF78`, objects `+0xF7C`, visuals `+0xF80`, named nodes `+0xF84`, fixed names `+0xF88`, and plan nodes `+0xF8C`.

# 2026-08-16 - Tree link families share routing shape but not record stride

- Runtime-tree publication and removal route seven list families through parallel head/tail slots, but their next pointers differ: scene links use `+0x40`, secondary-resource links use `+0x48`, while primary, link-84, link-8C, link-7C, and container records use `+0x24`.
- A null parent routes through the currently selected global tail without repairing that tail. Parent `0xffffffff` searches the global list and repairs the tail when the removed inclusive range contained it. Ordinary parents propagate head removal through ancestors or splice an interior range.

# 2026-08-16 - Owned runtime-tree ranges are inclusive, not null-terminated ownership

- `0x00405E50` saves each record's next pointer before releasing it, then stops when the released record equals the node's recorded tail. A non-null `tail->next` belongs to a later range and must not be released.
- Tests for these ranges should use at least head, tail, and successor records. Singleton tests cannot distinguish inclusive tail ownership from accidental traversal to null.

# 2026-08-16 - Parser-context release leaves a deliberate dangling owner head

- `0x004052F0` walks from owner `+0x6C` but never clears that field. This is safe only because the owning tree node is being destroyed immediately afterward; do not modernize it into a head-clearing loop.
- Resource removal occurs before context free whenever the post-decrement active-reference count is zero, including when it was already zero on entry. Preserve and test that ordering.

# 2026-08-16 - Section misses and parser-allocation failures have different resource ownership

- `0x00405380` does not release the already-loaded resource when the requested section is absent. It releases the original resource identity only after a valid section was found but parser-context creation failed.
- `0x00405D00` restores the parser cursor on every unsuccessful jump, including a matched target with missing names or failed tree creation; only successful creation commits the caller-supplied cursor.

# 2026-08-16 - Large parser dispatchers can be accepted in evidence-bounded tranches

- A production-default injected call table can make a large dispatcher observable while preserving its original callee arguments, return behavior, and ordering. Keep the seam limited to direct callees; do not abstract control flow or state mutation owned by the dispatcher itself.
- Record exactly which opcode/property families the harness covers in both the recovery ledger and Ghidra. Untested special families remain unresolved even when they share the same top-level function address.

# 2026-08-16 - Decompiled switch breaks may conceal a shared continue label

- In `0x004056C0`, several decompiler cases appeared to terminate parsing, but raw branch targets converged on the shared cursor-refresh/dispatch label. Verify every apparent `break` against its machine-code destination before translating a large switch.
- Callback operation codes carried in registers must be read from raw assembly at each shared-call predecessor. Structurally similar branches in this dispatcher deliberately use different codes (`4/1/2`, `0x50`, and `0x60`).

# 2026-08-16 - Object-field parsing reuses the prospective insertion index

- `0x00407FA0` searches existing field names and uses `field_count` itself as the prospective new index. It mutates the value/mask at that index first, then appends the 0x20-byte name and increments the count only when the search ended at the old count.
- String fallback saves the cursor before probing an image flag and restores it only when that probe returns zero. A failed subsequent string token skips insertion but continues the object parser; tests must observe both cursor restoration and the unchanged field count.

# 2026-08-16 - Typed-value fallback preserves asymmetric cursor and sentinel behavior

- `0x00408AA0` restores the saved parser cursor before probing the image parser and again before probing the final string parser, but it does not restore the cursor after final string failure.
- The image parser is tested only for zero. Consequently `0xffffffff`, despite being a common failure sentinel elsewhere, is accepted here as a type-1 image value; reproduce the caller's branch rather than normalizing callee result conventions.

# 2026-08-16 - Parser callee results may double as outer-loop control

- In `0x00408DD0`, the scope code normally controls repetition, but the image-flag result and second POS integer overwrite the same register tested at the loop tail. A `0xffffffff` result can therefore mutate palette or Y-position state and then terminate parsing.
- When translating decompiled parser loops, track the physical loop-test register through every branch. A tidy source loop that always retests the original scope token can silently change valid sentinel behavior.

# 2026-08-16 - Successful record-name parsing can commit despite nested parse failure

- `0x00409370` returns immediately only when the command count is full or the initial name token is missing. Once a name succeeds, later `/F` or `/MOUSE` failure exits the scope loop but still increments the global count.
- Existing-name reuse does not suppress that increment. It updates the matched earlier definition while extending the logical count, so tests must preserve the resulting untouched/blank later slot rather than deduplicating records.

# 2026-08-16 - Flattened tree lists are maintained only on first local insertion

- Scene and secondary-resource parsers append subsequent records directly after the node's current tail. Only the first local record invokes hierarchical insertion, because that operation publishes the beginning of the node's inclusive range into the flattened ancestor/global list.
- Allocation and initial-name failures occur before any local/global mutation. Tests should assert both views remain unchanged; checking only the node-local head cannot detect accidental publication through a global tail selector.

# 2026-08-16 - Primary-resource links and runtime plans share global list storage

- The flattened primary-resource list uses script-root `+0xF8C/+0xFA8`, the same storage exposed as runtime plan head/terminal. Primary insertion/removal must therefore preserve the plan-list alias exactly; creating a separate global-primary list would diverge despite locally correct links.
- LIST expansion owns only a temporary 0x8C-byte template. Whether the named list is absent, empty, or fully expanded, the template is freed; generated primary and link-84 records are independently owned by their node ranges.

## Function-accounting baseline

- Derive reconstruction statistics from the current Ghidra internal-function address set intersected with unique `// GAG.EXE: 0xXXXXXXXX` annotations. Keep import thunks separate, and positively classify bundled libraries and CRT/compiler runtime bodies before calculating the game-function denominator. “Represented in source” is address coverage; it must not be presented as completed fidelity re-verification.
# 2026-08-16 - Similar parser properties can target distinct callback operations

- In `DispatchRuntimeTreeParser` (`0x004056C0`), integer-valued properties are not interchangeable: property `0x0B` starts with operation 4 and can switch to 1/2, property `0x0F` uses operation 8, and property `0xA0` uses operation `0x0C`. Keep test expectations independently derived from each raw call site's ECX setup; a shared helper can otherwise conceal an incorrect collapsed operation code.

# 2026-08-16 - Tree constructors may publish before semantic dispatch succeeds

- `CreateRuntimeTreeNode` (`0x00405410`) links its new 0xBC-byte node into the root or child chain before invoking the parser dispatcher. A null dispatcher result suppresses activation and becomes the function result, but does not roll back the newly published node. Treat allocation/context-construction failures separately from post-publication semantic failure.

# 2026-08-16 - Preserve original identity arguments across resolved-state setup

- `BeginRuntimeTreeEnumeration` (`0x00406770`) resolves a nullable identity only to initialize iterator fields, but its nested-child branch passes the original identity—not the resolved node—to `GetNextRuntimeTreeNode`. Null identity therefore initializes the global root and still returns null for a nested tree. Do not normalize an original argument into a resolved pointer merely because that seems more useful; verify the call-site register immediately before the call.

# 2026-08-16 - Nonzero serializer inputs can emit empty semantic payloads

- `AppendScriptRuntimeFlags` (`0x004068F0`) suppresses output only when the entire input is zero. A nonzero mask containing none of its three recognized bits still emits `flags=;`. Keep the outer presence gate distinct from recognized-bit filtering.

# 2026-08-16 - Nested parser returns can overwrite outer loop controls

- Both conditional-tree parsers (`0x00406CB0`, `0x00406EA0`) reuse the outer scope local for the `/C` value parser's return. Missing `/C` input produces `0xFFFFFFFF` and terminates the outer scope loop immediately, ignoring later scopes such as `/GLOBAL`. Preserve the actual stack-local reuse rather than modeling nested parse results as disposable temporaries.

# 2026-08-16 - A nested parser return may deliberately not control its outer loop

- The inverse pattern occurs in `CreateOrUpdateRuntimeFixedNameNode` (`0x00407240`): `/FILE` calls `ParseScriptFileValue` into EAX while the outer scope code remains in ESI. Even a `0xFFFFFFFF` file result does not terminate the loop, so later flags are still consumed. Track the physical result register and the register tested at the loop latch independently.

# 2026-08-16 - Circular-list scans use the stored tail as an inclusive boundary

- The runtime named-list lookup and removal functions start at the head and stop only after examining the stored tail. This remains correct even though each tail links back to the head; translating these loops as ordinary null-terminated walks would never terminate on a miss.
- Several constructors rely on `HEAP_ZERO_MEMORY` to terminate a fixed-width copied name and then dereference the allocation without checking it. Preserve the distinction between constructors that explicitly handle allocation failure and these original unchecked paths.

# 2026-08-16 - Multi-field parser sentinels may be field-specific

- `ParseRuntimeNamedNode` treats the five `/ZONE` integer sentinels asymmetrically: fields one, two, four, and five retain their previous values on `0x7fffffff`, but field three is always assigned and then normalized from that sentinel to 1. Do not factor such sequences into one generic optional-field helper without proving identical behavior at every store.

# 2026-08-16 - Separate source coverage from fidelity acceptance

- A source address annotation proves representation, not that the function has passed the current raw-assembly and branch-test audit. Maintain a unique address-indexed acceptance ledger and derive progress from its rows; do not count prose milestone mentions because repeated audits and cluster summaries duplicate addresses.

# 2026-08-16 - Sequential token expansions are not necessarily exclusive

- `ParseScriptValueToken` tests PARAM and SVALUE with two independent branches. After PARAM rewrites the output buffer, that new value is immediately eligible for the SVALUE test. Replacing the second branch with `else if` would suppress a real chained expansion path.
- Several parser primitives deliberately accept empty tokens or commit cursors beyond bounded input on unterminated/truncated forms. Preserve and test their exact cursor arithmetic rather than constraining it to the input length.

# 2026-08-16 - Nested token-read failure may still feed an evaluator

- In `ParseScriptIntegerExpression`, the PARAM branch does not test the nested value-token result. Because that helper clears its output before failing, bare `PARAM` calls parameter evaluation with an empty name and can succeed. Do not add a conventional missing-argument early return unless raw control flow has one.

# 2026-08-16 - Lifecycle cleanup may intentionally be asymmetric

- Runtime media initialization returns failure after mutex creation fails without destroying the heap created immediately beforehand. Async-file shutdown deletes its global critical section after the final empty recheck without a matching leave. Preserve evidence-backed resource and lock ordering even when modern cleanup conventions would differ.

# 2026-08-16 - Async worker creation occurs after host publication

- `CreateAsyncFileHost` publishes the fully allocated host in the global list before calling `CreateThread`, and it does not check the returned thread handle. A null thread handle therefore remains part of a successful, externally visible host. Also, `ReleaseAsyncFileHost` has no subsystem-enabled guard and always touches the global critical section. Preserve these lifecycle boundaries instead of adding conventional rollback or state guards.

# 2026-08-16 - Shared async records acquire and release as a handle group

- Async record bit 2 means that all records with the same host and file handle share one underlying handle/buffer. Acquiring one such record sets ownership bit `0x10000` on every same-handle sibling, and releasing it clears the group. Closing a shared record frees no underlying resource while siblings remain; when exactly one remains, that sibling loses bit 2 and becomes the sole owner.

# 2026-08-16 - Async worker accounting follows requested reads, not completed reads

- `RunAsyncFileWorker` passes the requested tail/body count from EBP or EBX to `AdvanceAsyncHostWrite` after `ReadFile`; it does not pass the reported byte count. Short reads therefore advance producer cursor, file offset, buffered bytes, and available capacity by the request before short-read recovery runs. Preserve live-register provenance around API output parameters rather than assuming accounting consumes the API's reported result.

# 2026-08-16 - Palette remap exhaustion is a bounded byte-tolerance search

- `BuildPaletteIndexRemap` tests tolerances 0 through 240 in increments of 10. It does not test 250 and does not wrap the byte tolerance. When no candidate matches, it stores the exhausted candidate count: `0xEC` in 8-bit destination mode or `0x100` truncated to zero otherwise. Preserve the loop's post-increment bound rather than translating it as an unbounded search for an inevitable match.
# Custom-control bitmap state

- The GAG custom bitmap control stores its caller-owned state pointer in window extra offset zero. The confirmed 0x88-byte state has DCs at `+0x48/+0x4C`, palette handles at `+0x50/+0x54`, bitmap handles at `+0x58/+0x5C`, client `RECT` at `+0x60`, display depth at `+0x70`, source dimensions at `+0x74/+0x78`, cached bitmap identity at `+0x7C`, and caller-owned archive/comment string pointers at `+0x80/+0x84`.
- Preserve the distinction between complete BMP files and Win32 `RT_BITMAP` resources: CDF and caller-memory BMP inputs include `BITMAPFILEHEADER` and pass `input + 0x0E`, while `RT_BITMAP` begins at `BITMAPINFOHEADER` and is passed directly.

# Display-scene format selection

- A scene's `root_rectangle_callback` describes how its own pixels are filled and follows the source depth (8 or 16), even when the display root uses a different depth. The ordinary compositor is selected separately from the root depth, palette-source/palette-map presence, and scene flag `0x20`; a 24-bit root supports fill callbacks but no recovered compositor in this cluster.

# Display-scene compositor coordinate modes

- The six display compositors share two coordinate modes selected by flag `0x01000000`. Without it, the input rectangle is source-local and is rewritten to the clipped destination rectangle. With it, the rectangle is in global display coordinates, source/destination node origins are subtracted, and the caller's rectangle is not rewritten. Source flags bit 0 and `0x01000000` suppress all pixel writes in both modes.

# Main message loop termination uses the MSG field

# Flag parsers distinguish unknown tokens from extraction failure

- `ParseImageFlag` (`0x0040E580`) returns zero for a successfully extracted but unrecognized token. `0xFFFFFFFF` is reserved for null input, token extraction failure, or failed PARAM evaluation. This distinction lets callers rewind and reinterpret ordinary tokens as resource/tree names or typed strings; collapsing unknown and missing tokens prevents valid commands such as `/PRELOAD::ENTRY` from executing.

- `GagWinMain` (`0x0041CAE0`) does not branch on `GetMessageA`'s return value. It initializes `MSG.message` to `WM_COMMAND`, then unconditionally translates and dispatches after every retrieval and loops until `MSG.message == WM_QUIT`. Preserve this field-driven termination and the post-loop fatal-dialog check instead of substituting a conventional `while(GetMessage(...) > 0)` loop.

# Global addresses inside a cleared host block must remain aliases

- When raw instructions use a global address inside the single `InitializeGraphicsHost` allocation, model it as an alias of that embedded field rather than as an independent source global. `AcquireRuntimeLockRecord` (`0x00425F10`) uses `0x0047F840`, the host's `path_critical_section`, and `0x0047F864`, its `media_objects_parent_identity`; splitting either address creates an uninitialized lock or null named-node parent even though startup initialized the original storage correctly.
- The same storage-identity rule applies to global pointers used by unrelated-looking helpers. `SetScriptRuntimeRootIfValid` (`0x00404970`) and `SetActiveObjectField0824` (`0x00404980`) both read or write the pointer at `0x0043E2B0`; modeling the second helper with a separate `active_object` silently discards the 16-bit cursor-file selector even though its field offset is correct. Confirm both the base pointer's absolute address and the field offset before introducing distinct semantic globals.
- Audit aliases by absolute-address xrefs across all consumers, not by matching one semantic variable name per field. The original deliberately reuses `0x0047F828` for resource and game-DLL synchronization, `0x0047F898` for script time/property/presentation state, and the scene identity slots for temporary state transitions. Conversely, similar counters at `0x0047F868` and `0x0047F884` are distinct. Both artificial splitting and artificial merging can silently pass isolated unit tests.
- Pointer-valued host fields may be passed through an integer-typed legacy API slot. `ConstructRuntimeResource` loads `0x0047F7D8`, the async-file-host field at `+0x878`, as `OpenCdfArchive`'s integer alternate-stream argument. Splitting that address into a standalone integer silently changes an archive from async-record mode to Win32-HANDLE mode; later consumers then interpret HANDLE values as `AsyncFileRecord *`. Follow the absolute storage identity even when the decompiler's nominal types differ.
- A Win9x handle used as an integer timing marker may not remain numerically meaningful on modern Windows. GAG passes `HWAVEOUT` through `WOM_DONE.wParam` into its mixer, but its callback also records `timeGetTime()` in `lParam`. A modern compatibility branch can use that existing timestamp while keeping the original handle-based path in fidelity builds; keep `WOM_OPEN` on its original marker because it has no callback timestamp.

# Root-cause fixes require attempt cleanup

- Diagnostic probes and experimental compatibility changes are temporary evidence-gathering tools. Once the root cause is proven, review all changes made during that investigation and remove every unrelated or superseded attempt. Retain only the smallest root-cause fix, its necessary compatibility annotation, and focused tests; verify the cleaned result rather than reporting success from the diagnostic build.

# Script loop restarts can commit the parser cursor

- When reconstructing a command executor with shared cleanup labels, distinguish a restart that restores the saved pre-opcode cursor from a direct branch that preserves the opcode handler's advanced cursor. In `ExecuteScriptCommands` (`0x00421530`), successful `/PLOAD` and `/SPLOAD` jump directly to the outer-loop labels and bypass the common cursor restore. Collapsing both paths into one `restart_outer` disposition can rerun the load forever; model the restart and cursor-commit effects independently and test both the next cursor and link traversal.

# Script rectangle keywords can use different coordinate conventions

- Do not assume similarly shaped `/RECT` and `/POS` commands store the same four quantities. In `ParseRuntimeTreeLink84` (`0x0040AAC0`), `/RECT` stores absolute left/top/right/bottom edges, while `/POS` stores x/y/width/height and converts the extents to right/bottom. Confirm each command branch from raw stores and arithmetic; reversing them can create impossible hit-test ranges while broad fallback regions conceal the parser error.

# Preserve selector arguments used for identity lookup

- A tree constructor may search the global tree using an explicit parent-selector argument even when it also has direct access to the global root. In `CreateRuntimeTreeNode` (`0x00405410`), EDX is preserved across the first lookup and selects whether the new node is a root or child. Substituting the global root pointer silently forces null-parent loads to become children and prevents later root-publication logic from selecting them. Verify register provenance at the lookup call and test the observed selector identity directly.

# Fixed-size compatibility windows need style refresh on transitions

- `AdjustWindowRect`'s menu argument must describe whether the created top-level window actually has a menu; it is not a request to account for `WS_SYSMENU`. Passing `TRUE` for a menu-less fixed-frame window enlarges the client by a menu row, so a separately centered fixed-size rendering child exposes matching top/bottom bars. Keep startup, restore enforcement, saved-position validation, and tests on the same adjusted outer geometry.
- For a framebuffer whose dimensions the game owns, a movable modern Window mode should use caption/system-menu/minimize styles without `WS_THICKFRAME` or `WS_MAXIMIZEBOX`. When switching an existing popup between Full Screen and Window, apply `GWL_STYLE` first and include `SWP_FRAMECHANGED` in the following `SetWindowPos`; using the framed style only in `AdjustWindowRect` does not create a visible or draggable non-client frame.
- When Window-mode placement belongs to Windows or the user, using `CW_USEDEFAULT` only at creation is insufficient if a later layout pass supplies explicit top-level coordinates. Guard every top-level `SetWindowPos` in that compatibility path while retaining separate capture-child placement required by the framebuffer.
- Persist compatibility-window placement per user and apply a valid saved point in `CreateWindowEx`, rather than repositioning after the window becomes visible. Validate the reconstructed outer rectangle against the current monitor topology and fall back to `CW_USEDEFAULT` when displays changed or the value is malformed.
- Before interactive retesting, terminate the previous instance. GAG rejects a second process through `FindWindowA("FlcAppClassNT", nullptr)`; if an old unclosable instance survives, rebuilt launches silently fail validation and every visible observation still describes the stale executable.
- `SetWindowLong(GWL_STYLE, value)` replaces dynamic bits as well as structural frame bits. If it runs after `ShowWindow`, include or preserve `WS_VISIBLE`; otherwise DWM can leave a stale visual frame while `IsWindowVisible` is false and Windows excludes the entire parent/child hierarchy from hit testing. This presents as a transparent, unactivatable, unmovable, unclosable window even though its thread is healthy in `GetMessage`.
- A `WM_WINDOWPOSCHANGING` compatibility handler must preserve both Windows' incoming restore coordinates and `SWP_NOMOVE`. Preserving only `x/y` is insufficient when the original helper clears that flag, because coordinates are unspecified when `SWP_NOMOVE` arrives set; conversely, retaining the flag permits fixed-size enforcement without taking ownership of top-level placement.

# Sequential flag assignments may intentionally reuse the pre-update value

- Application-state routines sometimes load flags once and issue two successive assignments derived from that same saved value. The second assignment can therefore discard bits apparently added or cleared by the first; examples occur in inactive-state and current-state activation handling. Preserve register provenance across sequential stores instead of algebraically combining them as cumulative mutations.

# Shutdown message suppression has a private-message exception

- Both GAG application window procedures consume ordinary messages without calling `DefWindowProcA` while application flag `0x80000000` is set, but private messages `0x30F`, `0x310`, and `0x311` remain forwardable to the game child window. Apply the inclusive exception before ordinary message dispatch.

# Narrow x86 stack parameters remain ABI-significant

- `InitializeGraphicsHost` (`0x0041FA00`) receives width as signed 16-bit and height as unsigned 16-bit stack parameters even though x86 argument slots are four bytes. Preserve the narrow source types and explicit caller conversion: matching only the slot width can hide sign/zero-extension behavior and produces an imprecise recovered interface.
- For indirect x86 callback calls, do not trust a decompiler argument list while the callback global is untyped. Reconstruct the ABI from register setup and stack pushes immediately before the `CALL`; `RuntimeGameWindowProcedure` at `0x00423201` proves its DLL callback uses ECX/EDX plus two stack arguments, i.e. a four-argument `__fastcall` call.

# Fixed-stride overflow can reveal the real global backing allocation

- Do not add a detached safety element when the original indexes one slot beyond a nominal fixed-size global array. Compute `base + index * stride`: GAG's sound slot `0x400` begins exactly at `0x0043E048`, where the following sound globals live, so its field writes are intentional cross-views of those globals rather than writes to an independent 1025th slot. Model one contiguous backing image and test the side effects through both semantic views.
- A global can have two destructive roles even without differing types. Display-mode address `0x0043EAD8` is both the list-construction tail and the enumeration cursor; beginning or advancing enumeration overwrites the tail. Preserve the shared address and add a regression test for the overwrite instead of retaining two convenient C++ variables.

# Adjacent bitmasks require behaviorally opposing tests

- Same-typed adjacent fields can compile and mostly behave when swapped. `ScriptObjectState +0x47C` is the command mask used by pointer-scene eligibility and hover-mode transitions, while `+0x480` is the current active-field mask used to serialize and evaluate ON/OFF object values. Confirm the raw displacement at every consumer and test with one mask set while the other is clear; tests that give both masks compatible bits cannot detect the swap.
- An overlapping array view can be rotated by fields while retaining the correct stride. GAG's 0x28-byte runtime scene slots are the command-definition records themselves (`name +0`, visual `+0x20`, flags `+0x24`), not a second array beginning at the first visual pointer. A one-record rotation left visual switching functional but associated each scene with the next command name, breaking special `IView`/`Hide` behavior. Anchor every semantic field—not only stride and one convenient pointer—against absolute addresses.

# Re-evaluate decompiler arithmetic at unsigned wrap boundaries

- A decompiler expression such as `(-(failure) & 0xFFFFFFDF) + 0x20` must be evaluated with 32-bit wraparound. In `ParseScriptValueToken`, success produces `0x20`, while failure produces `0xFFFFFFFF`, not zero. Translating the apparent boolean selection without checking the emitted `CMP/SBB/AND/ADD` sequence can turn a hard parse failure into a valid empty token and publish malformed template-generated runtime objects. Add tests for both result values whenever compact flag arithmetic controls parser success.

# Test seams must not replace direct production calls

- Keep dependency injection at an explicit testing boundary; do not replace a statically resolved original call with a mutable production function pointer merely to make a small function observable. `EnableRuntimeSubsystem` and `DisableRuntimeSubsystem` directly call `ToggleRuntimeSoundState` in GAG. An uninitialized artificial callback turned an ordinary menu toggle into a garbage indirect jump. Prefer asserting the real callee's state change, or wrap the entire caller only in test code, when raw instructions show a direct call.

# Borderless fullscreen should virtualize presentation, not game geometry

- Keep the recovered framebuffer, dirty rectangles, script coordinates, and binary-facing structures at their original dimensions. Implement modern Full Screen as a fixes-only presentation layer: resize a child viewport, scale the final blit, and inverse-map input before any original consumer sees it.
- Save the complete framed Window-mode rectangle before replacing the top-level style. A persisted Full Screen startup must preload that rectangle (or a deterministic valid fallback) before the first fullscreen layout, otherwise the initial monitor-sized popup can be mistaken for the Window-mode restore geometry.
- For aspect or integer letterboxing, make the rendering child exactly the viewport size and let the black top-level parent own the surrounding area. This naturally keeps bar input outside the game surface and avoids embedding viewport offsets in the 640x480 game coordinate system.
- Scaled dirty clears need floor mapping for left/top and ceiling mapping for right/bottom so every destination pixel touched by a source rectangle is covered. Full-frame presentation can use one deterministic `StretchBlt` with `COLORONCOLOR`; Window mode should retain its original unscaled blit path.

# Fast compatibility transitions can expose level-triggered script actions

- A script event tied to a mouse-down state can be reevaluated several times before the physical release. If a modern compatibility transition completes much faster than the original operation and changes the state used to select the next command, one click can request alternating transitions. Gate the compatibility action once per physical press while leaving the original script dispatcher intact.
- Rearm such a gate on the next button-down, not button-up, when the script worker can process an already-active event after the UI thread has received the release. Do not add mouse capture merely to protect the release across the transition: changing capture ownership while the graphics child is moved or resized can disrupt later input routing, and the per-press script-action latch does not require capture.
- If a cross-thread `SendMessage` receiver calls `ReplyMessage` before finishing its callback, duplicate suppression inside the receiving window procedure can be too late: the released sender may issue another synchronous send and block before the busy UI thread can enter that procedure. Put the compatibility gate at the sending boundary so the duplicate call is never made.
- When an original input gate is deliberately paused during a borderless resize, preserve the physical button release needed to terminate the initiating level event, but continue rejecting resize-generated moves. A stationary screen cursor can map to a different game coordinate after viewport relocation; processing that synthetic move before the release changes the active region and releases the wrong interaction.

# Indirect string tables require slot-level verification

- When an instruction loads a string through an absolute pointer-table slot, read the DWORD stored in that exact slot and inspect the bytes at its target. Do not flatten the indirection based on a nearby literal or a decompiler-rendered string reference. Adjacent slots can intentionally point to related but noninterchangeable names: in `GagMainWindowProcedure`, `0x004408EC` points to `NewGame.cfg`, while adjacent `0x004408F0` points to `NEWGAME`. Selecting the neighboring slot produced a plausible extensionless name that could never match the exact CDF entry lookup.
- Audit these tables instruction-first: enumerate code references to readable four-byte data slots, retain slots whose initialized value points to a valid string, group every consumer by the exact slot, and then compare each consumer with its source function. Scan narrow, wide, and indexed forms separately. Requiring a real code reference avoids false pointer candidates created when four ASCII bytes inside an ordinary string happen to form an in-image address.
- When several instructions load the same slot, represent the recovered value with one shared source constant where practical. GAG slot `0x00440900` supplies `AutoSave.cdf` to startup detection, Resume Game, and shutdown state saving; independent literals allowed two consumers to drift to adjacent `CREDITS` even though the third remained correct.

# Fastcall callees can stage later calls across an intervening call

- In x86 fastcall wrappers, pushes performed before an intervening fastcall may be staging stack arguments for a later callee rather than arguments to the immediate callee. Track ESP after every saved register and push. `ActivateRuntimeTreeWithNotifications` pushes incoming argument 4 and `treeName`, calls a register-only resource resolver, then loads incoming argument 3 into EDX before calling `CreateRuntimeTreeNode`; argument 3 is the parent selector and argument 4 is creation text. Collapsing them into one semantic value turned the valid `0xFFFFFFFF` global-root sentinel into a string pointer. Test recovered multi-argument forwarding with behaviorally distinct values, including sentinel/null combinations, rather than passing the same pointer in every slot.

# Binary chunk scanners require literal-byte verification

- When original code scans loaded binary data using a pointer to a four-byte global, inspect the global's raw bytes rather than inventing a semantic integer marker from the decompiler's generic data label. `ConstructRuntimeResourceObject` scans WAV chunks for ASCII `data` at `0x0044216C`; replacing that literal with integer 1 can plausibly match PCM header fields, queue unrelated bytes, emit garbage audio, and only crash later in the mixer. Regression fixtures should include a plausible false integer marker before the real chunk ID so the distinction is observable.

# Similar slot scans can have opposite predicates

- A subsystem may scan the same slot array twice for opposite purposes. In `CreateSoundHandle`, the first format-selection scan advances over inactive slots and stops at the first active sound to decide between preserving the mixer with conversion and rebuilding it; the later handle-allocation scan advances over active slots and stops at the first inactive slot. Reusing one convenient “find free slot” loop for both silently breaks mixed-format playback. Verify the exact `TEST` branch direction at each scan and cover active-plus-free and entirely-inactive layouts separately.

# Adjacent parser keywords must be verified against their dispatch cases

- Similar adjacent string globals can be transposed while leaving parsing apparently functional. Verify each literal's exact address and numeric parser code, then follow that code through dispatch to its behavioral consumer. In GAG, `font` at `0x0043E468` is property `0x10` and creates a fixed-name resource node, while `time` at `0x0043E470` is property `0x0F` and publishes an integer setting. Swapping them silently prevented subtitle font construction while subtitle text resources still loaded normally.
- Keep parser and serializer mappings covered together. A round-trip-looking table test alone may preserve the same wrong bijection; include behavior-specific expectations, such as fixed-name serialization using `font` and runtime operation 8 serialization using `time`.

# Recovered modal UI depends on PE resources as well as code

- A correct `DialogBoxParam` reconstruction silently fails when its numeric dialog resource was not linked into the rebuilt executable. The caller may already have paused gameplay or entered a transition, making a missing resource look like a deadlock rather than a packaging failure. When modal UI never appears, compare the rebuilt PE resource table against the original before instrumenting the message loop.
- Locally extracted copyrighted resources can remain optional in source control while being auto-detected when present. Keep a resource-equivalence acceptance test enabled whenever the extraction directory is selected so a successful C++ link cannot conceal a resource-free executable.

# Recover array indices from stack offsets, not decompiler local names

- A decompiler may split one caller-owned array into an initial array local followed by separately named scalar and structure locals. Recover the original indices by subtracting stack offsets from the array base. In `ProcessAvailableRuntimeGenericChildren`, the state buffer starts at `-0x3c`; x/y at `-0x28/-0x24` are state[5]/state[6], not state[2]/state[3]. Using the latter treated a duration and text pointer as coordinates and placed subtitles far off-screen. Regression tests should give plausible candidate indices distinct values so the wrong interpretation cannot pass.

# Initialized BSS addresses may be live fields, not constants

- A writable address whose initial bytes look like an empty string may be a field inside a larger runtime object. Before flattening it into a constant, subtract the bases of known host allocations and audit runtime writers at the resulting offset. GAG address `0x00480158` is `ScriptRuntimeRoot +0x848`, populated by the script `class` property and later used to select the inventory subtree for removal; treating its initial zero bytes as a permanent empty string left the overlay active and intercepted world-object drops.

# Legacy HKLM persistence depends on process manifest behavior

- A faithful 32-bit reconstruction can call the same HKLM registry APIs and still lose settings on modern Windows. An embedded `requestedExecutionLevel` manifest disables legacy registry virtualization; if ordinary users have read-only access, the original write fails instead of being redirected to the per-user VirtualStore. Check the built PE manifest, key ACL, real and virtualized values, and ignored Win32 return codes before blaming state masks or shutdown flow. Keep an HKCU replacement explicitly fixes-only rather than presenting it as recovered original behavior.

# Resizable compatibility windows should isolate scaling in a child viewport

- Keep the top-level client freely resizable and make the rendering/input child exactly the calculated viewport. Centering that child lets the parent's background supply letterboxing and naturally prevents bar-area mouse events from entering game-space input.
- Derive the minimum top-level tracking size by applying the active framed style to the minimum client rectangle in `WM_GETMINMAXINFO`; constraining raw outer dimensions does not guarantee an exact minimum client area.
- Treat presentation scaling as a viewport property rather than a fullscreen property. The same final-blit, dirty-clear, repaint, and inverse-coordinate paths then work for fullscreen and resizable Window mode, while a viewport matching the framebuffer can retain the unscaled fast path.
- Persist the normal framed rectangle, not the current minimized/maximized bounds. `WINDOWPLACEMENT::rcNormalPosition` preserves the user's preferred size for transient window states; validate its size and monitor intersection before restoring, and retain older point-only values as a migration fallback.

# Paired text measurement and drawing passes must share terminators

- When a text engine first measures a segment and later draws it, audit both switch tables for identical structural terminators. A missing draw-only stop can render parser section labels beyond the measured dialogue even though allocation and positioning appear correct.
- Verify switch cases from the raw jump table when a decompiler or manual transcription may have dropped a low-information branch. GAG's generic subtitle renderer maps `'['` directly to cleanup because `.RUS` files use `[END]` as a section marker; omitting that one case exposes `END` as subtitle text.

# Game DLL names resolve through the active resource directory

- A bare `/GAME` DLL name is not resolved relative to `gag.exe`. GAG first combines it with `RuntimeCommandLoopState::resource_directory`, then passes that full path to `LoadLibraryA`. In a CD-style installation this can produce a path such as `E:\\XTETDLL.DLL`; placing a reconstructed DLL only beside the executable will therefore leave the script's original load attempt failing and immediately continuing.

# Synchronous host callbacks can re-enter a DLL window export

- A DLL that calls `SendMessage` on its host child window can re-enter its own ordinal window callback before the initiating callback returns. `PeekMessage` may likewise dispatch pending sent messages while draining posted input. When the original synchronization primitive is a Win32 critical section, preserve its same-thread recursive semantics; replacing it with `std::mutex` can throw `std::system_error` inside the callback. Retain the original inactive-state gate before locking as well, so a worker synchronously reporting failure cannot deadlock against UI-thread re-entry.

# Scaled hosts must map cursor queries back to fixed scene coordinates

- An original DLL may ignore mouse-message `lParam` and query the cursor with `GetCursorPos`/`ScreenToClient`. If a compatibility host scales the actual child client while leaving the DLL's scene/framebuffer fixed, inverse-scale that queried client point before scene hit testing. Preserve exact-size identity and outside coordinates so the compatibility adapter does not change original unscaled behavior or turn an outside point into a hit.

# Global synchronization must outlive objects whose destructors use it

- C++ globals in one translation unit are destroyed in reverse declaration order. If a global owner contains a worker whose destructor stops/joins code that can acquire a separate global mutex, declare the mutex before the owner. Declaring it afterward creates an unload-only use-after-destruction hazard even when every runtime lock acquisition is otherwise correct.

# Verify compact decompiler arithmetic against condition-code instructions

- Decompiled boolean-to-signed-state expressions can obscure `CMP`/`SBB` idioms. Evaluate the raw instruction sequence for every input class before assigning semantic constants. XTET's spawn sequence `DEC; CMP 1; SBB AL,AL; AND AL,2; DEC AL` yields `+1/-1`; reading the compact pseudocode as `-3/-1` removed horizontal facing variation while leaving all later orientation and mirror tables correct.


# Modal script messages can require cursor-preserving restarts

- A synchronous `/MESSAGE` can start a modal application operation and return only after a runtime-state transition. In `ExecuteScriptCommands`, a nonzero accumulated command-loop result branches directly to the outer loop and bypasses the common saved-cursor restore. Model that as `restart_outer_commit_cursor`; restoring the pre-message cursor repeats the modal command forever, while suppressing the intended post-operation menu reload would conceal the interpreter error and prevent state-dependent entries from updating.
- A cursor-preserving outer restart must not be implemented as “read the advanced cursor, then write it back.” Tree-changing commands such as `/PLOAD` may free the current link during dispatch. Preserve the original direct branch by performing no post-dispatch access through the parser/link pointer.
- Audit every branch to the interpreter's outer-loop label, not only modal and load commands. XTET-confirmed GAG paths for root `PEXIT`/`CONTINUE`, a nested deactivation with no continuation, and session reset also bypass the common parser tail because their current link can already be gone.
- When a persisted UI preference and a transient script flag encode inverse views of the same feature, startup content can legitimately overwrite the transient flag after preference restoration. Do not repair the divergence in a read-only UI query. For a compatibility persistence layer, toggle the persisted preference at the command boundary and derive the transient inverse from the new value; preserve the original transient-driven command in fidelity builds.

# Separate spatial hit testing from scripted aim resolution

- A minigame can use pointer coordinates only to select a target zone, then decide success from an independent animation phase sampled on button release. Trace the selected region, queued interaction record, matching event, and sampled phase separately before changing coordinate conversion. In GAG's rat game, all rat-zone clicks matched correctly; held duration controls a 50-frame gun animation, only frames 8-17 are aligned, and impacts away from the target are intentional feedback for other phase bands.

# Size parameter-evaluation temporaries for every materializable type

- A typed parameter evaluator may parse and write the actual value before comparing its type with the caller's expected type. Therefore, even scalar-only consumers must provide storage large enough for the widest materializable value and read the scalar member only after successful validation. In GAG, both integer-expression PARAM and image-flag PARAM callers use 0x20-byte temporaries because a mismatched string is written before the evaluator reports failure; reducing either temporary to one DWORD corrupts the caller's stack.

# Audit content with the shipped interpreter grammar, not a conventional grammar

- GAG's script lexer is deliberately permissive: an unknown extracted property, scope, opcode, or image flag maps to zero and scanning continues, and scope arguments may follow whitespace without a colon. Consequently a strict parser can report many malformed scripts that the shipped interpreter treats as harmless skipped text. Classify each anomaly by its original runtime cursor effect before attributing a mechanic to it.
- A misspelled structured-control terminator can still appear harmless when the scanner reaches the next section boundary, but inspect all text between the typo and that boundary: commands after the typo may be swallowed. Separately validate that any apparently exhaustive conditions really cover every reachable entry state.
- For large generated navigation scripts, validate every static transition target and compare reciprocal/structurally analogous cells. This distinguishes an actual missing live target from unresolved names in abandoned prototype blocks.
- When auditing orphaned archive assets, distinguish four categories: no script reference, references only on disabled comment lines, references in unreachable sections, and executable/implicit loading. Search embedded executable strings before calling a script-unreferenced metadata file unused, and inspect active commands that still name a resource whose declaration was disabled.
- Build section reachability with the runtime's actual identity rules: GAG resource filenames are case-insensitive, section names are exact, and duplicate section names resolve to the first definition. Count every conditional edge to keep the result conservative, treat leading-`*` property continuations as disabled, seed executable roots separately from external installer/config roots, and report later duplicate definitions as shadowed rather than ordinary graph nodes.
- Missing targets from reachable sections still require semantic classification. A zero-area zone, an intentionally empty transition operand, an empty visual-state placeholder, or a story-state-incompatible copy on the other disc is not equivalent to a live navigation failure. Strong typo evidence comes from an adjacent near-identical definition and a repeated structural pattern, such as `MZ.CIN::09012xy` versus shipped `[0901xy]` among the maze's special map-position helpers.

# Saved registers beyond a callee's stack cleanup are not hidden arguments

- When a wrapper saves a nonvolatile register and later pushes arguments for another call, calculate the callee's formal stack extent from its calling convention and return cleanup before assigning meaning to the saved word. A value physically adjacent to pushed arguments is not an argument if it lies beyond that extent.
- Do not preserve unexplained register residue with inline assembly merely to make a reconstruction look instruction-identical. First prove that the callee reads it or that behavior changes with it. In GAG, `SelectRuntimeResource` saves ESI, but the called constructor consumes only ECX, EDX, and six stack DWORDs (`RET 0x18`); propagating ESI as a ninth `loop_animation` parameter created a compiler-specific false ABI with no observable semantics.

# Calling-convention migrations must update the complete type graph

- Removing an architecture-specific calling convention requires changing definitions, declarations, function-pointer aliases, injectable API tables, test doubles, exported DLL functions, and host-side import casts together. A partial migration can still compile in isolated translation units while producing incompatible indirect calls or callbacks at runtime.
- Keep operating-system-owned conventions such as `WINAPI` and `CALLBACK` distinct from application-specific recovered conventions. Removing a game's x86 fastcall annotations for portability does not authorize changing Win32 callback or imported API contracts.

# Retiring a recovered DLL boundary requires an explicit linked lifecycle

- When scripts name a single known extension DLL but the implementation is linked into the host, preserve the script-visible filename as a compatibility identifier and replace module loading with a closed direct-binding adapter. Reject unknown names before mutating runtime state instead of retaining an unnecessary generic loader.
- A formerly unloadable DLL becomes process-lifetime static state. Add an idempotent shutdown that stops worker threads before taking their shared lock, releases audio and resource ownership, invalidates archive views before freeing backing bytes, and clears host pointers and callbacks so repeated initialization is safe.
- If host and linked component retain parallel recovered context types, protect the direct cast with size and field-offset assertions across every consumed field. A successful native build alone does not prove the callback-side layout matches.
- A fixes-owned in-memory script should still enter through the normal resource cache and type classification path. This preserves parser lifetime and release behavior while allowing a command-line startup mode to select crafted orchestration without adding a loose configuration file.
- Supplying virtual resource bytes is insufficient when construction dispatches on a separate pre-load type query. Teach both the type detector and byte loader about the virtual resource; otherwise an in-memory configuration can be successfully loaded only after it has already been misclassified and constructed as opaque generic data.
- A crafted standalone scene must reproduce the script-owned interaction infrastructure as well as its primary image and terminal command. If the host hides the operating-system cursor and normally draws cursor resources selected by command/zone priority, omitting those mouse definitions and zones leaves a fully clickable extension visually cursorless.
- Extension pause notifications can target both extension state and the surrounding host plan. When a crafted standalone plan itself owns essential UI such as a software cursor, preserve the extension's internal pause but do not suspend that host plan; retain the original host suspension handshake for ordinary embedded/story launches.
- Derive scripted hover zones from the extension's actual parsed control geometry, including edge controls. Broad rectangles approximating button banks can leave independently placed controls clickable but visually cursorless.
- Keep proprietary payload extractors aligned with the actual deployment contract. When the runtime consumes one archive, the extractor should accept the evidence file, locate that payload through bounded format-aware parsing, and emit only the runtime filename; checksums, manifests, resource scripts, and output-directory packaging add no value unless another consumer requires them.
- When a script-visible compatibility filename determines a companion payload, derive the payload basename at that script boundary and carry it through initialization. Hardcoding the currently supported name in a lower-level provider duplicates policy and lets the extractor, script handler, and runtime silently diverge.
- A CMake language-standard migration must cover production libraries, executables, utilities, and every test target. Updating only the target that first needs a new library facility can leave shared headers compiled under inconsistent language modes.
- A command-line extractor that only separates a basename and replaces an extension does not need `std::filesystem`. Keep the input non-owning with `std::string_view`, parse both slash forms explicitly, and materialize an owned string only at APIs that require null termination or for the relative output filename.
- If a codebase style uses unqualified C-compatible scalar typedefs, include `<stdint.h>` and `<stddef.h>` rather than relying on `<cstdint>` or `<cstddef>` to expose implementation-specific global aliases. Keep genuinely C++-library-owned types namespace-qualified.
- A PE payload extractor need not mirror the entire DLL in memory. Record the file size once, bounds-check each offset before seeking, retain only compact section/resource-directory records, and allocate the byte vector when the final resource RVA and size are known.
- For a narrowly scoped extractor operating on known evidence, direct checked `seekg`/`read` operations can be a clearer validation boundary than carrying total-file-size state through every parser function. Retain enough PE section information to translate RVAs, and fail immediately whenever the stream enters an error state.

# Pointer-derived identifiers must widen through the entire dataflow

- Changing only the final pointer cast is insufficient when an identity passes through structure fields, event queues, injectable API tables, parser results, or test doubles. Widen the producer, every intermediate slot, comparison, return type, and consumer together.
- Heterogeneous decompiler DWORD arrays often contain both numeric values and pointers. Replace them with a typed structure when field semantics are known, or with native-width slots when the record is genuinely heterogeneous. Reinterpreting a widened structure as its old DWORD array can read padding as data.
- Fixed binary interface indices are properties of the original interface layout, not the host pointer width. A COM vtable byte offset recovered from x86 must still be converted to its method index using four-byte original slots before indexing a native pointer table.
- When two recovered x86 globals have identical fields and confirmed identical addresses, represent the relationship as a type alias and one shared object. When their overlap is only an artifact of adjacent fixed-address storage, split them before widening fields so one native pointer cannot overwrite another logical value.

# Preserve the installer registry view across architecture migrations

- A 64-bit process defaults to the 64-bit registry view and therefore cannot see installation data written by an original 32-bit installer. When the installer remains the authority, fixes-enabled access must explicitly request `KEY_WOW64_32KEY` rather than duplicating or migrating the installation key into the native view.
- Apply the same explicit view to compatibility-owned per-user settings when Win32 and AMD64 builds are expected to share preferences. Keep original-fidelity access flags unchanged outside the fixes boundary.

# Fixed image offsets cannot survive native structure widening

- A scratch buffer addressed as `base + original_x86_offset` may safely alias an x86 image-layout structure but becomes an unrelated overwrite after pointers, handles, or synchronization primitives widen. This is especially dangerous when the displaced target is a `CRITICAL_SECTION`: the later failure appears inside `ntdll`, far from the earlier zeroing operation.
- During a native-width port, eliminate every raw fixed-offset alias into a widened aggregate. Preserve confirmed logical overlap with typed members; move independent image globals and scratch regions to separate typed storage, and update initialization and shutdown as a complete ownership unit.
- Audit pointer identities passed to threads and callbacks after splitting an image overlay. Two x86 views may have shared the same base address even though their reconstructed C++ types describe different slices; once represented as separate native objects, an asynchronous entry point must receive the object matching the type it actually casts and reads.
- Audit consumers as well as storage declarations. A function parameter may still carry the former shared-image base and decode fields with fixed offsets long after the underlying globals were split; replace each decoded field with its typed owner, including indirect UI callbacks such as bitmap capture, screenshot naming, activation, and keyboard forwarding.
- Never model an x86 DWORD write into a pointer field as a `uint32_t &` alias in a native-width build. Reset or assign the complete typed pointer; otherwise stale upper bits survive and produce plausible-looking invalid identities later.
- Embedded operating-system structures also invalidate original aggregate offsets. If a buffer packs Win32 metadata followed by payload, derive each native structure's offset from `sizeof` and `alignof` and derive the payload/stride from the resulting end. GAG's original 18-byte `WAVEFORMATEX` plus 32-byte x86 `WAVEHDR` placed PCM at `+0x32`; retaining that offset with the 48-byte AMD64 `WAVEHDR` let WinMM overwrite the first 16 PCM bytes and produced startup clicks.
- Separate decompiler overlay structures can share valid x86 offsets yet diverge after any earlier pointer or nested descriptor widens. When an object has a known canonical owner type, access its semantic fields through that type instead of preserving a padded overlay; for unavoidable common-prefix casts, add compile-time `offsetof` equality checks for every consumed field. Cursor rendering is especially deceptive here because independent hit testing can continue to work while scene coordinates are silently written into the wrong members.
- Splitting an original aggregate does not remove a function's observable return contract. If initialization returns a host/status view that callers query after a canonical subsystem object is initialized, explicitly publish the recovered result into that view or change the contract and all callers together. Otherwise control selectors can retain a zero default even while the underlying subsystem is correctly configured; GAG's 16-bit framebuffer was valid while its returned depth stayed zero and selected indexed cursor assets.
- Never retain a `uint32_t[]` view of a recovered structure merely because all x86 fields occupied DWORD slots. Once any field is known to be a pointer, pass and decode the canonical typed structure across adapters. GAG's serialized-script buffer is `{ uint32 length, uint32 capacity, char *data }`; treating it as three DWORDs on AMD64 truncated and misaligned `data` during exit autosave packaging.
- Some original x86 stack records deliberately reuse spare character-array storage and alias later pointer fields with a child control's private state. Widening two C++ overlay views cannot preserve both offset maps. For portability, split the child state into a real nested object, explicitly connect aliased semantic fields, and carry an owner back-reference when the child HWND must return the parent record. This preserves behavior without depending on pointer-size-specific overlap.
- When a compatibility implementation replaces a physical subsystem transition with a presentation-only operation, audit synchronization that follows the original transition as well as the transition call itself. A wait that was necessary for device teardown may become both unnecessary and deadlocking when persistent runtime activity is valid; bypass it only inside the compatibility path and retain the fidelity path unchanged.
- A decompiler may express a callback argument through a compact common-prefix overlay even though the producer passes a larger canonical object. Once pointers widen, use the producer's canonical type for every field beyond invariant leading bytes. GAG's state-activation callback receives a `RuntimeTreeNode`; retaining the x86 overlay moved the apparent parent and `NOSAVE` fields, allowing title-menu serialization to overwrite the gameplay autosave while leaving the CDF archive itself perfectly valid.
- Distinguish the current runtime-tree identity from the active mouse-region identity even when recovered x86 records place both near the same aggregate tail and both accept common-prefix objects. Confirm the field through independent consumers at the original offset. GAG's activation callback compares its tree with the script executor's current-tree field at game-context `+0x964`; comparing it with the active mouse region rejects every root activation and silently drops menu policy flags such as `NOSAVE`.

# Keep CMake source ownership aligned with the directory tree

- A source directory should own its targets and nested components through its own `CMakeLists.txt`; the root should retain project-wide policy, dependencies, options, and top-level orchestration.
- Formatting manifests should be explicit and maintained beside the sources they cover. Have each source-owning subdirectory publish its manifest upward, then define repository-wide format targets after all participating subdirectories have been entered. This avoids configure-time globbing while keeping the root independent of nested file layouts.
- Source-code and build-script comments should start with a lowercase letter and should not end with a colon.

# Keep a software cursor authoritative on client re-entry

- Hiding a native cursor once is insufficient when a window class supplies a cursor. Windows can restore the class cursor through `WM_SETCURSOR` whenever the pointer re-enters the client area. A software-rendered cursor child should also request `TME_LEAVE`: crossing directly into non-client space or another window may otherwise bypass a separately queued cursor-region selector. Queue an outside coordinate on leave so the selector cannot retain the old active region and reject same-region re-entry as unchanged. Re-arm tracking on entry and delegate non-client cursor handling so resize borders and window chrome retain normal cursors.
- Treat cursor visibility, pointer-region selection, and runtime scene suspension as separate state paths. If leaving suspends scene transitions, re-entry must balance that suspension before the active-region fast path can suppress another selection. If the desired outside behavior is no cursor at all, suspend the software scene directly while retaining native-cursor suppression; routing leave through a parent path that shows the native cursor produces an unwanted arrow or class cursor outside the client.
- A deferred scene identity is pointer-derived state. Its storage, transition callback, adapter, and tests must remain pointer-sized together; otherwise re-entry may appear logically balanced while restoring a truncated identity on a 64-bit build.

# Separate recovered x86 layouts from native semantic state

- Never reproduce an original DWORD pointer publication by writing a native pointer at the same byte offset inside unrelated scratch storage. Preserve the confirmed behavior with a dedicated typed field; otherwise AMD64 writes can overwrite two original slots or a neighboring inline buffer.
- A Win32 structure with pointer-sized fields cannot be overlaid with an original x86 all-DWORD helper even when later integer fields have matching names. Copy the semantic fields into the recovered helper and copy mutable results back. `WINDOWPOS` is a critical example because its two leading `HWND` fields expand before `x`, `y`, dimensions, and flags.
- Fixed serialized formats should use packed file-view structures and compile-time size/offset checks. Live runtime state should use native typed structures and explicit conversions; do not make a single overlay serve both domains.
- When original storage is deliberately polymorphic, expose named union aliases for each confirmed interpretation instead of scattering casts between unrelated node types. Use a typed common prefix only when every consumed field has compile-time `offsetof` equality checks.

# Replace native modal UI at the orchestration boundary

- Keep recovered native dialog procedures intact for fidelity builds, but branch to a compatibility-owned controller before dialog creation. This makes it structurally impossible for the compatibility path to instantiate child controls or secondary prompts while preserving the original implementation as executable evidence.
- A synchronous in-engine modal can use the host window's message pump when its active controller intercepts input before normal game dispatch. Render the overlay as higher scene nodes, leave the underlying scene unchanged, and release only overlay-owned scenes when the modal completes.
- Separate save discovery from presentation by returning path/caption records and the next numeric identifier. Both native widgets and engine UI can then share archive validation and naming rules without passing `HWND`, listbox messages, or control identifiers through the data layer.
- Treat save previews as untrusted archive content: validate bounded bitmap headers and palette/pixel ranges before scaling, then remap source palette colors to the active indexed palette. Validate compatibility UI assets through the active archive and show a modal engine error if they are absent; do not silently substitute loose or executable resources.
- A fixed-name resource identity is an owning `RuntimeResourceObject` wrapper, not necessarily the identity accepted by a media renderer. Follow the normal engine acquisition path: lock the resource wrapper, extract and validate its backend, release the wrapper lock, and pass the backend identity to font/bitmap media operations. Resolve compatibility assets by their serialized archive filename when script aliases are not a stable interface.
- GAG's font renderer treats atlas pixels below `0x10` and at or above `0x10` as separate replaceable color layers. Supplying one color for both destroys the authored stroke/shadow distinction and makes `FONT2.RUS` look substantially heavier; preserve a contrasting low layer and main high layer when drawing compatibility text.
- Keep a non-original compatibility UI in its own translation unit, including its model, rendering, input state, resource decoding, lifecycle, and test hooks. Let recovered orchestration call a narrow run/message/activity interface, and expose only small semantic accessors for engine-owned state; this keeps fidelity code readable and prevents compatibility state from leaking into recovered dialog procedures.
- When one compatibility feature should be independently comparable, give it a subordinate build option instead of tying it permanently to the umbrella fixes flag. Test all three meaningful configurations: fidelity build, fixes with the recovered path, and fixes with the replacement path.
- A nested in-engine modal message loop must explicitly unwind on a top-level close request. Mark the modal complete before allowing the normal window shutdown handler to continue; otherwise shutdown state can be set while the modal keeps waiting for unrelated input.

# Use virtual scripts for engine-native compatibility screens

- Model a compatibility screen as an ordinary script root when existing script declarations already cover its images, zones, cursors, and exit behavior. An embedded CFG can introduce no loose assets while retaining normal scene parsing, resource ownership, input dispatch, and `/PEXIT` restoration of the preceding root.
- Virtual resource lookup should return the resource bytes, byte count, and configuration type, and it must run before filesystem or archive probing in both classification and loading. Keeping these results together prevents each caller from reintroducing filename-specific type assumptions.
- When a synchronous script message formerly opened a modal dialog, route the replacement directly to the engine's pending-root transition and return without `ReplyMessage` or a nested command/dialog path. This leaves the script thread free to finish its event and lets the ordinary root-transition loop activate the new screen.
- Dynamic compatibility content does not require a new script grammar when the recovered language already exposes blank scene layers. Declare positioned layers in the embedded CFG, associate them with the owning tree during the normal rebuild lifecycle, and let the feature module fill and clear their pixel buffers. This preserves ordinary tree ownership and teardown while keeping dynamic archive decoding and interaction state outside recovered parser code.
- Help control BMPs such as `HELPEXIT.BMP`, `HELPBACK.BMP`, and `HELPNEXT.BMP` are hover-highlight overlays; the unselected artwork is already part of the Help background. Reuse the recovered `MENU_3S` state template so the overlay resource is stopped in the unselected state, restarted on selection, and removed by restoring the background rectangle when the pointer leaves. Drawing these assets unconditionally produces a permanently highlighted control.
- Reusing a script control entails its ownership hierarchy as well as its template body. The Help hover/reset behavior relies on CFG-owned `MM` state, a root `z_MAIN`, tag sublocations, and their shared `MENU_3S` clear event. Preserve that hierarchy and remap only the final click events when adapting the controls to a compatibility screen.
- Dynamic scene-link `Z` values participate in the display scene index ordering, not a small local layer scale; ordinary media resources allocate from `0x80000`, so compatibility layers should reserve deliberate indices relative to that range. Ordering alone is insufficient: a newly acquired ownerless scene retains the compositor's `0x01000000` inactive bit, and the normal begin-update path only clears it for scenes with registered owners. Explicitly activate compatibility-owned blank layers before drawing them, without changing recovered scene-link activation globally.
- A blank 8-bit scene layer does not carry a source palette like a BMP-backed resource. On a high-color display root this leaves its rectangle compositor callback null even when its pixel buffer is populated and active. Assign the current game palette to a dynamic indexed layer before drawing so the engine builds its indexed-to-destination mapping and installs the appropriate callback.

# Mediate a stock script editor through completion outcomes

- When an existing script text editor already supplies the required keyboard,
  caret, font, and capacity behavior, keep it unchanged and coordinate external
  controls around its normal completion boundary. Record a pending semantic
  outcome, enqueue the editor's ordinary Enter byte, and interpret the completed
  text only after the paused script event resumes. This lets navigation and Exit
  discard input while Save submits it without introducing a second keyboard
  path or duplicating editor state.
- Keep the static caption and editor mutually exclusive in the script. A
  conditionally active name zone can hide the caption before `/INPSTR`, while a
  one-time initialization event can start the same editor for an empty
  collection. Controls that must remain live during editing should send
  controller messages; controls that must be inert should have no zone in that
  mode.
- For archive-backed save names, separate the displayed caption, current full
  key, and physical archive path. Apply truncation only to display text, compare
  full `COMMENT.TXT` keys case-insensitively, resolve duplicate keys from newest
  to oldest, and allocate a physical filename only when no key matches.
- Prefer an inline state check after an action over a free-running conditional
  `/PEXIT` event in a compatibility screen. The inline form makes scene exit
  causally dependent on Save or completed input and prevents initialization-time
  event evaluation from dismissing a newly built screen. When initialization
  must conditionally enter a pausing opcode, use the script language's established
  `GOTO`/`BREAK`/`CSEND` label pattern instead of nesting one switch inside
  another.
- Do not use an editor-state object as a shared mode gate when multiple script
  sections reuse one common control hierarchy. Keep the visual tag shared, but
  let each mode own the click action: an unconditional action can exit directly,
  while the editor-owning mode can route the click through its controller and
  finish a paused editor before exiting.
- A parameterized script `/MESSAGE` is conditional on resolving an active object
  field; if lookup fails (including a legitimate empty string), the engine skips
  sending the message. Do not use that form for a completion callback that must
  always run. Stock `/INPSTR` retains its 32-byte session buffer after completion,
  so an unconditional message can copy that buffer at the immediate boundary and
  preserve both nonempty text and the empty-input case without changing the editor.

# Synthesize archive-backed resources after archive lookup

- Keep generated compatibility assets behind the ordinary archive lookup rather than treating them as virtual resources. Query the registry during type detection without building, and invoke its builder only after an active-CDF miss during loading. A real archive entry then remains authoritative, while successful generated bytes can use the ordinary memory-backed cache, reference counting, and release path.
- When combining indexed images with different palettes, choose one source's complete BMP as the authoritative output container and palette. Map each other source index to the nearest authoritative RGB entry with a deterministic tie rule before copying pixels; directly copying indices would silently reinterpret colors. Treat requested rectangles as top-left screen coordinates and translate their rows when accessing bottom-up BMP storage. Validate dimensions, bit depth, compression, full palette, row stride, pixel bounds, and declared file size before accessing either source.

# Preserve a dynamic indexed layer's authoritative palette

- Do not pre-remap a dynamic indexed image into the scene palette when the
  compositor already accepts a palette per source layer. Keep the original
  indices and install the image's palette on that layer. An indexed display can
  still perform its recovered indexed-to-indexed mapping, while a high-color
  display converts directly from every source palette entry and avoids a lossy
  intermediate palette.
- Convert file palettes explicitly into the engine's channel layout. Windows
  `RGBQUAD` bytes are blue, green, red, reserved, while GAG's internal packed
  palette stores red in the low byte, then green and blue. Reinterpreting the
  file bytes swaps red and blue.
- Keep unrelated dynamic layers independent: a text layer may require the
  active game palette even when an adjacent screenshot layer owns a private
  palette. Palette preparation belongs to the content being rendered, not to a
  generic layer-activation helper.

# Translate legacy palette blocks at typed portability boundaries

- A legacy pointer typed as palette entries may actually address a larger
  `LOGPALETTE`-style block whose header precedes entry zero. Confirm the producer
  and the consumer's first byte access before replacing it with a direct typed
  array; retaining the consumer's historical offset against the new pointer
  silently shifts every color and can read past the array.
- Capturing a display framebuffer into an indexed archive image must use the
  source scene's actual bits per pixel, channel masks, and byte pitch. Copy
  indices directly only for an indexed source. For high-color sources, decode
  each sampled pixel and map it deterministically into the output palette; a
  pointer plus width is not enough to reconstruct row geometry.

# Reconstruct shared indexed backgrounds without inventing pixels

- When aligned 8-bit screens share a visual background but not identical texture
  samples, pixel inequality alone is not evidence that both pixels are damaged.
  Preserve one source as the authoritative BMP container, identify foreground
  through confirmed palette ranges and content geometry, and substitute exact
  pixels from the second source only where the first foreground is masked and
  the second is clean.
- Represent regions obscured in every source with an unused palette entry whose
  color is set to an unmistakable diagnostic red. This retains the original
  indexed format, dimensions, header layout, and recoverable source pixels while
  keeping uncertainty visually explicit.
- Do not promote a shared foreground bounding rectangle or a dilated semantic
  mask directly to pixel-level unknown status. A rectangle contains authentic
  background between and around glyphs, and unconstrained dilation consumes
  additional exposed pixels. Preserve source pixels that are identical across
  inputs and distinguish foreground coverage at the individual-pixel level.
- A strict aligned-image diff can establish only RGB equality or inequality. It
  cannot classify matching pixels as shared foreground, nor determine which side
  of an unequal pair exposes the background. Keep the exact diff artifact
  separate from any semantic background reconstruction and state this limitation.
- When the requested inference rule treats cross-image agreement as authoritative,
  apply exact/near color agreement before semantic foreground classification.
  Shared text or artwork then remains by design, while semantic masks operate
  only on genuinely differing pixels. Use connected bright neutral regions to
  seed grayscale overlays, expand narrowly for dark accents, select the clean
  side for one-sided masks, and reserve red for overlapping differing masks.
- When adding a third indexed reference with materially different authored art,
  restrict recovery to explicit useful regions and express excluded artwork as
  hard masks whose output is byte-compared against the base. Remap accepted
  reference RGB colors to the authoritative output palette while excluding the
  diagnostic-red entry; never copy foreign indices directly across palettes.
- When every aligned source has dense, differently placed foreground and the
  user permits inferred texture instead of diagnostic unknowns, a semantic
  multi-reference reconstruction is more suitable than increasingly elaborate
  indexed-pixel rules. Inventory every foreground class explicitly, preserve the
  structural background invariants, inspect the generated result, then convert
  non-destructively to the exact required dimensions and true-color BMP depth and
  validate the serialized header.
- To prevent any generative alignment drift, treat the generated reconstruction
  only as a fill source and composite through an explicit edit mask over the
  authoritative base. Convert every locked indexed pixel directly through its
  original palette, replace only diagnostic holes and explicitly requested
  foreground glyph masks, and independently assert zero RGB differences outside
  the mask before delivery.
# 2026-08-21 - Reproducible embedded indexed-image patches

- Keep manual embedded bitmap-region payloads paired with a deterministic generator that validates the source BMP dimensions, bit depth, compression, and row orientation. Mirror the generator's half-open rectangle bounds in a focused test so geometry changes cannot silently leave stale expectations.

# 2026-08-21 - Guard clear-before-begin scene redraws at the adapter boundary

- When a recovered renderer clears or resizes a reusable scene before entering its ordinary begin/end update pair, a modern compositor can expose the intermediate blank buffer. Preserve the recovered routine and make the integration adapter hold an outer update only when the replacement fits the current allocation; retain that allocation through the reacquire, release after the nested redraw commits, and explicitly release on acquire or nested-begin failure. Do not hold the guard across a possible reallocating acquire because allocation failure may destroy the guarded scene.

# 2026-08-21 - Keep fixed script-layer indices outside dynamic scene pools

- A scripted layer's `Z` value is also its display-scene lookup index. Fixed private layers must not use values in recovered dynamic allocator bands such as `0x80000`: ownerless acquisition can return an already-existing same-index scene without resizing it, producing state-dependent geometry and content collisions. Diagnose an intermittent blank synthesized layer by comparing the link's requested geometry with the resolved scene geometry before investigating its source bitmap or compositor.

# 2026-08-21 - Match static text to stock INPSTR at the font-renderer boundary

- `/INPSTR` echo parameters four and five are direct foreground and frame palette indices, and its acquired text scene draws at local origin `(0,0)`. A static label intended to transition seamlessly into that editor should initialize the same font state with the same two indices and draw at the same origin; independent RGB remapping or centering produces a visible state change. Derive requested color indices from the authoritative active background palette rather than assuming VGA palette positions.
# 2026-08-22 - Preserve the original main-menu reload notification boundary

- Main-menu availability is a cached script-object state, not a live filesystem
  predicate. In the original flow, successful native Save completion sets
  application flag `0x40000`; `DIALOG.CFG` message 3030 consumes that flag and
  preloads `MENU_RESET`, whose message 3010 synchronizes the saved availability
  flags before messages 1001-1007 repopulate the `MM::*_DIS` fields. Any
  replacement nested dialog that changes menu availability must preserve this
  reload notification boundary.

# 2026-08-22 - Distinguish script text input from keyboard event binding

- The recovered CFG event grammar has no virtual-key condition. Its `/KEYUP`
  token means mouse-button release, not keyboard key-up. The runtime window
  procedure queues `WM_CHAR` bytes for `/INPSTR` and mouse messages for scripted
  interactions, but does not turn `WM_KEYDOWN` into script events. Non-character
  controls such as arrow keys therefore require a host/DLL input mapping even
  when the resulting action is an existing script message.
