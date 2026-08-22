# Learned patterns

- Portable game paths use an empty installation prefix and retain bare relative archive/file names; do not substitute the executable directory or scan drives.
- Local archive startup must not benchmark throughput or reject/prompt based on the original CD-drive speed threshold.
- Local preferences belong in `freegag.ini` in the process working directory. Resolve its explicit `.\\freegag.ini` name to an absolute path before calling Win32 profile APIs, because bare profile filenames otherwise use Windows-specific search behavior.
- Preference reads and writes are nonfatal. Apply the existing settings mask only after strict numeric parsing, and accept window geometry only when all four coordinates parse, meet minimum dimensions, avoid arithmetic overflow, and intersect a monitor.
- Keep the recovered fixed-slot application message table byte layout unchanged even when legacy registry diagnostics become unreachable.
- During the current high-change decompilation phase, keep the repository free of first-party tests and explicit test hooks. Do not add or restore tests without an explicit user instruction; future reenablement remains intentionally unspecified.
- Game-state save and load always use the scripted in-game `SAVELOAD.CFG` flow. Do not reintroduce a build option or the recovered native dialog path; keep shared archive enumeration and CDF persistence available to the scripted UI and autosave.
