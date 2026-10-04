# hUGEDriver 6.1.3

The Game Boy music driver by SuperDisk (Nick Faro) and contributors, from the
`hUGEDriver-6.1.3.zip` release (19,914 bytes, 2024-07-14) at
https://github.com/SuperDisk/hUGEDriver/releases/tag/v6.1.3. Public domain ("hUGETracker
and hUGEDriver are dedicated to the public domain", README of the repository).

- `hUGEDriver.h`: from the zip's `include/`, unchanged.
- `hUGEDriver.lib`: from the zip's `gbdk/`, unchanged: the driver assembled for GBDK 4.x
  (1,947 bytes of code in bank 0, 100 bytes of RAM).

The Makefile links the library and adds this folder to the include path. Songs are
converted by `tools/uge2c.py` (`make music`). How the game uses it: `src/sound/music.c`.
