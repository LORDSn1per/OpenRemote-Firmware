# OpenRemote Dock — AI Handoff, 2026-08-22

This is a **separate, new project** inside the OpenRemote ecosystem: firmware for a
"blaster dock" — a second, mains-powered device that pairs with the main remote over
ESP-NOW and does two things the remote's own ESP32-S3 hardware can't:

1. **Transmit** IR and RF433 commands on the remote's behalf (an IR LED, and later a
   433MHz transceiver — see below).
2. **Receive** RF433 signals so users can *learn* new RF remotes (garage doors, RF power
   sockets, doorbells) the same way they already learn IR remotes in WebConfig — the
   main remote has no RF433 receiver, only the dock will.

**Nothing about the main remote firmware or WebConfig needed to change to reach this
point beyond what's already shipped and pushed.** The dock side is what's missing.

## Where things physically live

- `SOFTWARE/Dock/` — this project. Git repo, versioned independently starting at
  **1.00**, +0.01 per bump (explicit user instruction — not tied to the main firmware's
  version numbers, which are at 3.08 as of this handoff).
- `SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/` — the main remote firmware
  (`OpenRemote_1.0.ino`, currently 3.08). This is where the ESP-NOW protocol is
  authoritative — the dock's copy must match it, not the other way round.
- `SOFTWARE/WebConfig/WebConfig 2.47.html` (also mirrored into the firmware repo's
  `webconfig/` folder and pushed to GitHub) — the browser configurator, which already
  has a full RF433 learn UI and ESP-NOW device pairing UI built and working, waiting on
  a real dock to talk to.

## Why this exists — the actual user request chain

The user wanted RF433 devices (garage doors etc.) learnable and controllable the same
way IR devices already are. Working through it live:

1. The remote's ESP32-S3 has no RF433 hardware and no spare pins/design margin to add
   one. **Adding a second, purpose-built device (the dock) was the user's own idea**,
   not something proposed to them — they specifically wanted it to live in the
   remote's charging dock, mains-powered, so it can sit in one room permanently.
2. Communication between remote and dock: **ESP-NOW** (Espressif's connectionless
   peer-to-peer Wi-Fi protocol) — chosen over HTTP/MQTT because it needs no router
   dependency (works if Wi-Fi is down/reconfiguring) and both ends are the user's own
   hardware, so no service discovery is needed. This is already fully built into the
   remote firmware (3.07) and WebConfig (2.41+).
3. Dock hardware: the user picked a **CC1101** module (SPI, 433MHz, ~$3, combined
   TX+RX in one board, coil antenna, ~5m range is enough for their use case) — see the
   AliExpress research earlier in this session. **Not yet purchased/in hand.**
4. Since dock hardware doesn't exist yet, the user is bringing up the *firmware* first
   against a **plain ESP32 dev board with a single LED on a GPIO** standing in for the
   real IR/RF433 transmit hardware, so the whole remote↔dock↔WebConfig flow can be
   exercised end-to-end before committing to specific hardware.

## What's actually in this folder right now (version 1.00)

`Dock.ino` — first bring-up firmware. Read its own top-of-file changelog comment for
full detail; summary:

- Joins the **same Wi-Fi network as the remote** (fill in `WIFI_SSID`/`WIFI_PASSWORD`
  near the top before flashing). This is deliberate, not a shortcut: Espressif's own
  Wi-Fi/BLE coexistence table rates ESP-NOW RX as stable only in STA mode, and joining
  the same AP is what keeps the dock on the same Wi-Fi channel as the remote
  automatically — including if the router changes channel later. Fixed-channel /
  AP-mode approaches were considered and rejected during the design discussion.
- Broadcasts a pairing announce packet every 500ms for the first 30 seconds after boot,
  so the remote's "Search for devices" (available both on the LCD, Settings > Debug >
  ESP-NOW Devices, and in WebConfig's ESP-NOW settings panel) has something to find.
- On receiving a command packet (`EspNowCommandHeader`, magic `ORCM`): blinks the LED.
  RAW-encoded commands bit-bang the actual mark/space timings onto the LED (no carrier —
  just on/off, so it's a rough visual/timing check, not a literal preview of what a
  real IR LED or the CC1101 would output). PARSED commands just get two blinks.
- On receiving an RF433 learn request (`EspNowRfLearnStartPacket`, magic `ORLS`):
  **always replies honestly with `ok:0`** ("nothing captured") after a short delay,
  because there is no RF433 receiver on this dev board. This exists specifically so
  WebConfig's RF learn flow (request → wait → failure shown in the UI) can be exercised
  for real without a receiver, and so the remote's `/api/rf/learn/status` polling
  doesn't just hang for the full 15s timeout every single time.
  **Do not replace this with a fabricated successful capture "for testing."** That
  would be indistinguishable from the dock actually working when it doesn't yet — the
  same "no silent no-ops" discipline that's been enforced everywhere else in this
  project (see the Global Cache bug fixed in firmware 3.06, and the `command.espNow`
  pass-through bug fixed in WebConfig 2.44 — both were exactly this failure class:
  something that looked like it worked but silently did nothing).

