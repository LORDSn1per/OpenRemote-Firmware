# OpenRemote Handoff

**Date:** 2026-08-21 (supersedes `OPENREMOTE_AI_HANDOFF_2026-08-17b.md`)
**Reason for handoff:** End of a long session covering Studio packaging, Pronto Hex support,
the first public GitHub release, and a full git-history identity rewrite. Nothing is
mid-edit; everything below is committed and pushed.

## Current Versions

| Component | Version | Location |
|---|---|---|
| Firmware | **3.06** | `SOFTWARE/FIRMWARE/BIN/OpenRemote_3.06.bin` |
| WebConfig | **2.40** | `SOFTWARE/WebConfig/WebConfig 2.40.html` |
| Studio (Mac) | **2.67** | `SOFTWARE/OpenRemote Studio/Mac/OpenRemote Studio 2.67.app` |
| Studio (Win) | **2.67** | `SOFTWARE/OpenRemote Studio/Windows/OpenRemote Studio 2.67.exe` |
| Sensor Test | 2026-08-04 (rot 0) | `SOFTWARE/Sensor_Test/BIN/OpenRemote_Sensor_Test_factory_2026-08-04_rot0.bin` |

---

## THE PROJECT IS NOW PUBLIC ON GITHUB

This is the biggest change this session. Two public repos, both genuine forks:

| Repo | Visibility | Fork of |
|---|---|---|
| `LORDSn1per/OpenRemote-Firmware` | **PUBLIC** | `OMOTE-Community/OMOTE-Firmware` |
| `LORDSn1per/OpenRemote-Hardware` | **PUBLIC** | OMOTE hardware repo |

The firmware repo holds all three software pieces: firmware at the repo root,
plus `studio/` and `webconfig/` subfolders.

### Fork status: how it works and how it broke once

GitHub **cannot make a fork of a public repo private** - `docs.github.com` states
"You cannot change the visibility of a fork by itself... a fork's visibility is tied
to the upstream repository's repository network." An earlier attempt this session
created a fork then immediately set it private; GitHub silently **detached it from
the fork network** to honour that (`fork:false, parent:null`). The fix was to create
the fork and leave visibility alone. **If a fork ever needs to be private, it cannot
stay a fork.** Don't re-attempt.

### READMEs

