# Miyoo Mini apps: Homebrew Store, Series Browser, Romhack Store, Game Atlas

Four apps for the **Miyoo Mini and Mini Plus running Onion OS**. They read the same Onion folders and work well
together:

| App | What it does | Download |
| --- | --- | --- |
| **Homebrew Store** | Download free, legal homebrew games for Game Boy, Game Boy Color, GBA and NES | 
| **Series Browser** | Browse the games you already have by franchise instead of by console | 
| **Romhack Store** | Find ROM hacks and fan translations for your games, and patch a copy on the device | 
| **Game Atlas** | Every game ever released for your Miyoo's systems, compared with what's on your SD card 

---

## Installing

1. Download the repo
2. Copy the folders to the `App` folder of your SD card:
   - `SD:/App/HomebrewStore`
   - `SD:/App/SeriesBrowser`
   - `SD:/App/RomhackStore`
   - `SD:/App/GameAtlas`
3. Put the card back in. The apps appear under **Apps** in Onion.

Nothing else is needed: each app has its own fonts and tools, and nothing on your SD card is changed by installing
them.

**Wi-Fi:** Homebrew Store always needs it. Series Browser never needs it. Romhack Store needs it to download patches and screenshots. Game Atlas needs
it for covers and English patches only; the rest works offline.

---

## Controls

Series Browser, Romhack Store and Game Atlas share these controls. The Homebrew Store differs in a few places (see
below).

| Button | Action |
| --- | --- |
| **A** | Open, choose, play |
| **B** | Back (quits from the home screen) |
| **Y** | Menu: search, sort, rescan and other options |
| **X** | The screen's extra action (shown in the bar at the bottom) |
| **SELECT** | Change the sort order |
| **L1 / R1** | Switch tabs on the home screen |
| **Up / Down, Left / Right** | Move one line, or one page at a time |
| **L2 / R2** | Jump through long lists quickly (Romhack Store, Game Atlas) |
| **MENU** | Quit |

The bottom bar always shows what the buttons do on the current screen.

**Homebrew Store differences:** X installs the selected game, Y opens search, L2/R2 switch the type (Games, Demos,
Music, Tools, Everything), START refreshes the catalog. On a game's page, Y removes it, L1/R1 flip screenshots and
L2/R2 go to the previous/next game.

**On-screen keyboard (search):** A types, B deletes, X adds a space, SELECT switches capitals, START searches.

---

## Homebrew Store

