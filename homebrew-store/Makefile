# Homebrew Store for Miyoo Mini / Mini Plus (Onion OS)
#
#   make desktop   -> ./homebrewstore-desktop (SDL2 preview window on macOS/Linux)
#   make miyoo     -> build/homebrewstore (static ARMv7 binary, built with zig)
#   make package   -> dist/HomebrewStore/ ready to copy to SD:/App/

COMMON = src/main.c src/gfx.c src/catalog.c src/net.c vendor/cJSON.c
CFLAGS = -O2 -std=gnu11 -Wall -Wno-unused-function

desktop: homebrewstore-desktop

homebrewstore-desktop: $(COMMON) src/plat_sdl.c src/*.h
	cc $(CFLAGS) $(shell sdl2-config --cflags) -o $@ $(COMMON) src/plat_sdl.c $(shell sdl2-config --libs) -lm

miyoo: build/homebrewstore

build/homebrewstore: $(COMMON) src/plat_fb.c src/*.h
	mkdir -p build
	zig cc -target arm-linux-musleabihf -mcpu=cortex_a7 -static -s -O2 -std=gnu11 -Wall -Wno-unused-function \
		-o $@ $(COMMON) src/plat_fb.c -lm
	@file $@ 2>/dev/null || true

icon.png: tools/mkicon.c src/gfx.c
	cc $(CFLAGS) -o build/mkicon tools/mkicon.c src/gfx.c -lm && ./build/mkicon icon.png

package: build/homebrewstore icon.png
	rm -rf dist/HomebrewStore
	mkdir -p dist/HomebrewStore/res dist/HomebrewStore/cache
	cp build/homebrewstore launch.sh config.json icon.png README.txt dist/HomebrewStore/
	cp res/*.ttf res/*OFL.txt dist/HomebrewStore/res/
	cd dist && rm -f HomebrewStore.zip && zip -qr HomebrewStore.zip HomebrewStore
	@echo "Copy dist/HomebrewStore to SD:/App/HomebrewStore"

clean:
	rm -rf build dist homebrewstore-desktop

.PHONY: desktop miyoo package clean