Root, `studio/` and `webconfig/` all have full README pages with hero images in
`images/` (four project-owned JPGs - **no assets reused from OMOTE's repo**, at the
user's explicit instruction). The root README positions OpenRemote against the
discontinued Logitech Harmony (cloud/account dependency, vendor shutdown) and leads
with the no-programming-required workflow as the actual difference from stock OMOTE.

Every feature claim in those READMEs was grep-verified against source before being
written - ATVV Bluetooth voice, Homebridge, NTP, IR learning, Wi-Fi QR setup,
SD backup/restore all confirmed present. Keep that standard if editing them.

---

## GIT HISTORY REWRITE - real name removed

The user's real name was in every pre-2026-08-21 commit as both author and committer
(`Phillip Carlson <phillipcarlson@Phillips-Mac-mini.local>` and a `-MacBook-Pro.local`
variant - the email leaked the name *and* the machine hostname).

**All three local/remote repos were rewritten with `git filter-repo --mailmap`** to
`LORDSn1per <112445119+LORDSn1per@users.noreply.github.com>`:

- **Firmware** - 37 of 43 commits rewritten, force-pushed. Verified: single identity,
  zero matches for "phillip|carlson" anywhere in history, all 43 commits preserved.
- **Studio** - rewritten locally (no GitHub remote). Verified clean.
- **Hardware** - 9 of 81 commits rewritten, force-pushed. Upstream OMOTE authors
  (Max K, Klaus Musch, thehilde, Matt Andreko, Blake) deliberately left untouched.

`git config --global user.name/user.email` are now set correctly, so new commits are
clean automatically. **Use `LORDSn1per` (matching the GitHub username exactly) - not
`lord_sn1per` or `LORD Sn1per`** - the noreply email is what links commits to the
account, but keeping the name consistent matters.

### Safety bundles (delete only when confident)

Full pre-rewrite backups, in the session scratchpad - **these still contain the real
name**, so they are a liability if left lying around, but they are the only rollback:

```
<scratchpad>/git-backups/firmware-before-rewrite.bundle   (3.9 MB)
<scratchpad>/git-backups/studio-before-rewrite.bundle     (50 MB)
<scratchpad>/git-backups/hardware-before-rewrite.bundle
```

Scratchpad is temp storage and may be cleared automatically. If a permanent backup is
wanted, move them somewhere private first.

### ⚠️ UNRESOLVED: old commits may still be reachable via the fork network

**This is the one thing not fully closed.** GitHub's own docs state:

> "Commits can remain accessible in the repository network even after a fork is deleted."
> "Commits pushed to any repository in a network can be accessible from other
> repositories in that network, including the upstream repository."

Force-pushing to a **fork** does not reliably purge the old commits - someone holding
an old SHA could in principle still fetch it through the upstream repo's network.

Practical exposure is **very low**: traffic on the repo was 0 views / 0 clones at the
time of rewrite (verified via the traffic API), so nobody has those SHAs, and a SHA is
not guessable. But it is not zero. Three options if the user wants it fully closed:

1. **Accept it** - realistic given 0 traffic. No action.
2. **Contact GitHub Support** and ask them to purge orphaned/cached commits. Keeps the
   fork badge.
3. **Delete the fork and recreate as a non-fork repo** - guarantees a clean network,
   but loses the "forked from OMOTE" badge the user specifically wanted.

**Not decided. Ask before acting.**

### Also outstanding (name-related)

- `LORDSn1per/OpenRemote-Firmware-backup` (private) and
  `LORDSn1per/OpenRemote-Firmware-standalone-attempt` (private) both **still contain
  the real name** in their history. Both are redundant leftovers from the fork work.
  Deleting them removes the exposure and tidies up - **not done, needs confirmation**
  (irreversible).
- `LORDSn1per/Carlsons-Smart-Clock` is **public and has the surname in the repo name
  itself**. Its commits are clean (`LORDSn1per`), but the repo name is visible. Unrelated
  project, not touched - flag it if the user cares.

---

## Work completed this session

### Studio 2.66 → 2.67

- **2.66: fixed the IRDB build failing with `CERTIFICATE_VERIFY_FAILED`.** A frozen
  PyInstaller build has no CA bundle - Python's default trust store is never populated.
  Vendored `certifi` and added `open_https_url()`, which tries certifi's bundle first
  and **falls back to the OS trust store only on a certificate-verification failure**
  (not on network errors, which retrying can't fix). The two-tier design is deliberate:
  certifi's bundle is frozen at build time and can go stale over years, whereas the OS
  store keeps updating. Verified by actually downloading from the failing
  `codeload.github.com` URL, both as a script and inside the frozen `.app`.
- **2.66: native app window instead of a browser tab.** Added `pywebview`, using the
  OS's own engine (WKWebView on Mac). `show_app_window_or_fallback()` wraps it and
  **falls back to the old browser-tab behaviour if the native backend fails for any
  reason** - deliberate, because Windows couldn't be tested. `serve_forever()` moved to
  a thread (Cocoa requires the main thread); the window's `closed` event now shuts the
  server down immediately instead of waiting out the 15s idle watchdog.
- **2.66: universal2 build.** Every bundled component was already universal2 (checked
  all 20 PyObjC `.so` files); PyInstaller just needed `--target-architecture universal2`.
  Now `x86_64 arm64` - runs on Intel Macs too. **The x86_64 slice has never been
  executed** (no Intel Mac available) - it's built from the same toolchain as the
  verified arm64 slice, but that's inference, not a test.
- **2.67: default window size 1483×860** (was 1280×860), matching the user's preferred size.
- **Build gotchas** (both fixed in the source tree, won't recur): `pip` installed
  `PyObjCTest` and `.dSYM` debug bundles into `vendor/`, and **both break `codesign`**.
  Removed. Build command now also needs `--collect-all webview --collect-all objc
  --exclude-module PyObjCTest` plus PyObjC hidden-imports.
- **Windows packaging.** The `.exe` is a ~1.9 MB **Go launcher**, not PyInstaller - it
  needs the `runtime/` (22 MB Python 3.12) and `app/` folders beside it or it shows
  "The bundled Python runtime is missing". A complete 21 MB zip is at
  `SOFTWARE/OpenRemote Studio/Windows/OpenRemote-Studio-2.67-windows.zip`
  (Mac-only PyObjC libs excluded). **Never ship the bare `.exe` as a release asset.**

### Firmware 3.05 - Bluetooth Sleep switch

Settings > Debug gained a **"Bluetooth Sleep"** toggle, on both menu styles.
**Off (default)** = current behaviour: the BLE link never disconnects, only its cadence
eases (2.95's idle connection profile). **On** = restores the 2.93 mechanism reverted in
2.94: release the BLE session after the Deep Sleep interval so the remote reaches real
light/deep sleep. That trade-off (≈0.5 s Chromecast pause on disconnect, ~3 s cold boot
on wake) was rejected as a *default* but is now an explicit opt-in. Persisted in NVS
(`bleSleep`) and runtime.json, preserved across factory reset.

### Firmware 3.06 + WebConfig 2.39 - Pronto Hex support

New `"pronto"` command type. `loadProntoTimings()` (sibling to `loadRawTimings()`)
decodes into the same `rawTimings`/`frequencyKhz` fields every RAW command already uses,
so nothing downstream knows the difference.

**Wired into both parsers, not one:** `loadRuntimeModel()`'s JSON loop (WebConfig-added
commands) *and* `loadIrDeviceFileIntoRuntime()`'s `.ir` line parser (files Studio copies
over USB). **This was the key design correction** - an earlier plan put the decoder in
WebConfig's JavaScript, which would have silently missed every `.ir` file, because a
`.ir` file's parser is *firmware* code that neither WebConfig nor Studio ever sees.
Decoding once in C++ also avoids two implementations of the same math drifting apart.

Format notes: `word[0]` must be `0x0000` (raw/learned); `0100+` preset-carrier codes are
**rejected outright** rather than mis-decoded. Only the "once" burst section is kept -
held-button repeat already re-sends the same array for every raw command - but the
repeat section is still walked and range-checked so a truncated code is rejected rather
than half-loaded.

**Verified thoroughly:** the C++ decode algorithm was extracted verbatim into a
standalone C program, compiled, run, and cross-checked against an independent Python
computation - exact match. Found and fixed a real inconsistency during that check: the
JS preview used `Math.round` where the firmware uses integer division (floor). Now matched.

### WebConfig 2.40 - Global Caché imports were silently inert

Found while investigating Pronto. `buildRuntimePayload()` only copies a command's `ir`
field into the sync payload **if it is already a `{type,data}` object**. Global Caché
imports produced a flat shape (`raw` as an *array*, no `ir` object), so those commands
reached the remote as empty `{id,name}` shells - **the button appeared and exported
correctly but did nothing when pressed**, apparently since the feature was built.

Both construction sites now also set
`ir:{type:"raw", data:rawMicroseconds.join(" "), frequency}` - note `.join(" ")`, because
`loadRawTimings()` wants a space-separated *string*, not the array. Existing flat fields
left in place.

**Not verified against a real remote sync** - correct at code level, nobody has pressed
a Global Caché button on hardware.

---

## Working Rules (the user's conventions)

1. **Every change bumps the version by 0.01**, adds a verbose root-cause-first changelog
   entry at the top of `OpenRemote_1.0.ino`, **and gets a git commit**.
2. **Save a compiled `.bin`** to `SOFTWARE/FIRMWARE/BIN/OpenRemote_<version>.bin`.
3. **COPY to a new version file, NEVER edit or rename an existing one.** This rule exists
   because an in-place `mv` once permanently destroyed WebConfig 2.26 and 2.27.
   **I violated this twice in one session** - editing `WebConfig 2.38.html` directly, then
   doing the same to `2.39.html` an hour later. Both were caught and corrected (restore
   the original from git, move changes into a new version file with updated
   `openremote-webconfig-version` meta tag and `<title>`), but **watch for it.**
4. **Handoffs** live in `SOFTWARE/AI Handoff/`, named `OPENREMOTE_AI_HANDOFF_<YYYY-MM-DD>.md`
   (append a letter for same-day repeats).
5. **Do not blind-fix.** Measure first. This is what got 876 ms out of boot time safely
   in the previous session, and what found the Global Caché bug in this one.
6. **Only `LORDSn1per` in commits** - never the real name. Global git config is set correctly.

## Build and Install

```bash
cd "/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
~/.platformio/penv/bin/platformio device monitor -p /dev/cu.usbserial-1330   # 460800 baud
```

Mac Studio build (all flags mandatory):

```bash
python3 -m PyInstaller --noconfirm --clean --windowed \
  --name "OpenRemote Studio <ver>" --icon "$APP/OpenRemoteIcon.icns" \
  --target-architecture universal2 \
  --paths "$APP/vendor" \
  --hidden-import esptool --hidden-import serial --hidden-import serial.tools.list_ports \
  --hidden-import configparser --hidden-import argparse --hidden-import shlex \
  --collect-submodules esptool --collect-all webview --collect-all objc \
  --exclude-module PyObjCTest \
  --hidden-import AppKit --hidden-import Foundation --hidden-import WebKit \
  --hidden-import Quartz --hidden-import Security --hidden-import UniformTypeIdentifiers \
  --hidden-import PyObjCTools \
  --add-data "$APP/studio.html:." --add-data "$APP/vendor:vendor" \
  --add-data "$APP/factory:factory" --add-data "$APP/OpenRemote.png:." \
  --add-data "$APP/openremote_product.png:." --add-data "$APP/OpenRemoteIcon.png:." \
  "$APP/openremote_studio.py"
```

Then `plutil` version stamp, `codesign --force --deep --sign -`, `xattr -cr`.

**Verify a Mac build functionally**: run it with `HOME=<tempdir>` (isolates it from a
running instance's lock), read the port from
`$HOME/Library/Application Support/OpenRemote Studio/instance.json`, and POST
`/setup/detect` with a bogus port. A **port** error means esptool loaded; an **import**
error means the build is broken.

Repos: `OpenRemote_1.0` (firmware, git) and `SOFTWARE/OpenRemote Studio` (git, baseline
commit only). `SOFTWARE/WebConfig`, `SD Card Structure`, `Sensor_Test` and
`OpenRemote_2.0` are **not** git repos - copy-per-version only.

## Open Items / Not Verified

1. **Fork-network commit exposure** - see the warning above. Undecided, needs the user.
2. **Two private repos still contain the real name**; `Carlsons-Smart-Clock` is public with
   the surname in its name. Neither touched.
3. **Pronto Hex not verified on hardware.** Code-level verification was thorough (C
   extraction + Python cross-check) but no Pronto code has been transmitted.
4. **Global Caché fix not verified on hardware** either.
5. **Firmware 3.05's Bluetooth Sleep switch not tested on hardware** - both positions.
6. **Studio Windows native window untested** - no `pythonnet` vendored, so Windows
   currently falls back to a browser tab. Needs a real Windows machine.
7. **Studio x86_64 slice never executed** (no Intel Mac).
8. **macOS builds are ad-hoc signed, not notarised** - first launch needs right-click →
   Open. Fixing properly needs a paid Apple Developer account ($99/yr); user chose to
   accept the bypass for now.
9. **Two backlight blinks on power-on** (reported after 3.02, never investigated) - likely
   `lcdPowerOn()`'s `digitalWrite` followed by `initBacklightPwm()`'s `ledcAttachChannel()`
   re-taking the same pin. Cosmetic.
10. **`.ir` file parsing (173 ms) is the remaining boot-speed lever** if that work resumes.
11. **DFS/CPU below 80 MHz during BLE idle is impossible on this build** - Arduino's
    prebuilt libs lack `CONFIG_PM_ENABLE`. Documented, do not re-investigate without an
    ESP-IDF-from-source migration.
12. **Traffic data is only kept 14 days by GitHub** and was 0/0 at time of writing. If
    long-term stats matter, snapshot `gh api repos/.../traffic/views` periodically.

## Constraints

- Preserve the user's SD card data: devices, activities, themes, icons, Wi-Fi, backups.
- Do not erase flash or format the SD card unless explicitly asked.
- The user tests on real hardware. A successful build proves nothing about runtime -
  say so rather than implying verification that didn't happen.
