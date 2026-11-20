SERIES BROWSER for Miyoo Mini / Mini Plus (Onion OS)
===================================================

Browse your whole game collection by franchise instead of by console.
Open "Final Fantasy" and see every Final Fantasy game you have, on every
system, in one list. Games launch with your normal Onion emulator settings
(per-game core overrides, play-time tracking and Recents all keep working).

INSTALL
  Copy the SeriesBrowser folder to  SD:/App/SeriesBrowser
  It appears under Apps in Onion.

HOW GAMES ARE FOUND
  Series Browser reads every system in SD:/Emu (the same config.json files
  Onion uses) and scans their Roms folders, including sub-folders.
  Names come from miyoogamelist.xml when present, otherwise from the file
  name ("Legend of Zelda, The (USA)" becomes "The Legend of Zelda").
  Multi-disc games with a .m3u playlist show up once.

  About 150 franchises are recognised automatically, including Japanese
  titles (Rockman, Akumajou Dracula, Seiken Densetsu, Biohazard...) and
  arcade short names (sf2ce, mslug3, kof98...).

CONTROLS
  Series list   A open · Y menu · SELECT sort · L1/R1 tabs · B quit
  Inside one    A play · X remove from series · SELECT sort · Y menu · B back
  Unsorted tab  A play · X add to a series · Y menu
  Add games     A check/uncheck · Y search · L1/R1 system · START save · B cancel
  Keyboard      A type · B delete · X space · SELECT shift · START done

MANAGING SERIES (all on the device, press Y)
  New series...            name it, then tick its games
  Add games...             tick or untick any game on the SD card
  Auto-add by keyword...   e.g. "bomberman": future games join by themselves
  Add to another series    put one game in several series
  Remove (X)               take a game out, even an auto-matched one
  Rename / Hide / Delete   built-in series can be hidden and shown again

  Your changes are saved in series.json in this folder. Back it up if you
  like; deleting it resets everything to the built-in rules.

TROUBLESHOOTING
  Picture upside down?  Create an empty file named "norotate" in this folder.
  New games not showing?  Y > Rescan games.
  Errors are written to log.txt in this folder.

Fonts: Atkinson Hyperlegible and Press Start 2P, SIL Open Font License (res/).