Free and legal homebrew games from [Homebrew Hub](https://hh.gbdev.io), the community archive of new games made
for old consoles.

- Covers **Game Boy, Game Boy Color, Game Boy Advance and NES**.
- **Tabs (L1/R1):** one per platform. **L2/R2** switch between Games, Demos, Music, Tools or Everything.
- Each game has screenshots, a description and its author.
- **X installs** the game straight into the right Onion folder (`Roms/GB`, `Roms/GBC`, `Roms/GBA`, `Roms/FC`) with
  box art in `Roms/<system>/Imgs`, so it appears in your game list right away.
- Y on a game's page removes it again.

---

## Series Browser

Your collection, grouped by franchise across every console. Open **Final Fantasy** and see every Final Fantasy game
you own, on every system, in one list.

- Reads the same `Emu/*/config.json` files Onion uses, and scans their `Roms` folders (sub-folders too).
- About 150 franchises are recognised automatically, including Japanese titles (Rockman, Akumajou Dracula, Seiken
  Densetsu, Biohazard…) and arcade set names (sf2ce, mslug3, kof98…).
- Multi-disc games with an `.m3u` playlist show up once.
- Games launch with your normal Onion settings, so per-game cores, play time and Recents keep working.
- **Make your own series on the device** (Y menu): create one and tick its games, auto-add by keyword ("bomberman"),
  put a game in several series, rename, hide or delete. Changes are kept in `series.json` in the app folder.

---

## Romhack Store

About 12,900 ROM hacks and fan translations, matched with the games already on your card.

- **Tabs:** *For my games* (hacks for ROMs you own), *All systems*, *Installed*.
- Each hack has screenshots, a description, author, version, date and download count.
- **Installing never touches your original ROM.** It downloads the patch, lets you pick which ROM to patch (exact
  checksum matches are listed first), checks it, then saves a patched copy next to the original with box art, for
  example `Roms/SFC/Learn 2 Kaizo.sfc`.
- Handles IPS, UPS, BPS, PPF and xdelta patches, and zip, rar and 7z downloads. SNES copier headers are added or
  removed automatically, and PlayStation `.cue/.bin` games are supported.
- Warns you before patching if your ROM is a different version from the one the hack was made for.
- Y > **Check all ROMs** computes checksums once so exact matches show everywhere.

Where the hacks come from:
- **romhacking.net**, the collection preserved on the Internet Archive: about 10,400 hacks released up to September
  2021.
- **[Romhack Plaza](https://romhackplaza.org)**: about 2,500 more, mostly released since 2022, marked *via Romhack
  Plaza*. Patches download straight from Romhack Plaza through its public API, with their permission. The app shares
  one API key, so if you see "too many downloads right now", wait a minute and try again.

---

## Game Atlas

A complete catalog of **28,089 games** for 36 consoles and handhelds, plus **3,003 arcade games** and the **82 Onion
Ports**, compared with your SD card.

- **Tabs:** Systems, Genres, Metascore (90–100, 80–89, 70–79, 60–69, 50–59, 0–49, no score), Decades, Developers
  and English patches.
- Every row shows **games you have / total** with a progress bar. The side panel shows total, installed, missing and
  the percentage.
- **X = Missing only**, on any screen: the counts switch to what you're missing and lists show only missing games.
- Every game has its own page: cover (downloaded when you open it), platform, genre, release date, developer,
  publisher, region, series and Metascore when there is one. A plays it if it's on your SD.
- **Search** finds any regional title, so "Rockman" finds Mega Man and "Seiken Densetsu 3" finds Trials of Mana.
- Regional releases count as one game: owning "Mega Man (USA)" also covers "Rockman (Japan)". Betas, demos,
  prototypes and pirate copies are left out.

**English patches:** 1,132 games (mostly Japan-only) are marked **EN**. If you have the Japanese ROM, press X on the
game's page and then A: Game Atlas patches a copy into English, using the same installer as the Romhack Store.
Y > *Only games with an English patch* filters any list to these games.

**Good to know**
- Metacritic started in 1999, so most GBA, DS and PlayStation games have a Metascore but older systems have almost
  none.
- Some obscure games show "Unknown" for genre, developer or date.
- Homebrew and very rare games often have no cover.

---

## Using them together

- **Homebrew Store** adds new, free games to your consoles. Series Browser and Game Atlas pick them up after a
  rescan. Game Atlas's list is mostly official releases, so most homebrew won't change your completion numbers.
- **Game Atlas** shows what you're missing and which Japanese games can be played in English.
- **Romhack Store** finds hacks for the games you have. Game Atlas also recognises English copies made by the Romhack
  Store (they are named like "Mother 3 (English)"). Each app's *Installed* list only shows the copies it made itself.
- **Series Browser** groups everything, including your new patched copies, by franchise.

After adding games to the card, use **Y > Rescan** in any of the apps.

---

## Files each app keeps in its folder

| File | App | What it is |
| --- | --- | --- |
| `log.txt` | all | Errors, if something goes wrong |
| `series.json` | Series Browser | Your own series and changes |
| `installed.txt` | Romhack Store, Game Atlas | Patched copies you made |
| `hashes.txt` | Romhack Store, Game Atlas | ROM checksums, computed once |
| `cache/` | Homebrew Store, Romhack Store, Game Atlas | Downloads, screenshots and covers. Safe to delete |

---

## Troubleshooting

- **Picture upside down:** create an empty file named `norotate` in the app's folder.
- **New games not showing:** Y > Rescan.
- **"Offline" instead of covers, or downloads fail:** check that Wi-Fi is on in Onion.
- **A patch says it's for a different ROM version:** the hack may still work, but it was made for another
  region or revision. The app tells you which one it expects.
- **Something else:** look at `log.txt` in the app's folder.

---

## Credits

- **Game data:** No-Intro, Redump and the libretro-database project. Clone lists from retool.
- **Game details and scores:** MobyGames, RAWG and Metacritic datasets.
- **Covers:** libretro-thumbnails and the Onion Ports Collection.
- **Homebrew games:** Homebrew Hub by gbdev. Each game keeps its own licence.
- **Hacks and translations:** their authors and translators; romhacking.net, preserved by the Internet Archive;
  and [Romhack Plaza](https://romhackplaza.org), which hosts the newer ones.
- **Bundled software:** 7-Zip (`bin/7zzs`) by Igor Pavlov, GNU LGPL with the unRAR restriction.
- **Fonts:** Atkinson Hyperlegible and Press Start 2P, SIL Open Font License.
- **Onion OS** by the Onion team.

Apart from the free homebrew offered by the Homebrew Store, these apps don't include or download any games or
ROMs. Use them with games you own.
