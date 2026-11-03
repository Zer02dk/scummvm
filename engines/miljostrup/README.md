# Miljøstrup engine

A ScummVM engine for *Miljøstrup* (DOS, 1994), a Danish educational
point-and-click adventure about energy, waste and water. All seven locations
are implemented and the game can be played from start to finish.

This engine lives in a personal fork. It was written by an AI agent (Claude)
from reverse-engineering notes, not from decompiled code. Under ScummVM's
`AI-GUIDELINES.md` it can't be submitted to the ScummVM project.

## You need the original game

No game data is included. The engine's tables only hold offsets into the
original files, and it reads images, texts and sounds from your copy at run
time. Put these files from the DOS version together in one directory:

| File | Size |
|---|---|
| `MLJSTRUP.EXE` | 204,368 bytes (detected by MD5) |
| `MLJSTRUP.OVR` | 383,486 bytes (detected by MD5) |
| `MLJSTRUP.ARK` | 3,815,530 bytes |
| `MLJSTRUP.LIC`, `MLJSTRUP.BAS` | only for the "Use the original login" option |

## Building on Windows (MSYS2), the tested way

1. Install [MSYS2](https://www.msys2.org/) and open an **MSYS2 UCRT64** shell.
2. Install the compiler and libraries:
   ```
   pacman -S --needed git make diffutils mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2
   ```
3. Get this branch and configure it with only this engine:
   ```
   git clone --branch miljostrup --single-branch https://github.com/Zer02dk/scummvm.git
   cd scummvm
   ./configure --disable-all-engines --enable-engine=miljostrup --disable-detection-full
   ```
4. Build, then strip the debug information (101 MB down to about 60 MB):
   ```
   make -j$(nproc)
   strip scummvm.exe
   ```
5. Run it, from the same UCRT64 shell so that it finds the SDL2 DLLs. Started
   from Explorer or PowerShell, it silently does nothing.
   ```
   ./scummvm.exe --path=/c/path/to/game miljostrup
   ```
   You can also start `./scummvm.exe` without arguments and add the game in the
   launcher with "Add Game...".

ScummVM must be closed while `make` links, or the link fails.

## Building on Linux or macOS (untested)

This is the usual ScummVM build. Install a C++ compiler, `make` and the SDL2
development package (e.g. `sudo apt install build-essential libsdl2-dev` on
Debian/Ubuntu, `brew install sdl2` on macOS), then do steps 3 and 4 above and
run `./scummvm --path=/path/to/game miljostrup`.

## Options

- **Use the original login** (in the game's options in the launcher, off by
  default): ask for a user name at the start and keep each user's progress in
  status files like the original. When it's off, use ScummVM's save and load
  from the main menu.
- Debug settings, in the game's section of `scummvm.ini`:
  `skip_intro=true`, `fast_walk=true`, `all_items=true`, `start_location=N`
  (1 to 7).
- Debugger console (Ctrl+Alt+D): `room <n>`, `door all open`, `items`,
  `points <n>`.

The engine is marked unstable, so ScummVM shows a warning before the game
starts. You can switch it off with `enable_unsupported_game_warning=false`
under `[scummvm]` in `scummvm.ini`.
