#!/usr/bin/env bash
#
# Archives the built firmware binaries and mirrors every archived binary to the
# NAS. Run it after every flash.
#
# It exists because doing this by hand was forgotten repeatedly: a version would
# be built, flashed and committed while its binary stayed only in .pio/build,
# where the next build overwrote it. The binaries for 4.44, 4.47, 1.56 and 1.60
# were lost exactly that way and cannot be recovered.
#
# The version is read from the source and then asserted to be present inside the
# archived copy, rather than being guessed from the binary. A marker read out of
# the .bin is only trustworthy once you already know what you are looking for -
# grepping for "the first thing that looks like a version" finds whichever
# string sorts first, which is not the same thing at all.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NAS="/Volumes/home/Documents/Arduino/OpenRemote"

sourceVersion() {                 # sourceVersion <file> <macro>
  grep -o "$2 \"[0-9.]*\"" "$1" | head -1 | sed 's/.*"\(.*\)"/\1/'
}

# Deliberately not "strings | grep -q": grep -q exits at the first match, the
# closed pipe kills strings, and pipefail then reports the whole check as failed
# on a binary that was perfectly fine.
containsVersion() {               # containsVersion <bin> <version>
  local found
  found="$(strings "$1" | grep -xF "$2" || true)"
  [ -n "$found" ]
}

archive() {                       # archive <built> <source> <macro> <dir> <prefix>
  local built="$1" src="$2" macro="$3" dir="$4" prefix="$5"
  [ -f "$built" ] || { echo "  no build at $built - skipped"; return 0; }
  local version; version="$(sourceVersion "$src" "$macro")"
  [ -n "$version" ] || { echo "  !! could not read $macro from $src"; return 1; }
  containsVersion "$built" "$version" \
    || { echo "  !! the build does not contain $version - stale build directory?"; return 1; }

  local dest="$dir/${prefix}${version}.bin"
  if [ -e "$dest" ] && cmp -s "$built" "$dest"; then
    echo "  $prefix$version already archived"
    return 0
  fi
  if [ -e "$dest" ]; then
    # Same version, different bytes: the archived copy was taken before a later
    # rebuild of the same version and is not what is on the hardware. Keep the
    # old bytes rather than destroy them, but make the archive match reality.
    local keep="$dir/${prefix}${version}.superseded-$(date +%H%M%S).bin"
    mv "$dest" "$keep"
    echo "  !! $prefix$version was already archived with DIFFERENT bytes"
    echo "     previous copy kept as $(basename "$keep")"
  fi
  cp "$built" "$dest"
  containsVersion "$dest" "$version" && echo "  archived $prefix$version" \
    || { echo "  !! $dest has no version marker after copying"; return 1; }
}

mirror() {                        # mirror <local dir> <nas dir>
  local from="$1" to="$2" copied=0
  [ -d "$to" ] || { echo "  NAS path missing: $to"; return 0; }
  for file in "$from"/*.bin; do
    [ -e "$file" ] || continue
    local name; name="$(basename "$file")"
    case "$name" in *.superseded-*) continue;; esac
    if [ ! -e "$to/$name" ] || ! cmp -s "$file" "$to/$name"; then
      cp "$file" "$to/$name"
      if cmp -s "$file" "$to/$name"; then echo "  -> NAS $name"; copied=$((copied+1))
      else echo "  !! NAS copy of $name does not match"; fi
    fi
  done
  [ "$copied" -eq 0 ] && echo "  NAS already in sync"
  return 0
}

echo "Archiving builds:"
archive "$ROOT/remote/.pio/build/openremote_rev5/firmware.bin" \
        "$ROOT/remote/OpenRemote_1.0.ino" OPENREMOTE_VERSION_STRING \
        "$ROOT/releases/remote-bin" "OpenRemote_"
archive "$ROOT/dock/firmware/.pio/build/openremote_dock/firmware.bin" \
        "$ROOT/dock/firmware/OpenRemote_Dock.ino" OPENREMOTE_DOCK_VERSION_STRING \
        "$ROOT/releases/dock-bin" "OpenRemote_Dock_"

echo "Mirroring to the NAS:"
mirror "$ROOT/releases/remote-bin" "$NAS/SOFTWARE/FIRMWARE/BIN"
mirror "$ROOT/releases/dock-bin"   "$NAS/DOCK/FIRMWARE/BIN"
echo "Done."
