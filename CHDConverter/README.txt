CHD CONVERTER for Miyoo Mini / Mini Plus (Onion OS)
==================================================

Turns your .bin/.cue disc images into .chd files, right on the console.
A CHD holds the same game in less space (often 30-60% smaller for
PlayStation games) and is a single file instead of a .cue and several .bin.

INSTALL
  Copy the CHDConverter folder to  SD:/App/CHDConverter
  It appears under Apps in Onion. No Wi-Fi needed.

WHAT IT CONVERTS
  It looks in the Roms folders of every system whose emulator plays CHD
  (PlayStation, Sega CD, PC Engine CD, Neo Geo CD...), including the hidden
  ".Game" folders used for multi-disc games, and lists:
  - each .cue with its .bin files (one or several tracks, audio tracks too),
  - each .bin or .iso that no .cue uses.
  Cartridge systems (Mega Drive .bin, Atari .bin...) are never touched.
  Images that already have a .chd next to them aren't listed.

HOW TO USE
  A        tick / untick a game (everything is ticked at first)
  X        tick all / none
  START    convert the ticked games
  Y        scan the SD card again
  B        quit
  Before starting you choose, with X, whether to DELETE THE .bin/.cue FILES
  afterwards or keep them. The choice is remembered.

  While converting you see the progress of the current disc and of all of
  them, with the time left. B stops: the disc being converted stays as it
  was, the ones already done are kept.

SAFETY
  - Each CHD is read back completely and compared with the original before
    anything is deleted. If the check fails, the CHD is removed and the
    .bin/.cue stay.
  - The originals are only deleted after a successful check, and only if
    you chose to delete them.
  - Playlists (.m3u) that listed a deleted .cue/.bin are updated to list the
    new .chd, so multi-disc games keep working.
  - Onion's game list for the system is refreshed, so the .chd shows up.
  - The new .chd has the same name as the .cue/.bin, so box art, saves and
    save states keep working.
  - It checks that there is enough free space before each disc.

GOOD TO KNOW
  - It takes a few minutes per PlayStation disc (it's a lot of compression
    work for a small handheld) and uses both of its processor cores. Keep
    the console charged; the app asks Onion not to go to sleep meanwhile.
  - The CHD files are the same kind chdman (MAME) makes: they work with the
    PlayStation, Sega CD, PC Engine CD and Neo Geo CD emulators, and with
    Romhack Store / Game Atlas, which can also patch CHD games.

FILES IN THIS FOLDER
  settings.txt   your choice about deleting the originals
  log.txt        what was converted, and errors if any

CREDITS
  libchdr (reading CHD files) by Romain Tisserand and contributors, BSD
  licence, see licenses/libchdr-License.txt. LZMA SDK by Igor Pavlov,
  public domain. miniz (deflate) by Rich Geldreich and contributors, MIT
  licence, see licenses/miniz-License.txt.
  The CHD format is MAME's.
  Fonts: Atkinson Hyperlegible and Press Start 2P, SIL Open Font License (res/).
