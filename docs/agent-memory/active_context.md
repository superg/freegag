# Active context

- Startup is fully detached from the Windows registry. There are no production `Reg*` calls, registry API seams, installation/version checks, or logical-drive/CD discovery.
- The legacy archive/CD read-speed benchmark and startup speed gate have also been removed, including their API and tests.
- `ApplicationState::installation_path` is initialized to an empty prefix. `Gag01.cdf`, autosaves, and generated `.GSF` paths resolve relative to the process working directory.
- Runtime settings and modern-window geometry persist in `freegag.ini` in the working directory. The file uses `[Game] Settings` and `[Window] Left/Top/Right/Bottom`; invalid or unavailable values fall back nonfatally.
- The optional `gag_resources.rc` build pipeline and `gag_resource_test` were removed. `src/resource_test.cpp` was deleted, and `advapi32` is no longer linked.
- First-party test sources, CTest targets, `GAG_TESTING` code, and explicit `*_for_testing` hooks are intentionally absent during the current decompilation phase. Tests may be reenabled later, but no restoration approach has been selected.
- Do not create, restore, or add tests unless the user explicitly instructs you to do so.
- Scripted in-game save/load is unconditional. `FREEGAG_IN_GAME_SAVE_LOAD` and the recovered native save/load dialog path have been removed; this behavior applies whether `FREEGAG_WINDOWS_FIXES` is enabled or disabled. Screenshot export still uses its separate Win32 save-file dialog.
- Verified Debug builds with MSVC both with and without modern Windows fixes; the resource extractor and repository `check-format` target also pass. Fresh CTest discovery reports zero tests in both configurations.
- No open questions or immediate follow-up work for this task.
