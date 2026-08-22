# Repository agent guide

This repository reverse-engineers and hosts the Win32 `XTETDLL.DLL` minigame
from `GAG.EXE`.

## Core technology

- C++17 and the Win32 API.
- CMake 3.20 or newer.
- AMD64/x64 is the primary development and testing architecture.
- The original DLL's x86 ABI remains reverse-engineering evidence, not a build-target restriction.
- GDI supplies the compatibility window and 8-bit DIB framebuffer.
- Ghidra plus its MCP bridge is the source of truth for static ABI findings.

## C++ formatting

- The root `.clang-format` file is authoritative for generated and edited C++.
- Use four-space indentation (`IndentWidth: 4`, `TabWidth: 4`) and never insert
  tab characters (`UseTab: Never`).
- Use Allman braces: opening braces belong on a new line for functions, types,
  namespaces, and control statements.
- Do not put a space before an opening parenthesis in function calls,
  declarations, control statements, or overloaded-operator syntax; examples are
  `function(argument)`, `if(condition)`, and `operator()(argument)`.
- Keep pointer and reference markers next to the declarator (`Type *value`,
  `Type &value`). The configured column limit is 200.
- When formatting is needed, run clang-format using the repository file rather
  than manually approximating these conventions.

## Execution rules

- A project README is intentionally not needed at this stage. Do not create,
  expand, or maintain README documentation unless the user requests it later.
- Preserve the repository `.gitattributes` line-ending policy: all project text
  uses LF on every platform.
- Configure with `cmake -S . -B build -A x64`. Use
  `cmake --build build --config Debug` for routine development and verification;
  build Release only when the user requests it or release-specific behavior must
  be checked.
- Use the existing root `build/` directory for all routine Debug
  configuration, builds, and acceptance checks. Do not create alternate build
  directories such as `build-acceptance/` or `build-zlib-acceptance/` unless the
  user explicitly requests an isolated build tree.
- Never change or overwrite binaries under `data/orig/`; treat them as immutable
  reverse-engineering evidence.
- Preserve confirmed facts about the original DLL's 32-bit register ABI in
  reverse-engineering records. Recompiled application interfaces use native-width
  types and the current platform ABI.
- Distinguish confirmed disassembly facts from runtime hypotheses in code,
  documentation, and Ghidra comments.
- Do not guess compatibility data or behavior—including palettes, pixel formats,
  dimensions, structure fields, callback semantics, or state values. Derive them
  from `GAG.EXE`, `XTETDLL.DLL`, their embedded resources, or controlled runtime
  observation. If evidence is not yet available, leave the item unresolved and
  continue tracing the original implementation.
- A diagnostic fallback may be used only to expose or measure behavior. Label it
  explicitly, keep it isolated, and never describe it as the recovered behavior
  or final implementation.
- Keep Ghidra names, prototypes, comments, and structure discoveries synchronized
  with loader-facing declarations when reverse-engineering changes the ABI model.
- Prefer small compatibility shims over copying unrelated parts of the original
  GAG graphics or audio engine.
- Verify source changes with an x64 Debug build by default. Interactive
  rendering or audio changes also require a user/runtime check.
- Do not commit generated `build/` contents as source artifacts.

## Agent memory protocol

Read these files before beginning substantial work:

- [Active context](docs/agent-memory/active_context.md)
- [Learned patterns](docs/agent-memory/learned_patterns.md)

Upon completion of every task or plan:

1. Update `docs/agent-memory/active_context.md` to reflect the current project
   state, verified results, open questions, and immediate next steps.
2. Append newly established reusable architectural or reverse-engineering patterns
   to `docs/agent-memory/learned_patterns.md`.
3. Do not add speculation to learned patterns; keep uncertain observations in
   active context until verified.
