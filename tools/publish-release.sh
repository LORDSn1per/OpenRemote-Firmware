#!/usr/bin/env bash
# Publish the current build of every component to the GitHub release, with the
# version in each filename, and rewrite the download links in the READMEs to
# match.
#
# Version numbers in filenames and permanent download links pull against each
# other: GitHub's releases/latest/download/<name> only works if <name> never
# changes. Rather than give up one, this script owns both ends - it names the
# assets with their versions AND updates every README link in the same pass, so
# they cannot drift apart. Run it after any version bump; it is the only thing
# that should edit those links.
set -euo pipefail
cd "$(dirname "$0")/.."
REPO="LORDSn1per/OpenRemote-Firmware"
TAG="latest-builds"

ver_of() { grep -o "$2\"[0-9.]*\"" "$1" | grep -oE '[0-9]+\.[0-9]+'; }
RV=$(ver_of remote/OpenRemote_1.0.ino 'OPENREMOTE_VERSION_STRING ')
DV=$(ver_of dock/firmware/OpenRemote_Dock.ino 'OPENREMOTE_DOCK_VERSION_STRING ')
WV=$(ls webconfig/*.html | grep -oE '[0-9]+\.[0-9]+' | sort -V | tail -1)
SMAC=$(ls -d "studio/Mac/OpenRemote Studio "*.app | grep -oE '[0-9]+\.[0-9]+' | sort -V | tail -1)
SWIN=$(ls "studio/Windows/OpenRemote Studio "*.exe | grep -oE '[0-9]+\.[0-9]+' | sort -V | tail -1)
SLIN=$(ls "studio/Linux/"*.AppImage | grep -oE '[0-9]+\.[0-9]+' | sort -V | tail -1)

A_MAC="OpenRemote-Studio-$SMAC-macOS.zip"
A_WIN="OpenRemote-Studio-$SWIN-Windows.zip"
A_LIN="OpenRemote-Studio-$SLIN-Linux.AppImage"
A_REM="OpenRemote-Remote-Firmware-$RV.bin"
A_DOK="OpenRemote-Dock-Firmware-$DV.bin"
A_WEB="OpenRemote-WebConfig-$WV.html"

echo "remote $RV | dock $DV | webconfig $WV | studio mac $SMAC win $SWIN linux $SLIN"

STAGE=$(mktemp -d); trap 'rm -rf "$STAGE"' EXIT
cp "releases/remote-bin/OpenRemote_$RV.bin"            "$STAGE/$A_REM"
cp "releases/dock-bin/OpenRemote_Dock_$DV.bin"         "$STAGE/$A_DOK"
cp "webconfig/WebConfig $WV.html"                      "$STAGE/$A_WEB"
cp "studio/Linux/OpenRemote Studio $SLIN x86_64.AppImage" "$STAGE/$A_LIN"
# A .app is a bundle: ditto keeps the resource forks that plain zip drops.
ditto -c -k --sequesterRsrc --keepParent "studio/Mac/OpenRemote Studio $SMAC.app" "$STAGE/$A_MAC"
# The Windows .exe is a ~2MB launcher and is useless on its own - it needs the
# app and runtime folders beside it, so the whole folder ships.
WSTAGE="$STAGE/win/OpenRemote-Studio-$SWIN-Windows"; mkdir -p "$WSTAGE"
cp "studio/Windows/OpenRemote Studio $SWIN.exe" "$WSTAGE/"
cp studio/Windows/README.txt "$WSTAGE/" 2>/dev/null || true
cp -R studio/Windows/app studio/Windows/runtime "$WSTAGE/"
(cd "$STAGE/win" && zip -qr "$STAGE/$A_WIN" .)
rm -rf "$STAGE/win"

# Fail loudly rather than shipping a firmware image for the wrong chip.
python3 - "$STAGE/$A_REM" "$STAGE/$A_DOK" "$RV" "$DV" <<'PY'
import sys,struct,re
for f,exp,want in [(sys.argv[1],0x0009,sys.argv[3]),(sys.argv[2],0x0005,sys.argv[4])]:
    d=open(f,'rb').read()
    got=re.search(rb"OPENREMOTE(?:_DOCK)?_(?:FIRMWARE_)?VERSION=([0-9.]+)",d).group(1).decode()
    chip=struct.unpack('<H',d[12:14])[0]
    assert chip==exp, f"{f}: chip 0x{chip:04x}, expected 0x{exp:04x}"
    assert got==want, f"{f}: marker {got}, expected {want}"
    print(f"  verified {f.split('/')[-1]}  v{got}  chip 0x{chip:04x}")
PY

python3 - "$A_MAC" "$A_WIN" "$A_LIN" "$A_REM" "$A_DOK" "$A_WEB" <<'PY'
import sys, re, pathlib
mac,win,lin,rem,dok,web = sys.argv[1:7]
base = "https://github.com/LORDSn1per/OpenRemote-Firmware/releases/latest/download/"
# Each pattern matches the shape of a name, not a fixed version, so the link is
# rewritten no matter which version it currently points at.
pats = [(r"OpenRemote-Studio-[0-9.]+-macOS\.zip", mac),
        (r"OpenRemote-Studio-[0-9.]+-Windows\.zip", win),
        (r"OpenRemote-Studio-[0-9.]+-Linux\.AppImage", lin),
        (r"OpenRemote-Remote-Firmware-[0-9.]+\.bin", rem),
        (r"OpenRemote-Dock-Firmware-[0-9.]+\.bin", dok),
        (r"OpenRemote-WebConfig-[0-9.]+\.html", web)]
for f in ["README.md","studio/README.md","webconfig/README.md","dock/README.md"]:
    p = pathlib.Path(f); s = p.read_text(encoding="utf-8"); o = s
    for pat, new in pats:
        s = re.sub(pat, new, s)
    if s != o:
        p.write_text(s, encoding="utf-8"); print(f"  links updated: {f}")
PY

NOTES=$(mktemp)
cat > "$NOTES" <<EOF
Latest builds of every OpenRemote component. **You do not need to compile anything** - download, then install with Studio or WebConfig.

| Download | Version | What it is |
|---|---|---|
| \`$A_MAC\` | $SMAC | Desktop app - macOS (Intel + Apple Silicon) |
| \`$A_WIN\` | $SWIN | Desktop app - Windows 10/11 (unzip the whole folder, then run the .exe) |
| \`$A_LIN\` | $SLIN | Desktop app - Linux x86_64 |
| \`$A_REM\` | $RV | Remote firmware, ESP32-S3 |
| \`$A_DOK\` | $DV | Dock firmware, ESP32-C3 |
| \`$A_WEB\` | $WV | Browser configurator, installed to the remote's SD card |

### Installing
- **Remote firmware** - Studio > *Recovery* > *Update Firmware* (keeps every setting), or wirelessly from WebConfig.
- **Dock firmware** - two ways, either is fine: wirelessly from WebConfig > *Update the dock wirelessly*, or over USB from Studio > *New Dock*. A dock that has never been flashed needs the Studio route once.
- **WebConfig** - Studio > *Recovery* > *Install WebConfig*, or from WebConfig itself.

Full details in the [README](https://github.com/LORDSn1per/OpenRemote-Firmware#readme).
EOF

TITLE="Latest builds - Studio $SMAC - Remote $RV - Dock $DV - WebConfig $WV"
if gh release view "$TAG" --repo "$REPO" >/dev/null 2>&1; then
  for old in $(gh release view "$TAG" --repo "$REPO" --json assets -q '.assets[].name'); do
    gh release delete-asset "$TAG" "$old" --repo "$REPO" --yes >/dev/null 2>&1 || true
  done
  gh release edit "$TAG" --repo "$REPO" --title "$TITLE" --notes-file "$NOTES" --latest >/dev/null
  gh release upload "$TAG" "$STAGE"/* --repo "$REPO" --clobber >/dev/null
else
  gh release create "$TAG" "$STAGE"/* --repo "$REPO" --title "$TITLE" --notes-file "$NOTES" --latest >/dev/null
fi
rm -f "$NOTES"
echo "published: https://github.com/$REPO/releases/tag/$TAG"
