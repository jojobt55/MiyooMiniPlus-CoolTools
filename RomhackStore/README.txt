ROMHACK STORE for Miyoo Mini / Mini Plus (Onion OS)
===================================================

Browse about 13,300 ROM hacks and fan translations, see which ones fit the
games already on your SD card, and install them on the handheld. Installing
patches a COPY of your ROM: the original file is never changed.

INSTALL
  Copy the RomhackStore folder to  SD:/App/RomhackStore
  It appears under Apps in Onion. Wi-Fi is needed to download patches.

WHERE THE HACKS COME FROM
  1. The romhacking.net hacks and translations collection as preserved on
     the Internet Archive (archive.org item "rhdn-20210914"): about 10,400
     hacks released up to September 2021.
  2. Romhack Plaza (romhackplaza.org): about 2,900 more, most of them
     released since 2022, plus the latest version of about 220 older hacks
     that were updated after the archive was made. Shown as "via Romhack Plaza" on the hack's page.
     Their patches are downloaded straight from Romhack Plaza through its
     public API, used with Romhack Plaza's permission. The app shares one
     API key, so if many people download at once you may see "too many
     downloads right now": wait a minute and try again.
  Covered: NES, Famicom Disk System, SNES, Game Boy / Color, GBA, Mega Drive,
  Master System, Game Gear, Sega CD, 32X, PlayStation, PC Engine (+CD),
  SuperGrafx, MSX, Neo Geo Pocket, WonderSwan, Virtual Boy, SG-1000, Lynx,
  Atari 2600/7800, Pokemon mini, ColecoVision, Nintendo DS.

TABS (L1 / R1)
  For my games   games on your SD card that have hacks or translations
  All systems    everything in the catalog, by system
  Installed      the patched games you made (A play, X delete)

CONTROLS
  A open / details / install     B back      Y menu / search
  SELECT  change sort (popular, newest, A-Z)
  Detail page: X or L1/R1 next picture, Up/Down scroll the text,
               Left/Right previous/next hack
  Keyboard: A type, B delete, X space, SELECT shift, START search

HOW INSTALLING WORKS
  1. The patch is downloaded (zip, rar or 7z; 7-Zip is included in bin/).
  2. You pick which of your ROMs to patch. ROMs whose checksum matches the
     one the hack was made for are marked "exact" and listed first.
  3. If the download has "headered" and "unheadered" versions, the right
     one is picked for your ROM. SNES copier headers are added or removed
     automatically when needed.
  4. The new game is saved next to the original, e.g.
     Roms/SFC/Learn 2 Kaizo.sfc, with the hack's title screen as box art.
     Zipped ROMs are unzipped first. For PlayStation .cue/.bin games a new
     .cue and .bin are written.
  If your ROM is a different version than the patch expects you are warned
  first. BPS and UPS patches refuse a wrong ROM by themselves.

  Patch formats: IPS, UPS, BPS, PPF, xdelta/VCDIFF (including xdelta's
  default lzma compression).

  Y > "Check all ROMs" computes checksums for every ROM once (saved in
  hashes.txt) so "For my games" can show exact matches everywhere.

LIMITS
  - CHD, PBP and CSO disc images can't be patched: keep a .cue/.bin or
    .iso copy of the game you want to patch.
  - Some releases are not simple patches (PC tools, pre-patched files,
    MSU-1 audio packs). The app says so when no patch is found.
  - xdelta patches made with the rare "djw" compression aren't supported.
  - Arcade hacks are not included.

FILES IN THIS FOLDER
  installed.txt  the hacks you installed      hashes.txt  ROM checksums
  cache/         downloaded patches and pictures (safe to delete)
  log.txt        errors, if something goes wrong

CREDITS
  Hack and translation authors, romhacking.net, the Internet Archive,
  and Romhack Plaza (romhackplaza.org), which hosts the newer hacks.
  7-Zip (bin/7zzs) by Igor Pavlov, GNU LGPL + unRAR restriction, see
  bin/7zip-License.txt.
  Fonts: Atkinson Hyperlegible and Press Start 2P, SIL Open Font License.
