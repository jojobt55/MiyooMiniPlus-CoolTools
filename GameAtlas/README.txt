GAME ATLAS for Miyoo Mini / Mini Plus (Onion OS)
================================================

Every game released for every system your Miyoo runs, compared with what is
on your SD card. See how many games exist, how many you have and how many
you are missing, by system, genre, Metascore, decade and developer, then
browse the missing ones. Covers are downloaded when you look at a game, so
nothing big is stored on the card.

Japanese games that have an English fan translation are marked EN. If you
have the Japanese ROM, Game Atlas can patch a copy of it into English on
the device (your original ROM is never changed).

INSTALL
  Copy the GameAtlas folder to  SD:/App/GameAtlas
  It appears under Apps in Onion. Wi-Fi is needed for covers and patches;
  everything else works offline.

WHAT'S IN THE LIST (28,089 games)
  36 consoles and handhelds: NES, Famicom Disk System, SNES, Game Boy,
  Game Boy Color, GBA, Nintendo DS, Virtual Boy, Pokemon mini, Mega Drive,
  Master System, Game Gear, SG-1000, 32X, Sega CD, PC Engine, PC Engine CD,
  SuperGrafx, Neo Geo Pocket (+Color), Neo Geo CD, WonderSwan (+Color),
  Atari 2600 / 5200 / 7800 / Lynx, ColecoVision, Intellivision, Vectrex,
  Odyssey 2, Channel F, Supervision, MSX, MSX2, PlayStation.
  Plus 3,003 arcade games (FinalBurn Neo) and the 82 Onion Ports.

  One entry per game: the US, European and Japanese releases of a game
  count once ("Rockman" and "Mega Man" are the same game). Betas, demos,
  prototypes and pirate copies are left out.

TABS (L1 / R1)
  Systems          completion for each console
  Genres           Action, Platformer, Role-playing, Shooter...
  Metascore        90-100, 80-89, 70-79, 60-69, 50-59, 0-49, no score
  Decades          1970s to 2020s
  Developers       sorted by number of games
  English patches  Japanese games you can play in English with a fan patch

  Each row shows "you have / total" and a progress bar; the panel on the
  right shows total, installed, missing and the percentage.

CONTROLS
  A        open / details / play
  B        back
  X        MISSING ONLY on/off (works everywhere; the counts switch to
           what you're missing)
  SELECT   sort a list: A-Z, Metascore, newest, oldest
  Y        menu: search, "only games with an English patch", rescan
  L1 / R1  change tab on the home screen
  Left / Right on a game page: previous / next game
  Keyboard: A type, B delete, X space, SELECT shift, START search.
  Search finds any regional name ("Seiken Densetsu 3" finds Trials of Mana).

GAME PAGE
  Cover, platform, genre, release date, developer, publisher, region,
  series and Metascore (when there is one), and whether it's on your SD.
  A plays it. X opens its English patch: A installs it (it downloads the
  patch, lets you pick your ROM, checks it and saves a patched copy next to
  the original, e.g. Roms/SFC/Trials of Mana (English).sfc).

HOW "ON YOUR SD" IS DECIDED
  Files in each Onion Roms folder are matched by name (any region's name,
  ignoring tags like "(USA)" or "[!]") and by checksum for zipped ROMs.
  Arcade games match by set name (mslug.zip), ports by their Onion
  shortcut. Y > Rescan SD card after adding games.

WHERE THE DATA COMES FROM
  Game lists: No-Intro and Redump, through the libretro-database project.
  Genre, developer, publisher, dates: libretro-database, MobyGames, RAWG.
  Metascores: Metacritic (scraped datasets). Metacritic started in 1999, so
  only later games have one: most GBA, DS and PlayStation games, very few
  older ones.
  Covers: libretro-thumbnails (GitHub), and the Onion Ports Collection.
  English patches: the romhacking.net archive on the Internet Archive and
  Romhack Plaza (romhackplaza.org), through its public API.

FILES IN THIS FOLDER
  cache/covers   downloaded covers (safe to delete)
  cache/         downloaded patches and screenshots (safe to delete)
  installed.txt  English copies you made     log.txt  errors, if any

CREDITS
  No-Intro, Redump, libretro-database and libretro-thumbnails contributors,
  MobyGames, RAWG and Metacritic data, retool clone lists, romhacking.net
  translators, the Internet Archive, Romhack Plaza and its authors, and the
  Onion team.
  7-Zip (bin/7zzs) by Igor Pavlov, GNU LGPL + unRAR restriction, see
  bin/7zip-License.txt.
  libchdr (CHD support) by Romain Tisserand and contributors, BSD licence,
  see bin/libchdr-License.txt.
  Fonts: Atkinson Hyperlegible and Press Start 2P, SIL Open Font License.
