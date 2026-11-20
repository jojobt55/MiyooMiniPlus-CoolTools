#!/bin/sh
# Homebrew Store launcher for Onion OS
appdir="$(cd "$(dirname "$0")" && pwd)"
cd "$appdir"
export LD_LIBRARY_PATH="/mnt/SDCARD/.tmp_update/lib:/mnt/SDCARD/.tmp_update/lib/parasyte:/mnt/SDCARD/miyoo/lib:/customer/lib:$LD_LIBRARY_PATH"
export PATH="/mnt/SDCARD/.tmp_update/bin:$PATH"
log="$appdir/log.txt"

{
    echo "=== Homebrew Store $(date) ==="
    echo "dir: $appdir"
    uname -a
    for f in bits_per_pixel virtual_size stride; do
        echo "fb0 $f: $(cat /sys/class/graphics/fb0/$f 2>&1)"
    done
    ls -la "$appdir"
} > "$log" 2>&1

bin="$appdir/homebrewstore"
chmod +x "$bin" 2>/dev/null
"$bin" >> "$log" 2>&1
code=$?
# SD card mounted without exec permission: run a copy from /tmp instead
if [ $code -eq 126 ] || [ $code -eq 127 ]; then
    echo "exec failed ($code), retrying from /tmp" >> "$log"
    cp "$bin" /tmp/homebrewstore && chmod +x /tmp/homebrewstore && /tmp/homebrewstore >> "$log" 2>&1
    code=$?
fi
echo "exit code: $code" >> "$log"
sync
