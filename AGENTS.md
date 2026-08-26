# Repository agent guide

This repository is a decompiled videogame "GAG: The Impotent Mystery" (ГЭГ: Отвязное приключение).

## Core technology

- C++23 and the Win32 API.
- CMake 4.2 or newer.

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

## Agent memory protocol

Read these files before beginning substantial work:

- [Active context](docs/agent-memory/active_context.md)
- [Learned patterns](docs/agent-memory/learned_patterns.md)

Upon completion of every task or plan:

1. Update `docs/agent-memory/active_context.md` to reflect the current project
   state, verified results, open questions, and immediate next steps.
2. Append newly established reusable architectural patterns
   to `docs/agent-memory/learned_patterns.md`.
3. Do not add speculation to learned patterns; keep uncertain observations in
   active context until verified.
