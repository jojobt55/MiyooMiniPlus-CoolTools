GENRE BROWSER for Miyoo Mini / Mini Plus (Onion OS)
==================================================

Browse your whole game collection by genre instead of by console. Open
"Role-playing (RPG)" and see every RPG you have, on every system, in one
list. Games launch with your normal Onion emulator settings (per-game core
overrides, play-time tracking and Recents all keep working).

INSTALL
  Copy the GenreBrowser folder to  SD:/App/GenreBrowser
  It appears under Apps in Onion. No Wi-Fi needed.

HOW GENRES ARE FOUND
  Genre Browser reads every system in SD:/Emu (the same config.json files
  Onion uses) and scans their Roms folders, including sub-folders.
  Multi-disc games with a .m3u playlist show up once.

  Each game's genre comes from a list of about 16,600 games (res/genres.tsv,
  the same data as the Game Atlas app). Any regional title works, so
  "Rockman X" is found as Mega Man X and "Seiken Densetsu 3" as Trials of
  Mana. Arcade games are found by their set name (mslug3, sf2ce...).

  Games the list doesn't know, or only calls "Action", are sorted with
  keywords: "soccer" goes to Sports, "mahjong" to Board & cards, "Gradius" to
  Shooter, and so on. A game can be in several genres.

  Genres: Action, Platformer, Shooter, Fighting, Beat 'em up,
  Role-playing (RPG), Adventure, Puzzle, Strategy, Simulation, Racing,
  Sports, Board & cards, Music & rhythm, Pinball, Quiz, Educational,
  Compilations, Other.

TABS (L1 / R1)
  GENRES      every genre that has games, with how many
  MY GENRES   the genres you made
  NO GENRE    games that aren't in any genre yet

CONTROLS
  Genre list    A open · Y menu · SELECT sort · L1/R1 tabs · B quit
  Inside one    A play · X remove from genre · SELECT sort · Y menu · B back
  No genre tab  A play · X add to a genre · Y menu
  Add games     A check/uncheck · Y search · L1/R1 system · START save · B cancel
  Keyboard      A type · B delete · X space · SELECT shift · START done

MANAGING GENRES (all on the device, press Y)
  New genre...             name it ("Favorites", "Co-op", "Metroidvania"...),
                           then tick its games
  Add games...             tick or untick any game on the SD card
  Auto-add by keyword...   e.g. "zelda": future games join by themselves
  Add to another genre     put one game in several genres
  Remove (X)               take a game out, even one the list put there
  Rename / Hide / Delete   built-in genres can be renamed and hidden (and
                           shown again); your own genres can be deleted

  Your changes are saved in genres.json in this folder. Back it up if you
  like; deleting it resets everything to the built-in genres.

TROUBLESHOOTING
  Picture upside down?  Create an empty file named "norotate" in this folder.
  New games not showing?  Y > Rescan games.
  A game in the wrong genre?  Open the genre, select it, X removes it; then
  Y > Add to another genre.
  Errors are written to log.txt in this folder.

CREDITS
  Genre data: No-Intro, Redump and libretro-database names, with genres from
  libretro-database, MobyGames and RAWG (via the Game Atlas app).
  Fonts: Atkinson Hyperlegible and Press Start 2P, SIL Open Font License (res/).