## The ESP-NOW wire protocol — where it's defined and the sync risk

The four packet structs and their magic values are defined in
`OpenRemote_1.0.ino` (search for `EspNowAnnouncePacket`, `EspNowCommandHeader`,
`EspNowRfLearnStartPacket`, `EspNowRfLearnResultHeader` — currently around line 3540
and 13462) and **copied byte-for-byte into `Dock.ino`**, not shared via a common
header, because these are two separate PlatformIO projects with no shared include path
today.

**This is the single biggest maintenance risk in this whole feature.** If either side's
struct layout changes — a field added, reordered, or resized — the other side won't
fail to compile. It will silently misparse packets at runtime (the magic number check
is the only thing that catches gross mismatches; a subtler field-order change would
just produce garbage). If asked to touch either copy, always diff both files' struct
definitions before and after.

Longer-term this probably wants a shared header vendored into both projects (copied,
not symlinked, matching this whole project's copy-per-version philosophy) rather than
staying hand-synced indefinitely — not done yet, not asked for.

Magic values, for reference:
- `OREN` (`0x4F52454E`) — pairing announce, dock → remote broadcast
- `ORCM` (`0x4F52434D`) — command to send, remote → dock
- `ORLS` (`0x4F524C53`) — "start RF433 listening", remote → dock
- `ORLR` (`0x4F524C52`) — RF433 capture result, dock → remote

## What is genuinely NOT done

- **No real IR or RF433 transmit/receive hardware.** The LED is a stand-in only.
- **No dock hardware chosen/purchased** — the CC1101 module was picked as the
  intended module but hasn't arrived. `platformio.ini` targets a generic `esp32dev`
  board and will need its `board =` line changed (likely to an ESP32-S3 variant, to
  match the remote's chip family and headroom) once real hardware is chosen.
- **No pairing button** — the dock currently announces itself for 30s on *every* boot,
  which is fine for a bring-up rig sitting on a desk but wrong for a real product (it
  would mean anyone's remote could pair with it on every power cycle). Real hardware
  will want this gated behind a physical button or similar.
- **No persistence** — the dock doesn't remember anything across reboots (no paired
  remote list, no settings). Whether it needs to is an open question: ESP-NOW commands
  arrive from whatever MAC sent them, so the dock may not need to track peers at all
  unless it wants to restrict which remotes it accepts from.
- **Not verified on any hardware.** Compiles clean (`platformio run` succeeds, verified
  this session), nothing has been flashed or tested against a real remote yet.

## House conventions this project follows (same as the rest of OpenRemote)

- **Version bump = update the top-of-file changelog comment (verbose, explain the
  *why* not just the *what*) + bump the version number + commit to git + archive a
  copy of the compiled `.bin` in `BIN/OpenRemote_dock` style, named for its version,
  never overwritten.** This was a real miss earlier in the session for the main
  firmware (3.07 and 3.08 shipped without archived `.bin`s until asked) — see
  `SOFTWARE/FIRMWARE/BIN/` for the main firmware's convention to mirror.
- Git commits are authored as `LORDSn1per <112445119+LORDSn1per@users.noreply.github.com>`
  — the user's real name must never appear in any commit, file, or comment. This was
  the subject of a full git-history rewrite on the main firmware/hardware/Studio repos
  earlier in the project's history; treat it as a hard rule, not a style preference.
- This folder is **not yet pushed to GitHub** — it exists locally only, git-initialized
  but with no remote configured. Ask before adding one / pushing, same as every other
  push in this project has been an explicit, separate request.

## Suggested next steps (not started, just the obvious ones)

1. Get real hardware — a bare ESP32(-S3?) dev board to replace the LED stand-in
   assumption, and eventually the CC1101 module.
2. Flash `Dock.ino` 1.00, flash the *remote* with firmware 3.08 (already built,
   archived at `SOFTWARE/FIRMWARE/BIN/OpenRemote_3.08.bin`), fill in the dock's Wi-Fi
   credentials, and try pairing from the remote's LCD or WebConfig's ESP-NOW panel.
3. Once paired, try sending a test command from an existing IR device with its
   `espNow` transport set — this exercises the LED-blink RAW path.
4. Try WebConfig's "Learn RF433" flow against the dock — it should show the honest
   `ok:0` failure, proving the whole chain works before any real receiver exists.
