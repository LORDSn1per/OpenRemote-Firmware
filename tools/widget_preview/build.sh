#!/bin/sh
# Builds LVGL 8.3.11 (the firmware's copy) for the desktop once, then renders
# the widget previews into out/. Objects are cached in .build/.
set -e
cd "$(dirname "$0")"
LVGL=../../remote/.pio/libdeps/openremote_rev5/lvgl
mkdir -p .build out
if [ ! -f .build/liblvgl.a ]; then
  find "$LVGL/src" -name '*.c' | while read -r src; do
    obj=".build/$(echo "$src" | sed 's#[/.]#_#g').o"
    cc -c -O1 -DLV_CONF_INCLUDE_SIMPLE -I. -I"$LVGL" "$src" -o "$obj"
  done
  ar rcs .build/liblvgl.a .build/*.o
fi
c++ -std=c++17 -O1 -DLV_CONF_INCLUDE_SIMPLE -I. -I"$LVGL" -I"$LVGL/src" \
  preview.cpp .build/liblvgl.a -o .build/preview
./.build/preview
ls out
