# OpenRemote project rules

These paths and release rules are authoritative for every task in this repository.

## Source of truth

- All working software source is on the local drive under `/Users/phillipcarlson/Documents/Arduino/OpenRemote`.
- Remote firmware source: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/remote`.
- Dock firmware source: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/dock/firmware`.
- WebConfig source and versioned HTML: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/webconfig`.
- OpenRemote Studio source and local builds: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/studio`.
- Do not treat `/Volumes/home/Documents/Arduino/OpenRemote` as a software source tree. It is the NAS archive/release mirror.

## Hardware

- The current product hardware is OpenRemote Revision 6 and Dock Rev6.
- Hardware design files are under `/Volumes/home/Documents/Arduino/OpenRemote/HARDWARE/OpenRemote-Hardware`.
- Remote PCB: `PCB/Revision 6`.
- Dock PCB: `PCB/Dock Rev6`.
- Do not change any firmware pin mapping unless Phillip explicitly asks for a specific pin change. Do not infer pin changes from schematic review, build-environment names, comments, or older OMOTE references.
- The historical PlatformIO environment name `openremote_rev5` does not authorize renaming the target or changing the working hardware configuration.

## Chromecast media metadata

The working rich-media implementation is split between the dock and remote. Preserve this design when extending media-app support:

- The dock is the only device that connects to the Chromecast, ADB, media catalogues, or artwork servers. The remote must not join Wi-Fi to fetch media metadata or poster art.
- The dock sends title, subtitle, source, playback state, position, duration, and a token-free artwork cache key to the remote over ESP-NOW.
- The dock downloads and decodes artwork into RGB565 at no more than 96x96. Apple TV and ABC remain square; portrait Plex, Stremio, and Prime Video posters preserve their source aspect ratio, so a conventional 2:3 poster is sent as 64x96 rather than centre-cropped or padded.
- The remote caches completed RGB565 images plus their actual width and height under `/media/art` on its SD card using the artwork key. Replaying the same item should load its poster from the remote's SD cache without another download. WebConfig's `Clear Cached Artwork` action may delete only this dedicated directory's files; it must not touch Widget Wallpapers, icons, settings, or source images.
- Keep the dock and remote `EspNowNowPlayingPacket` layouts byte-identical. Titles are 128-byte buffers and the packed packet is 214 bytes as of remote 4.67 and dock 1.81.
- Do not put account tokens, authenticated artwork URLs, Wi-Fi credentials, or other secrets in ESP-NOW packets, serial logs, source code, release notes, or cache filenames.
- Before applying any app-specific fallback, the dock queries Android's active
  `media_session` owner over the approved ADB connection. Treat that package
  and its PLAYING/PAUSED/STOPPED state as authoritative because the Cast
  receiver can continue publishing an old Apple TV session after Apple TV has
  stopped or another native app has taken over. Supported package mappings are
  `com.apple.atve.androidtv.appletv` (Apple TV), `com.plexapp.android` (Plex),
  `com.stremio.one` (Stremio), `org.smarttube.stable` (SmartTube), and
  `com.google.android.youtube.tv` (YouTube), and
  `com.amazon.amazonvideo.livingroom` (Prime Video). ABC iview is the special
  foreground-only mapping `au.net.abc.iview` because it creates no media
  session while playing.
- Keep the last confirmed SmartTube/YouTube Cast record while that Android
  package remains active. This prevents a delayed Apple receiver record from
  replacing the current YouTube title between videos. Clear all app-specific
  rich caches when Android reports an app-owner change, and clear the widget
  when the supported owner reports STOPPED or NONE.
- Normalize media text to the remote's ASCII display-font range on the dock
  before building the ESP-NOW packet. Convert curly apostrophes/quotes,
  bullets, middle dots, long dashes, ellipses and full-width comma/apostrophe
  forms so they never appear as missing-glyph squares on the LCD.
- Android `PlaybackState.position` is a base position at its `updated`
  elapsed-realtime timestamp, not a live counter. SmartTube may leave that
  base unchanged for minutes. Sample `/proc/uptime` in the same ADB shell
  response and advance playing positions by `(uptime - updated) * speed`
  before sending them to the remote, or waking the remote will reset its local
  timer to the old base position.
- The remote requests now-playing once when a visible media widget wakes or
  appears and once after a successful Chromecast BLE keypress. It advances the
  timer locally and does not use timed media polls or a passive receive window.
  Keep ESP-NOW active only while an explicit transaction is in flight, such as
  a metadata request awaiting its reply, an artwork transfer awaiting completion
  and acknowledgement, a relay request, scan, pairing operation, or dock OTA.
  When the transaction completes, send the dock link-down message and stop the
  radio immediately; the dock LED and the remote's blue time-pill outline must
  reflect that same transaction state. Pause, stop and app changes are learned
  on the next wake, request, or Chromecast keypress while the radio is off. The
  dock persists the selected Chromecast target, so the remote sends that target
  on boot/change and does not repeat it periodically.
- Treat that wake/request as one bounded transaction. The dock must rediscover
  its saved Chromecast and force a fresh Android media-owner query before it
  completes an explicit request; it must not answer from a cached boot-time
  empty state. The remote keeps ESP-NOW fully awake while the metadata request
  or artwork transfer is pending, then shuts it down after a valid record plus
  CRC-confirmed poster cache write, an explicit settled-empty result, or the
  bounded transaction timeout. This is not periodic media polling.
- During a metadata or artwork transaction, suppress ordinary dock keepalive
  pings. The artwork frames and acknowledgements already prove the link, while
  a colliding ping can enter channel recovery and move the remote away from an
  active transfer. Artwork ACKs are sent immediately from the ESP-NOW receive
  callback. If a transfer stalls, keep the visible widget's transaction open
  and request the artwork again automatically; do not require another wake or
  button press. Release ESP-NOW only after the poster is CRC-verified and saved,
  or when the media widget is no longer visible and no transfer is active.

The Apple TV app on Google TV/Chromecast does not reliably publish complete Cast metadata. The working fallback is:

1. Identify the active Cast application by the exact display name `Apple TV`.
2. Use the dock's approved, persistent ADB identity to query the Google TV device. Prefer its stable authenticated listener on TCP 5555; discover the TLS pairing port through `_adb-tls-connect._tcp` only when needed.
3. Filter the Apple TV app's `Luna` log for its current `mediaIdentifier=` record and extract the `umc.cmc.*` content ID and whether it is a movie or episode.
4. Resolve that ID through the public Apple TV catalogue page to obtain title, description, duration, and poster URL.
5. Request Apple's compact square `96x96cc-60.jpg` rendition so the remote receives a filled square without black bars.
6. Android retains the approved ADB public key, while the dock firmware carries the matching identity. Do not replace that identity during routine builds because doing so forces the user to approve debugging again.

The Plex Android TV app exposes an active Cast media namespace but commonly leaves the title fields and media-status record empty. The working Plex fallback is:

1. Preserve the Cast application identity `Plex` even when its media-status array is empty.
2. Inspect Plex's full Cast artwork URL before copying it into any fixed-size packet buffer. That URL contains the local Plex Media Server address and an account token even when Plex omits the title.
3. Extract the server address and token privately on the dock and store them in the dock's `plexmeta` Preferences namespace. Never print the token. This persisted connection lets Plex metadata recover after a dock power outage.
4. Query the local Plex Media Server's `/status/sessions` endpoint using the token in an HTTP header. Select the active session whose `<Player address>` matches the chosen Chromecast/Google TV address.
5. Read the title, media type, year or series/episode labels, `ratingKey`, `viewOffset`, duration, player state, and thumb path from that session. Plex durations and offsets are milliseconds and must be converted to seconds before sending them to the remote.
6. Give the remote a short token-free key such as `plex://<chromecast-address>/<ratingKey>`. Keep the authenticated fetch URL only in dock memory.
7. Request the Plex thumb through `/photo/:/transcode` with `width=96`, `height=96`, `minSize=1`, and `upscale=1`. Plex commonly returns a cover-sized portrait in this mode. Preserve that aspect ratio in the RGB565 transfer and let the remote render the narrower image with rounded corners.
8. Plex can queue a short JPEG and its TLS close together. Continue draining already-received plaintext briefly after the connection reports closed; otherwise the ESP32 can see a valid HTTP 200 and content length but read zero body bytes.
9. Keep overlaying the last confirmed Plex record on every intervening Cast poll. Plex's native Cast record is blank, and allowing it through between the five-second server polls makes the widget flicker between rich metadata and `Nothing playing`.
10. Plex can briefly remove the server session while starting, seeking, or changing items. Require two consecutive successful `/status/sessions` responses with no `<Player address>` matching the selected Chromecast before treating it as stopped. Then clear the prior title, timers, artwork key, and private fetch URL. A network failure is not an empty-session confirmation.

The implementation is in `dock/firmware/pio_src/apple_tv_metadata.cpp`, its header, the Cast polling and ESP-NOW artwork service in `dock/firmware/OpenRemote_Dock.ino`, and the media widget/cache receiver in `remote/OpenRemote_1.0.ino`. SmartTube and YouTube often provide text without art; the remote draws its local YouTube logo in that case. Long media titles use a circular scrolling LVGL label, and refresh code must avoid resetting unchanged label text because setting it once per second restarts the marquee.

The Stremio Android TV app publishes usable title, episode and timing data through Cast but commonly omits artwork. The working artwork fallback is:

1. Identify the exact Cast application name `Stremio` and retain its published text and timing.
2. Through the dock's approved ADB connection, filter the `StremioServer` log for its latest `/local-addon/stream/<type>/<content-id>.json` request. A series episode ID resembles `tt14688458%3A3%3A1`; decode `%3A` to `:` and use the leading `tt...` catalogue ID.
3. Use the public compact series/movie poster at `https://images.metahub.space/poster/small/<catalogue-id>/img`. Send a token-free key such as `stremio://series/<catalogue-id>` to the remote so every episode of the same show reuses the cached series poster.
4. The compact Metahub poster is typically 300x450. Decode it at a safe scale, preserve its portrait aspect ratio in the RGB565 transfer, and let the remote draw the resulting 2:3 poster with rounded corners.
5. The Metahub endpoint can return an HTTP 307 redirect to its live image host. The dock artwork fetcher must follow HTTPS redirects before requiring a successful HTTP 200 JPEG response.
6. Transfer and cache it through the same dock-to-remote ESP-NOW artwork path used by Apple TV and Plex.

The ABC iview Android TV app creates no active Android media session and does not publish useful Cast metadata. Its working fallback is:

1. Treat foreground activity package `au.net.abc.iview` as the authority only for ABC. Do not let a stale Cast receiver or abandoned media session replace it.
2. Run this fallback only during an explicit remote now-playing transaction. First dump the accessibility hierarchy without sending any key. Profile, home and detail screens contain accessible TextViews and must immediately produce an empty now-playing result without changing focus or navigating.
3. Only when that first tree is the empty full-screen video surface may the dock send DPAD Down to reveal playback controls. Dump the resulting hierarchy in the same raw ADB shell. `uiautomator` can be silent for several seconds, so emit a harmless one-line heartbeat each second while it runs; a detached process is killed when the raw shell closes. Never send Back: ABC uses it to leave the player. Let the progress overlay fade away on its own.
4. Parse the combined accessibility description for the program and episode, the `Pause`/`Play` action for playback state, and the two `m:ss`/`h:mm:ss` labels for elapsed and total time.
5. Derive ABC's public show slug from the program title and query `https://api.iview.abc.net.au/v3/show/<slug>`. Prefer its portrait image, then request a compact centre-cropped 192x192 baseline JPEG through `wsrv.nl` so the no-PSRAM dock can decode it safely.
6. Send only the token-free `abc://v1/<slug>` identity to the remote. Decode the private fetch URL on the dock to the existing 96x96 RGB565 buffer and use the acknowledged ESP-NOW/CRC transfer and remote SD cache. Episodes of the same show reuse the series artwork.

## Dock firmware transfers (ESP-NOW OTA)

Pushing a dock image over ESP-NOW has broken twice, both times because something
else decided the radio was idle and took it away. These rules are the result of
a dual serial capture of both boards on one clock, and they are load bearing.

- **An ESP-NOW transaction must be declared busy BEFORE the link is raised, not
  after the slow work that follows it.** `dockOtaPrepare()` brings the link up
  and then checksums a 1.4MB image off the SD card, which was measured at
  **1131 ms** (`WebConfig server: handleClient() took 1131ms`). `loop()` runs on
  the other core throughout. Because `dockOtaState` was still `DOCK_OTA_IDLE`
  for that whole window, `serviceEspNow()`'s "nothing is in flight, put the radio
  away" rule released the link that had just been raised for the transfer:

  ```
  ESP-NOW: link up in 1 ms (on the station, channel 8)
  ESP-NOW: stopped
  ESP-NOW: link released (operation complete)
  Dock update: 1477824 bytes, CRC 0x3E0F8E31 -> OpenRemote Dock
  E (220689) ESPNOW: esp now not init!
  ```

  `dockOtaPreparing` is raised for the whole of `dockOtaPrepare()` and folded
  into `dockOtaBusy()` to close this. Any future operation that raises the link
  and then does slow work before arming its own state must do the same. Setting
  the busy flag "once we know it succeeded" is the bug.

- **`dockOtaBusy()` must stay the single answer to "is a transfer in flight",
  and every teardown path must consult it.** It is deliberately false for
  `DOCK_OTA_DONE` and `DOCK_OTA_FAILED`, which are terminal, and true for
  preparation plus `BEGIN`/`DATA`/`END`. Do not reintroduce `!= DOCK_OTA_IDLE`
  tests, and do not add a new sleep, shutdown or radio-release path without
  adding `dockOtaBusy()` to it.

- **`stopNetworkStack()` calls `stopEspNow()` directly and therefore bypasses
  `releaseEspNowLink()`'s `espNowOperationBusy()` guard.** Anything that calls
  `stopNetworkStack()` is responsible for checking `dockOtaBusy()` itself. This
  is why `dockOtaBusy()` appears in `enterLowPowerWait()`,
  `enterBleConnectedIdle()`, `enterDeepPowerSleep()`, `enterDisplaySleep()`'s
  trailing gate, and `loop()`'s three `displaySleeping` re-entry checks. A
  stop-and-wait transfer of 1.4MB in 160 byte chunks takes roughly **107
  seconds**, far longer than any screen timeout, so the display timeout will
  always land inside one.

- **A long WebConfig upload must raise `webConfigTransferActive` for its whole
  duration and release it on completion AND on abort.** The dock firmware upload
  handler did not, so the display timeout could stop the web server and turn
  Wi-Fi off part way through a 1.4MB image.

- **Never retry an ESP-NOW send into a stack that cannot accept it.** The
  transfer used to spend its entire 36 second retry budget on frames the driver
  refused with `esp now not init!`, then report the dock as out of range.
  `serviceDockOta()` re-establishes the link before sending and fails with a
  message naming the real problem if it cannot.

- **The screen may darken during a transfer; only light sleep, BLE-connected
  idle and deep sleep are held off.** Do not "fix" this by pinning the backlight
  on.

- **Never tighten `DOCK_OTA_SLOW_TIMEOUT_MS`.** `esp_ota_begin()` erases the
  dock's whole 0x1E0000 spare slot before it can acknowledge, measured at
  **5.72 s** on a healthy run. At the old 6000 ms that left about 280 ms of
  headroom; 4.70 raised it to **15000 ms**. Going over the edge does not cost
  one clean retry - the retry arrives while the dock is still erasing, the dock
  reads it as a fresh begin, calls `esp_ota_abort()` and erases the whole slot
  again, and the next timeout lands inside that second erase. The original
  3.2 second budget produced exactly this cascade. A longer ceiling costs a
  healthy transfer nothing: the acceptance ack ends the wait as soon as it
  arrives and the dock repeats that ack every second, so an absent dock still
  fails promptly enough. Also do not add work to the dock's `loop()` ahead of
  `serviceOta()`.

- **Diagnose this path with `LOGS/capture-both.command` (or an equivalent dual
  capture) before theorising.** Both failures looked identical from the remote
  side and from WebConfig - "the dock never accepted the transfer... out of
  range rather than busy" - while the dock sat idle and in range the whole time.
  Inspection of the remote's OTA state machine finds nothing, because the state
  machine is correct; the fault is always something outside it taking the radio.
  A healthy run reads:

  ```
  DOCK   Dock: OTA begin frame received (20 bytes)
  DOCK   Dock: firmware transfer starting - 1477824 bytes, version '1.81', into app1
  REMOTE Dock update: ack seq=0 status=0 while in state 1
  REMOTE Dock update: first chunk away, 170 bytes
  REMOTE Dock update: complete, the dock is restarting
  DOCK   Dock: restarting into the new firmware
  DOCK   OPENREMOTE_DOCK_VERSION=1.81
  ```

- **A dock update is only proven by the dock's own reboot banner.** WebConfig
  reaching 100% is not the test; `OPENREMOTE_DOCK_VERSION=` on the dock's serial
  output after `restarting into the new firmware` is.

## Release artifacts

Build only from the local source tree. After every completed version, keep the versioned release artifacts in both the local tree and the exact NAS destination listed below. Never overwrite an older version.

- A local Git save is mandatory after every single version bump to any program or versioned HTML file. Commit the complete source and matching versioned file changes before treating that version as complete or beginning another version bump.

- Remote application BIN:
  - Local: `releases/remote-bin/OpenRemote_<version>.bin`
  - NAS: `/Volumes/home/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_<version>.bin`
- Dock application BIN:
  - Local: `releases/dock-bin/OpenRemote_Dock_<version>.bin`
  - NAS: `/Volumes/home/Documents/Arduino/OpenRemote/DOCK/FIRMWARE/BIN/OpenRemote_Dock_<version>.bin`
- Versioned WebConfig HTML:
  - Local: `webconfig/WebConfig <version>.html`
  - NAS: `/Volumes/home/Documents/Arduino/OpenRemote/SOFTWARE/WebConfig/WebConfig <version>.html`
- Compiled OpenRemote Studio artifacts:
  - Local source, build, and compiled-artifact root: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/studio`.
  - NAS compiled-artifact mirror root: `/Volumes/home/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio`.
  - Preserve the matching `Mac`, `Linux`, or `Windows` platform folder and the compiled artifact's versioned name.

Before reporting a release complete, verify the embedded firmware version marker in each BIN and use SHA-256 to confirm each local/NAS copy is byte-identical to its build output or versioned HTML source. Do not copy versioned WebConfig releases into SD-card template paths unless Phillip explicitly asks for that deployment.

### GitHub `latest-builds` release

A normal Git commit and push updates repository source only. It does **not** update the downloadable files on the GitHub Releases page. Every completed Remote, Dock, WebConfig, or Studio version bump must also update the existing `latest-builds` release at `https://github.com/LORDSn1per/OpenRemote-Firmware/releases/tag/latest-builds` before the release may be reported complete.

- Never push commits, tags, assets, releases, or any other change to the GitHub website unless Phillip explicitly asks for that GitHub push in the current request. Local commits and required local/NAS release packaging do not authorize a GitHub push. If a completed version is waiting for permission, keep its verified artifacts and commits local and report that GitHub publishing remains pending.

1. Build from the local source tree and verify the embedded firmware version marker.
2. Create the prescribed versioned local and NAS artifacts without overwriting any older version, then use SHA-256 and byte comparison to prove that the build output, local artifact, and NAS artifact match. For WebConfig, compare the local versioned HTML directly with its NAS copy.
3. Give GitHub assets these exact public names:
   - `OpenRemote-Remote-Firmware-<version>.bin`
   - `OpenRemote-Dock-Firmware-<version>.bin`
   - `OpenRemote-WebConfig-<version>.html`
   - Preserve the existing platform-specific naming convention for Studio assets.
4. Upload the new assets to the existing `latest-builds` release first. Update its title and version table so every displayed version and filename is current. Update the README's direct download filenames and URLs in the same release commit.
5. Query the release again and verify each uploaded asset's name, byte size, and downloaded SHA-256 against the verified local artifact. Only after that verification may the superseded asset for the same component be deleted. Do not delete unchanged current assets, such as Studio builds when Studio was not bumped.
6. Commit the release documentation changes locally and push `main` to `origin`. Move the mutable `latest-builds` Git tag to that exact new `main` commit and push the tag update so the release's generated source archives and “commits since this release” comparison are current. Confirm local `HEAD`, `origin/main`, and `refs/tags/latest-builds` resolve to the same commit and that the working tree is clean. Finally reopen or query the public release URL and ensure the title, table, links, and asset list all show the new versions.

Never claim that GitHub downloads are updated merely because source was pushed. A release is complete only when the repository, GitHub Release assets, local artifacts, and NAS mirror have all been verified.

## Current implementation history

Keep this section updated as active work progresses so a later session can resume without reconstructing decisions from chat history.

### 2026-09-12 — Remote 4.71 display panel presets and a hidden Debug menu

- **One preset table, three consumers.** `DisplayPanelPreset` /
  `LCD_PANEL_PRESETS[]` in the remote firmware is the single definition of a
  complete display configuration. The LCD Panel dropdown, the Red+Blue rescue
  combo and the factory defaults all read it. Do not re-spell those six values
  anywhere else - that is how a fixed panel clock ends up living in one path and
  not the others.
- **`DISPLAY_FACTORY_DEFAULT` is deliberately not the Adafruit preset.** They
  differ in one field, the LCD driver: a freshly programmed ESP32 starts on
  LovyanGFX, the Red+Blue rescue restores Arduino_GFX. The rescue exists for a
  screen that cannot be read, so it hands back the synchronous driver that works
  on every supported panel; a new board has no such problem and should have the
  faster DMA driver. This is not a copy/paste slip - do not "fix" it.
- **Anything that writes a display configuration must also call
  `persistSettingsToRuntimeConfig()`.** `displayInverted` lives in NVS *and* in
  `settings{}` in `runtime.json`, and `applySettingsJson()` treats the SD copy
  as authoritative at every boot - it reads it, then calls `saveSettings()`,
  which writes what it just read back over NVS. An NVS-only write is therefore
  reverted by the very reboot meant to apply it. This is the 3.15 fault and
  `applyDisplayPanelPreset()` is where it is now handled.
- **The `prefMig1` migration must never force a display default.** It runs on a
  fresh chip's first boot, so anything it forces overwrites the factory default
  on exactly the boards that default exists for. It was forcing Arduino_GFX;
  that line is gone. Remotes that already carry `prefMig1` keep their stored
  choice either way.
- **Settings > Debug is hidden unless `debugMenuVisible` is on**, switched from
  "Debug Menu" on Settings > About under SD Card. Debug holds bring-up controls
  that can leave a remote with an unreadable screen. Because of that, panel
  selection has to stay reachable elsewhere - that is what the Display page's
  LCD Panel dropdown is for. If Debug ever gains another control an ordinary
  user needs, it needs a home outside Debug too.
- **The LCD Panel prompt writes nothing until Reboot is pressed.** This is the
  opposite of the Debug page's dropdowns, which save to Preferences immediately
  and only then ask about rebooting, so Cancel there still leaves the value
  stored. Cancel on LCD Panel must change nothing. It also rebuilds the page
  rather than holding the dropdown object across the modal: settings screens are
  recreated from scratch on every visit, so a stored widget pointer can name
  something LVGL has already freed.
- The two menu styles (`menuStyle` 0 = OpenRemote, 1 = OMOTE) have separate
  renderers for every settings page. A row added to one must be added to the
  other. The OpenRemote style positions rows by literal `y`, so hiding a row
  means closing the gap, not leaving a hole.
- Remote 4.71 builds at 2,682,587 bytes and was flashed over USB with esptool
  hash verification; boot banner confirms 4.71. SHA-256
  `d3e5d2637501b0a2c0c10c13e17c20dbc0a75ee3053da693295b8739d701272e`. Local and
  NAS copies verified byte-identical.
- **Not yet verified on hardware:** the Red+Blue seven-second combo, the About
  switch, the hidden Debug row and the LCD Panel prompt are compile-and-flash
  only. They need someone holding the buttons and reading the screen.
- GitHub publishing remains pending because Phillip did not ask for a GitHub
  push in this request.

### 2026-09-12 — Remote 4.70 begin-phase headroom and WebConfig 2.84 naming

- Remote 4.70 raises `DOCK_OTA_SLOW_TIMEOUT_MS` from 6000 to 15000 ms. The dual
  capture that verified 4.69 measured the dock's `esp_ota_begin()` partition
  erase at 5.72 s against a 6000 ms budget - roughly 280 ms of margin on a run
  that passed. The failure past that edge cascades rather than retrying cleanly:
  the retry lands mid-erase, the dock treats it as a fresh begin, aborts and
  erases the whole slot again, and the next timeout falls inside that second
  erase. Costs a healthy transfer nothing, because the ack ends the wait as soon
  as it arrives.
- WebConfig 2.84 renames the Settings section heading `ESP-NOW` to
  `OpenRemote Dock`, and updates the two in-app instructions that referred to
  that section by name ("Pair an ESP-NOW dock first (Settings > ...)") so they
  still point at something that exists. The `ESP-NOW` toggle inside the section
  keeps its name: it enables the protocol, and its hint explains what ESP-NOW
  is. Diff against 2.83 is exactly five lines and the file grew by exactly the
  24 bytes those edits account for; JavaScript syntax validated.
- Remote 4.70 builds at 2,680,523 bytes, was flashed over USB at `0x10000` with
  esptool hash verification, and its boot banner confirms 4.70 is running.
  SHA-256 `81610ee9974f0d3f89a94c675216d630322d8032550613d7da0b63ff98a36ab3`.
  WebConfig 2.84 SHA-256
  `be0edd0cbc194727e13c4f975fcbfe2a7be840a284a19da695da94f6a1267ef4`. Local and
  NAS copies verified byte-identical.
- WebConfig 2.84 has **not** been installed onto the remote's SD card as
  `/www/index.html`; the renamed heading will not appear until it is.
- Dock firmware is unchanged. Dock 1.81 remains current.
- GitHub publishing remains pending because Phillip did not ask for a GitHub
  push in this request.

### 2026-09-12 — Remote 4.69 makes dock firmware updates work

- Dock firmware updates could never complete. The cause was found on a dual
  serial capture of both boards on one clock, after an inspection-only diagnosis
  had already produced a wrong answer; see the "Dock firmware transfers" rules
  above, which exist so this cannot recur.
- `dockOtaPrepare()` raises the ESP-NOW link and then spends **1131 ms**
  checksumming the 1.4MB image off the SD card before it can set
  `dockOtaState`. `loop()` runs on the other core throughout, read an idle
  `dockOtaState`, concluded no transfer existed, and released the link that had
  just been raised for it. The begin frame went into a deinitialised stack
  (`esp now not init!`), nothing re-raised the link mid-transfer, so all six
  attempts failed identically and the remote reported a dock that was in range
  and idle the whole time as out of range. A `dockOtaPreparing` flag now covers
  the whole of `dockOtaPrepare()` and is folded into `dockOtaBusy()`.
- `serviceDockOta()` re-establishes the link before sending instead of spending
  its full 36 second retry budget on frames the driver refuses, and fails with a
  message naming the real problem if the link cannot be brought up.
- Verified end to end on hardware: begin acknowledged after a 5.72 s partition
  erase, 1,477,824 bytes transferred in roughly 107 s with zero retries, then
  `Dock update: complete, the dock is restarting` followed by the dock's own
  `OPENREMOTE_DOCK_VERSION=1.81` boot banner, re-pairing on channel 8 and
  rejoining Wi-Fi.
- The begin-phase timing margin found here was hardened in 4.70.
- Remote 4.69 builds at 2,680,688 bytes, was flashed over USB at `0x10000` with
  esptool hash verification, and its boot banner confirms 4.69 is running. Local
  and NAS copies are byte-identical with SHA-256
  `7cc7d7fa1b834c37bfda18680103026c208b9fbcb98a5a85f81f8cb1e8a2d3b3`.
- Dock firmware is unchanged. Dock 1.81 remains current; nothing was ever wrong
  on the dock side.
- GitHub publishing remains pending because Phillip did not ask for a GitHub
  push in this request.

### 2026-09-12 — Remote 4.68 sleep guards (necessary, but not the fault)


- 4.68 was diagnosed by inspection and shipped believing this was the cause. It was not: the failure took 36 seconds, long before any screen timeout. The guards below are still correct and still needed - a 1.4MB stop-and-wait transfer runs for roughly 107 seconds and would hit them - but the actual fault is the one recorded under 4.69. The remote's display timeout ran `enterLowPowerWait()` straight through an in-flight transfer, and that calls `stopNetworkStack()`, which calls `stopEspNow()` **directly** instead of going through `releaseEspNowLink()` — so the `espNowOperationBusy()` guard every other teardown path relies on to notice a running transfer was never consulted. A few seconds after "Send to Dock", with nobody touching the remote because there is nothing to touch while it works, the ESP-NOW stack was deinitialised underneath the transfer, Wi-Fi went off and the CPU stopped. Every send after that failed into a dead stack and no acknowledgement could arrive, so the transfer sat at 0% until it burned its retry budget and reported "the dock never accepted the transfer… it is out of range rather than busy". The dock was in range and idle throughout.
- `dockOtaBusy()` now appears in every sleep gate that already holds a WebConfig transfer out: `enterLowPowerWait()`, `enterBleConnectedIdle()`, `enterDeepPowerSleep()`, `enterDisplaySleep()`'s trailing gate, and `loop()`'s three `displaySleeping` re-entry checks.
- The same fault had an upload half. `handleDockFirmwareUploadData()` never raised `webConfigTransferActive`, although `handleRuntimeConfigUploadData()` does so for exactly this reason, so the display timeout could stop the web server and turn Wi-Fi off part way through a 1.4 MB image. It now holds the flag for the duration and releases it on completion or abort, and an aborted upload reports itself rather than leaving a truncated file.
- The screen is still free to darken during either operation; only light sleep, BLE-connected idle and deep sleep are held off, exactly as for a WebConfig transfer.
- Dock firmware is unchanged — nothing was ever wrong on the dock side. Dock 1.81 remains current.
- Remote 4.68 builds at 2,680,331 bytes total image size (78.5% flash, 37.3% RAM), was flashed over USB at `0x10000` with esptool hash verification, and its boot banner confirms `OPENREMOTE_FIRMWARE_VERSION=4.68` is running. Local and NAS copies are byte-identical with SHA-256 `388b6e6170c90117d32ab0a44973e884e649efc94784f4aa8c3e0d9ddcb157c8`.
- GitHub publishing remains pending because Phillip did not ask for a GitHub push in this request.

### 2026-09-12 — Reliable WebConfig dock actions and portrait artwork

- Remote 4.67 tears down ESP-NOW before a WebConfig Wi-Fi mode transition and recreates it for the next explicit transaction. This fixes the stale software-active state that produced intermittent `not in range`, `esp now not init`, and dock-OTA `did not respond` failures after the Wi-Fi driver had already discarded ESP-NOW.
- Dock 1.81 services incoming IR, dock-info replies, OTA state, identify requests, and LED transitions ahead of blocking Cast/ADB/HTTP work. A five-second identify temporarily defers background network work so every 90 ms LED phase remains visible and even.
- WebConfig 2.83 makes the Remote and Dock firmware-version pills active Identify buttons. Dock Identify uses its LED; Remote Identify draws four white/black LCD flashes on top of the existing QR screen, then removes the overlay and restores that exact QR screen. Pair, ADB status, saved-connection deletion, artwork-cache clearing, and both Identify actions execute immediately without requiring Synchronise.
- ADB status uses a prompt-free authenticated shell probe and reports connected/disconnected with a visible coloured indicator. The explicit Pair action may present Android's existing RSA debugging-approval prompt for the dock's permanent identity; passive page-load checks never enter that 30-second approval wait. The C3 implementation does not yet implement Android's separate TLS 1.3/SPAKE2 six-digit pairing-code protocol, so do not claim that the entered code itself has authorized a new identity.
- Remote 4.66 and Dock 1.80 artwork work is included in these final releases: Plex, Stremio, and Prime posters preserve portrait proportions, Prime requests an aspect-preserving `_SX128_` rendition, and versioned `v2`/`v5` cache identities prevent old square or clipped files from being reused. Apple TV and ABC remain square.
- Final builds succeeded and were flashed over USB with esptool verification to Remote Rev6 MAC `a4:cb:8f:e8:8a:d4` and Dock Rev6 MAC `40:4c:ca:f9:ef:54`. Remote total image size is 2,680,131 bytes (78.5% flash, 37.3% RAM); Dock total image size is 1,477,679 bytes (72.6% flash, 22.7% RAM). WebConfig 2.83 passed JavaScript syntax validation and was installed as the remote's 1,840,229-byte `/www/index.html`.
- Verified local and NAS release copies are byte-identical. Remote 4.67 SHA-256 is `ba1d0fe6115658e07939a07731886ab80a0569e60c5f5e27083d1a0ebb1749b1`; Dock 1.81 is `0c95c2301ba6a220d87065956803af1153719fda2d2c0ce358ec103b81f113f1`; WebConfig 2.83 is `f360d1b9d257842292a744aaefa314a24c9c9b99502d2e8a1b9f68467a849b02`.
- Source saves are local commits `1d2aa86`, `eefcf86`, and `fa31c0c`. GitHub publishing remains pending because Phillip did not ask for a GitHub push in this request.

### 2026-09-11 — Selectable sequential IR in Remote 4.65 and WebConfig 2.82

- `Transmit IR from` now has four stable saved values: This remote, Remote and dock, Dock only, and Remote then dock. Existing values 0–2 retain their meanings; the new sequential choice is value 3, so old runtime files, NVS settings, and backups remain compatible.
- Remote and dock again uses the original overlapping best-effort behaviour. Remote then dock emits two complete non-overlapping frames in that order. Both LCD menu styles and WebConfig warn that sequential sends twice and must not be used for toggle commands such as Power.
- Remote 4.65 builds successfully at 2,674,423 bytes total image size (78.3% flash, 37.3% RAM), carries `OPENREMOTE_FIRMWARE_VERSION=4.65`, and was flashed to Remote Rev6 MAC `a4:cb:8f:e8:8a:d4`; esptool verified every image. Dock firmware remains 1.79 because routing is controlled entirely by the remote.
- WebConfig 2.82 passes JavaScript syntax, structural, unique-route-control, option-value, title, and version-marker checks. It was installed over ORUSB as `/www/index.html`; the complete 1,834,361-byte file read back from the remote is byte-identical to the versioned source.
- Remote 4.65 SHA-256 is `f07ea0fb1d538183981f7d250afef393b6e75bf7e80d170774046df1878bd91a`; WebConfig 2.82 SHA-256 is `fa816a1ed8bf4beff1ab9484da44fea850971a5bd169763c56acc61493ebe1c1`. Their local release/source copies and NAS mirrors are byte-identical.
- GitHub publishing remains pending unless Phillip explicitly asks for it. The source commits and release artifacts are local only.

### 2026-09-11 — Held dock IR cadence and collision-free dual IR

- A simultaneous Remote/Dock serial capture reproduced the held-volume pause. The remote delivered 210 ms repeats, but a blocking dock Chromecast pass left its single pending-command slot occupied and discarded four subsequent frames before returning to the IR service.
- Dock 1.79 services pending IR before network work, defers blocking Cast operations while an IR stream is active, and releases the callback staging slot before the RMT transmission so the next held frame can queue safely.
- Remote 4.64 keeps ESP-NOW active throughout a held dock-routed command, eliminating a radio shutdown/restart on every repeat. Its dock-transmit time-pill outline is a solid saturated blue for at least 320 ms, with held repeats extending one continuous indication.
- Exact phase-aligned simultaneous IR from two independent microcontrollers is not possible over ESP-NOW and overlapping 38 kHz emitters can corrupt the received envelope. Remote-and-dock mode now sends two clean, non-overlapping frames in deterministic order: remote first, dock second.
- Remote 4.64 builds at 2,674,331 bytes total image size (78.3% flash, 37.3% RAM) and Dock 1.79 builds at 1,475,335 bytes (72.5% flash, 22.7% RAM). Both were flashed over USB and esptool verified every written image.
- Remote 4.64 SHA-256 is `21a3c93184b61c3641f2fe74739e08c74884ff71b7c914672fd1c834c415772e`; Dock 1.79 SHA-256 is `da38e49558b5fdd5cfd7e62d3aff94245bf7f4612f586f99928b04835447af36`. Each build, local release copy, and NAS copy is byte-identical.
- GitHub publishing remains pending unless Phillip explicitly asks for it. The source commits and release artifacts are local only.

### 2026-09-11 — Command feedback and Widget Wallpaper lifetime fixed in Remote 4.63

- Local command feedback previously armed its 80 ms deadline before the synchronous IR sender ran. A long raw frame could consume that interval before LVGL painted the status pill, leaving only a one-frame red blink. Remote 4.63 starts a full 240 ms visible interval after control returns to the LVGL loop, and held repeats extend it without forcing redundant full-screen redraws.
- When an IR command is routed through both the remote and dock, the status pill now keeps its red local-transmit fill inside a saturated blue dock-transmit outline. The dock pulse no longer brightens to a near-white blue that could be mistaken for the idle white outline.
- The persistent page strip keeps activity and device widgets alive while Settings is rendered. `renderCurrentPage()` nevertheless freed their shared Widget Wallpaper PSRAM descriptors, so returning from Settings exposed image objects backed by freed memory. Cache invalidation now happens only once before `renderAllPageSlots()` rebuilds every dependent page.
- Remote 4.63 builds successfully at 2,674,283 bytes total image size (78.3% flash, 37.3% RAM) and was flashed to Remote Rev6 MAC `a4:cb:8f:e8:8a:d4`; esptool verified every image and the live USB status reports firmware 4.63 with no UART overruns.
- Byte-identical release copies are in local `releases/remote-bin/OpenRemote_4.63.bin` and NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.63.bin`, with SHA-256 `34abc12314fcc4fe6cd928e1cf5825129b2826ec4b10fc18e31a6e5c43fe1333`. The downloaded GitHub `latest-builds` asset was compared byte-for-byte with that artifact before Remote 4.62 was removed from the release.

### 2026-09-11 — GitHub `latest-builds` release corrected

- A source push does not update GitHub Release downloads. The `latest-builds` release had remained on Remote 4.48, Dock 1.61, and WebConfig 2.75 even though newer source had been pushed.
- The release title, version table, README links, downloadable assets, and mutable `latest-builds` tag now publish Remote 4.62, Dock 1.78, WebConfig 2.81, and the unchanged Studio 2.79 builds from the current `main`. The superseded Remote 4.48, Dock 1.61, and WebConfig 2.75 release assets were removed only after the replacements were uploaded, downloaded again, and hash-verified.
- Downloaded GitHub assets match the verified local artifacts: Remote 4.62 SHA-256 `8ee006a30323badc90fd575b3512cb571ad4dfcb14c4e132df4736b0a0f808b0`, Dock 1.78 SHA-256 `f24f318583c66900cdbc1e9406420eb817c56374d2ab3d4cfdbc48254084392b`, and WebConfig 2.81 SHA-256 `30e5e721df74be45cc656abaa26213b3f35ba3804458ba5fc0ddc2930ad5cd19`.

### 2026-09-11 — ABC playback no longer exits in Dock 1.78

- ABC iview treats Back on its playback overlay as leave-player, so Dock 1.77 could still return a running show to its detail page after successfully reading metadata.
- Dock 1.78 removes Back from the ABC metadata path entirely. It reveals controls with DPAD Down only after the key-free preflight confirms a full-screen video, reads the accessible metadata, and lets ABC's progress overlay fade automatically.
- Live regression test under Jane: resumed `GG: Spawn Squad`, captured `S3 Episode 34 Caleb VS Super Smash Bros CPUs!`, position 241 seconds of 891 seconds, and decoded its 96x96 artwork. After the overlay faded, ABC remained in `MainActivity` with protected full-screen video still playing and no Resume/detail controls present.
- Dock 1.78 builds at 1,475,201 bytes total image size (72.5% flash, 22.7% RAM) and was flashed to Dock Rev6 MAC `40:4c:ca:f9:ef:54`; esptool verified every image. Byte-identical release copies are in local `releases/dock-bin/OpenRemote_Dock_1.78.bin` and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.78.bin`, with SHA-256 `f24f318583c66900cdbc1e9406420eb817c56374d2ab3d4cfdbc48254084392b`.

### 2026-09-11 — ABC profile-screen navigation regression fixed in Dock 1.77

- Dock 1.76 sent DPAD Down and then Back whenever ABC was foreground, including while the user was choosing a profile. A media-widget request could therefore exit ABC immediately after Jane or another profile was selected.
- Dock 1.77 performs a key-free accessibility preflight. Any interactive ABC screen returns an empty playback record without sending Down or Back. Only an accessibility-empty full-screen video is allowed to reveal controls, and cleanup sends Back only after an actual Play/Pause control proves that the dock opened the playback overlay.
- Live regression test: the installed dock probe ran on ABC's `Who's Watching?` screen, left `ProfileSwitchActivity` open with Jane still focused, and selecting Jane then entered ABC Big Kids `MainActivity` normally. No profile, home or playback selection was changed by the dock probe.
- Dock 1.77 builds at 1,475,321 bytes total image size (72.5% flash, 22.7% RAM) and was flashed to Dock Rev6 MAC `40:4c:ca:f9:ef:54`; esptool verified every image. Byte-identical release copies are in local `releases/dock-bin/OpenRemote_Dock_1.77.bin` and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.77.bin`, with SHA-256 `7dbf86bc2ab2a9d8ce746e0d02ff9a2d7f1dad6ad042aeb591d1f3a7dc7671ff`.

### 2026-09-11 — ABC iview rich metadata and artwork in Dock 1.76

- ABC iview was confirmed to create no Android media session and to publish no useful Cast record. The dock now recognizes foreground package `au.net.abc.iview` and, only for an explicit media-widget request, reveals iview's controls with DPAD Down and reads the accessibility hierarchy without pausing playback.
- The first raw-ADB implementation launched `uiautomator` in a detached shell, but Android killed that process when the shell closed. The final implementation keeps one bounded shell open and emits a small heartbeat while the hierarchy is generated, then sends Back to hide the overlay.
- Under the active Jane profile, a live test read `Pokemon Horizons: The Series`, `S2 Episode 4 Dot and Nidothing`, playing position 834 seconds, and duration 1,242 seconds. The program continued playing throughout the probe.
- The dock resolves the public ABC show record, requests a compact square series poster, assigns token-free cache key `abc://v1/pokemon-horizons-the-series`, and successfully decoded the live poster to the final 96x96 RGB565 buffer.
- Dock 1.76 builds at 1,474,339 bytes total image size (72.4% flash, 22.7% RAM) and was flashed to Dock Rev6 MAC `40:4c:ca:f9:ef:54`; esptool verified every image. Byte-identical release copies are in local `releases/dock-bin/OpenRemote_Dock_1.76.bin` and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.76.bin`, with SHA-256 `228ed8604324813baf1cdaca5266f45f0c8788e290445eb4c57642fe468214a5`.

### 2026-09-11 — Dock pairing search restored in Remote 4.62

- The `CANNOT SEARCH` regression came from tying dock discovery to the normal Wi-Fi station. Remote 4.61 could start a standalone ESP-NOW scan with Wi-Fi off, but it still rejected every scan while WebConfig's setup access point was active.
- Remote 4.62 keeps the setup access point alive, enables the station interface alongside it for ESP-NOW, listens on the access point's pinned channel, and remembers that channel for later on-demand dock traffic. The dock already sweeps channels 1–13 during pairing, so no saved or associated Wi-Fi network is required.
- Releasing ESP-NOW after a scan no longer mistakes `networkStackActive == false` during setup-AP operation for an unused radio and shuts WebConfig down.
- The firmware builds successfully at 2,674,259 bytes total image size (78.3% flash, 37.3% RAM), carries `OPENREMOTE_FIRMWARE_VERSION=4.62`, and has SHA-256 `8ee006a30323badc90fd575b3512cb571ad4dfcb14c4e132df4736b0a0f808b0`. Byte-identical copies are in local `releases/remote-bin/OpenRemote_4.62.bin` and NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.62.bin`.
- WebConfig 2.81 was preserved unchanged. This correction does not alter any Rev6 pin mapping and does not query, wake, or control the Chromecast.

### 2026-09-11 — Widget Wallpaper content panel and Remote 4.61 release

- Remote 4.61 adds an optional per-Wallpaper rounded content panel behind every widget foreground element. Its colour and transparency are saved in the Wallpaper record and applied consistently to compact and expanded Weather, Battery and Media widgets.
- WebConfig 2.81 adds the matching `contentBoxColour` and `contentBoxTransparency` controls to the Widget Wallpaper editor. It passes JavaScript syntax validation, carries the corrected 2.81 version marker, and ends with a complete `</html>`.
- Remote 4.61 builds at 2,674,139 bytes (78.3% flash, 37.3% RAM), carries `OPENREMOTE_FIRMWARE_VERSION=4.61`, and has SHA-256 `b0a7ec290935240e093170e8f78af1277b6af8bee9d9982c0d30d057d18d2ba5`. Byte-identical copies are in local `releases/remote-bin/OpenRemote_4.61.bin` and NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.61.bin`.
- WebConfig 2.81 has SHA-256 `30e5e721df74be45cc656abaa26213b3f35ba3804458ba5fc0ddc2930ad5cd19`; the versioned source and NAS WebConfig release are byte-identical.
- Dock 1.75 was rebuilt from the current source and re-released unchanged: SHA-256 `090bdf5e86926ec04579a3052ba0ce1a5adcce7a3966a45e973a1b93d85bedda`, byte-identical in local `releases/dock-bin/OpenRemote_Dock_1.75.bin` and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.75.bin`.
- This fix was built and packaged offline. The Chromecast was not connected to, queried, woken, or controlled. Remote 4.61 and WebConfig 2.81 were not installed on the hardware while Phillip's wife was using the Chromecast.

### 2026-09-11 — Widget Wallpaper upload and expanded background fix

- Live WebConfig 2.79 reported `Widget Wallpaper upload failed`. The compact asset uploaded, but a normal generated wallpaper ID plus `_expanded.rgb565` was 52 characters. Remote 4.59 reused the theme filename sanitizer's 48-character limit, clipped the identifying suffix, and then checked the 228x268 upload against the 224x96 compact byte count.
- Remote 4.60 uses a Widget Wallpaper-specific 72-character filename sanitizer, preserving the complete `_expanded.rgb565` suffix and its exact 122,208-byte size validation. A representative 52-character generated name now survives unchanged.
- A full-screen widget set to Follow Page Theme previously inherited the theme's glass transparency. This revealed the compact widget and page underneath, producing the duplicated text and controls visible in Phillip's photo. Remote 4.60 keeps the compact card tied to the page theme but gives the expanded view an opaque blue-grey vertical gradient.
- Remote 4.60 builds successfully at 2,673,952 bytes (78.3% flash, 37.3% RAM), carries `OPENREMOTE_FIRMWARE_VERSION=4.60`, and has SHA-256 `6eef244ed699ba6123f7c2e0d4ee254d7a874f6fba26184b9caa5d7252152373`. Byte-identical copies are in local `releases/remote-bin/OpenRemote_4.60.bin` and NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.60.bin`.
- WebConfig 2.80 retains the Widget Wallpaper editor and carries the corrected version marker. It passes JavaScript syntax and structural checks and has SHA-256 `fb1d53c299c6b09c37112dc5c9c795d94edd0b2d39d855dbce38559f57722217`; the versioned source and NAS WebConfig release are byte-identical.
- This fix was built and packaged offline. The Chromecast was not connected to, queried, woken, or controlled. Remote 4.60 and WebConfig 2.80 were not installed on the hardware while Phillip's wife was using the Chromecast.

### 2026-09-11 — Shared Plex servers and current offline-only work

- Dock 1.75 adds shared/external Plex-server recovery. Plex artwork URLs are now detected without depending on exact spacing in Android logs, and the log buffer is no longer resized in a way that clears the record before it is read.
- The dock privately extracts the Plex server, machine identifier, rating key, and token. When a shared Plex account receives HTTP 401/403 from `/status/sessions`, it queries `/library/metadata/<ratingKey>` for the title, year, duration, thumb, and token-free artwork key while retaining Android's active media session as the authority for playing/paused state and elapsed position.
- Switching between Plex servers is handled by refreshing the private artwork connection while Plex remains active. Account tokens stay in dock memory/preferences and never enter ESP-NOW packets, cache names, or logs.
- The first live shared-server test recovered title, year, duration, and artwork. A follow-up fix now overlays Android's playback state and position even when the Cast source already says `Plex`; this is intended to restore the progressed/remaining timers. Dock 1.75 builds and flashes successfully, but that final timer merge has not yet been live-tested.
- The Chromecast is currently in use by Phillip's wife. Do not connect to it, query it over ADB/Cast, wake it, or send any control until Phillip explicitly says it is available again. Continue only with local source changes and offline builds in the meantime.
- Widget work is complete in WebConfig 2.79 and Remote 4.59. Weather, Battery, and Media settings are collapsed by default; each closed header keeps its preview, name, description, and use count visible. The obsolete `Solid blue-black` choice is removed. Each widget can follow its page theme or select any user-created Widget Wallpaper.
- WebConfig 2.79 adds the Widget Wallpaper library/editor below the three widget cards. A wallpaper uses either a gradient or a photo, has zoom and independent horizontal/vertical crop controls, and can bake optional fine, neon, light-bleed, double-glow, or ember borders into its assets. Saving produces a 224x96 compact RGB565 image and a separately cropped 228x268 expanded RGB565 image, plus a PNG preview and optional source image.
- Remote 4.59 loads those exact SD-backed assets from `/widgets/Wallpapers`, places them behind both compact and full-screen widget content, and falls back to the current page theme if a selected file is unavailable. Its six-slot PSRAM cache can retain all three compact and all three expanded widget backgrounds on one page. Upload/delete APIs, factory reset, native backup, portable browser backup, category restore, and full restore all include the wallpaper records and files. Browser restore reattaches the embedded source and preview PNGs from a firmware-created full backup so both RGB565 crops can be regenerated on a fresh SD card.
- Offline validation completed without contacting or controlling the Chromecast. WebConfig 2.79 passes JavaScript syntax, balanced-tag, duplicate-ID, version-marker, required-control, and asset-dimension checks. Remote 4.59 builds at 2,673,264 bytes (78.3% flash, 37.3% RAM); Dock 1.75 clean-builds at 1,463,472 bytes (71.9% flash, 22.4% RAM). Both embedded version markers were verified.
- Release copies are byte-identical: Remote 4.59 SHA-256 `e8b115472550893c539a0a63b8858da53b8710aac935a50bae802f529b5f4a72`; Dock 1.75 SHA-256 `090bdf5e86926ec04579a3052ba0ce1a5adcce7a3966a45e973a1b93d85bedda`; WebConfig 2.79 SHA-256 `3600075d3114d245305718892b5c3322a692644482086cd19dca7789d737dca5`. The BINs are in the prescribed local release folders and NAS firmware folders, and WebConfig 2.79 is in its local source folder and NAS WebConfig folder.
- Do not flash Remote 4.59 or reinstall WebConfig 2.79 while the Chromecast is unavailable for testing. A remote reboot could initiate normal media traffic. Live checks still required after Phillip says the Chromecast is free: confirm the shared-Plex progressed/remaining timers, create and sync one gradient and one photo Widget Wallpaper, and inspect compact/expanded rendering and backup/restore on hardware.

### 2026-09-10 — WebConfig service setup and widget appearance

- Started WebConfig 2.77 from the released 2.76 source.
- Requested Settings-page layout: add a collapsed `Home Bridge` disclosure containing all Homebridge connection settings.
- Device wizard rule: Home Assistant, Homebridge, and MQTT must discover using credentials already saved through their Settings APIs. Their wizard pages must never ask for connection credentials directly.
- Each of those three wizard pages must always show a `Settings` button as well as `Connect & Discover`. `Settings` opens a scrollable in-wizard editor with the same controls and save behavior as the main Settings page; saving returns to the service's device wizard and the saved connection becomes the source for discovery.
- Widget request: every widget type needs a saved choice between following the page theme and using an opaque modern blue-black card background. This must affect the WebConfig previews and the actual remote rendering.
- Media widget settings still require visible Chromecast ADB setup instructions, a six-digit pairing-code field/action, and a way to delete a saved Chromecast connection.
- WebConfig 2.77 work completed so far: removed HA and Homebridge credentials from their wizard panels; added permanent `Connect & Discover` and `Settings` buttons to HA, Homebridge, and MQTT; added a scrollable shared service-settings modal; added the collapsed main Settings `Home Bridge` card; and made MQTT reveal its device/topic editor only after validating the saved broker.
- Remote 4.54 work completed so far: added `/api/homebridge/config`, `/api/mqtt/connect`, and persistent Homebridge relay-choice handling so the new editors save to NVS and discovery can use an empty request body to reuse saved credentials.
- Added `background: "theme" | "solid"` to each Weather, Battery, and Media widget settings object. WebConfig previews show the selected appearance. Remote 4.54 parses each option and styles both the compact card and expanded overlay; `solid` is opaque blue-black and `theme` uses the active theme's glass colour/opacity.
- Added the visible ADB instructions, pairing-address field, six-digit code field, Pair Dock button, status text, and Delete Saved Chromecast Connection button to the Media widget card. Backend binding and validation remain in progress.
- Remote 4.54 now treats delayed artwork as a loading layout rather than a missing image: the poster frame and placeholder glyph stay hidden, title/subtitle expand into the open space in both compact and full-screen widgets, and the artwork layout returns automatically when the cached RGB565 image becomes available.
- Media widget follow-up: removed the optional `Fetch through the dock` control and made dock-backed metadata mandatory in WebConfig and remote parsing. With artwork disabled, the same larger text-only layout is used and no missing-art glyph is created. SmartTube/YouTube retain the local YouTube logo when artwork display is enabled. Implemented the previously inert `Expand when playback starts` option: a newly playing title now opens a visible compact Media widget once, and stopping playback rearms the same title for a later session.
- Release validation completed: WebConfig 2.77 has one syntax-valid inline script, no duplicate HTML IDs, all three permanent service-editor buttons, all three widget background selectors, and the ADB address/PIN controls. Browser visual automation was unavailable, so validation used structural checks plus the remote's own USB installer.
- Remote 4.54 built successfully (2,666,288-byte application image), carried `OPENREMOTE_FIRMWARE_VERSION=4.54`, and was flashed to the Rev6 remote at MAC `a4:cb:8f:e8:8a:d4`; esptool verified the written image. WebConfig 2.77 was then installed over the remote's ORUSB protocol as `/www/index.html`; the remote reported firmware 4.54 and an installed HTML size of 1,784,809 bytes.
- Release copies were created without overwriting older versions: `releases/remote-bin/OpenRemote_4.54.bin`, NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.54.bin`, local `webconfig/WebConfig 2.77.html`, and NAS `SOFTWARE/WebConfig/WebConfig 2.77.html`. Remote BIN SHA-256 is `6bfacb5ea620bd5f0b19608b50c2f55af421ec475866f9a56b6d265f0b3bd018`; WebConfig SHA-256 is `9dcba295aad434bcbbd87af6741a3de7241a661dd62e7e62465cc938955bd62b`.

### 2026-09-10 — Bluetooth voice-control wording

- The Bluetooth streamer wizard still claimed that microphones and assistant buttons were unavailable, although remote firmware now implements live microphone capture and Google Android TV Voice-over-GATT (ATVV) Voice Search. Preparing WebConfig 2.78 to describe the supported voice path and its hold-to-talk interaction accurately while retaining the separate IR power guidance.
- WebConfig 2.78 now calls the profile `Bluetooth remote with Voice Search`, lists Voice Search among its capabilities, says that live audio comes from OpenRemote's built-in microphone over Android TV Voice-over-GATT, and tells the user to hold the assigned Voice Search button while speaking. It retains a compatibility qualification for ATVV-capable streamers and leaves the Apple TV IR/Siri limitation unchanged because that is a separate protocol.
- WebConfig 2.78 passed JavaScript syntax and duplicate-ID checks, was installed over USB as `/www/index.html` on remote firmware 4.54, and the remote reported the expected 1,785,124-byte installed file. The local and NAS copies are byte-identical with SHA-256 `117ee4e58aaaef663f962a8f599ab3a1a4acac66d4c95d26d0ccb6618b938298`.

### 2026-09-10 — Prime Video media metadata

- Live diagnosis began while the 2026 movie `Goat` was playing in the native Prime Video app on the paired Google TV at `192.168.1.124:5555`.
- Android reports the authoritative active media-session owner as `com.amazon.amazonvideo.livingroom`. Dock 1.71 does not map that package, so its authority filter rejects the Prime session and clears the widget.
- Prime publishes reliable `PLAYING` state, elapsed position and update timestamp through `dumpsys media_session`, but its standard metadata contains only the generic description `PrimeVideo`; title, duration and artwork are blank. The implementation therefore needs a Prime-specific dock-side fallback for identity, duration and artwork while retaining Android's state/clock as authoritative.
- Prime's raw Android media metadata was decoded from its hidden session bundle. `android.media.metadata.MEDIA_ID` and Amazon's Alexa playback-source field both carry the current Global Title Identifier (GTI); for `GOAT` the live value is `amzn1.dv.gti.5ba8df96-80f1-49bf-af00-f861d23eb08a`. Prime deliberately reports `android.media.metadata.DURATION=-1` and leaves display title blank.
- A direct unauthenticated request to `https://www.primevideo.com/detail/<full-gti>` resolves the active GTI. The page exposes title, synopsis, release year, exact duration in seconds, and a public `og:image` packshot. Amazon's image modifier `._SS192_.jpg` produces a small square JPEG that can use the existing centre-crop/RGB565/ESP-NOW artwork path without black bars.
- The stock Android `dumpsys` and `cmd media_session` tools hide the raw media bundle. A compact read-only Java helper was proven through `app_process`: it queries Android's media-session Binder service, selects `com.amazon.amazonvideo.livingroom`, and prints only its current GTI. Dock firmware will carry a versioned 1.8 KB helper JAR, create it under `/data/local/tmp` over the already-approved ADB shell when missing, and reuse that cached file across polls and TV reboots.
- Dock 1.72 implements this Prime-specific path in `dock/firmware/pio_src/apple_tv_metadata.cpp`, `apple_tv_metadata.h`, and the generated `openremote_prime_media_helper.h`. It maps the package to `Prime Video`, treats Android's playback owner/state/position as authoritative, resolves a changed GTI through the public Prime catalogue, and keeps the public artwork fetch URL private on the dock. The remote receives only the existing 214-byte now-playing packet with a token-free `prime://<gti>` cache identity, so no ESP-NOW packet or remote code change was required.
- The first live 1.72 test found `GOAT` and its current position but returned a zero duration because scanning the 300+ KB Prime page once per incoming byte exhausted the HTTP deadline. The parser now reads 512-byte blocks and searches a bounded rolling window once per block. A second flashed build resolved the complete live record as `Prime Video: 2026 - Movie - GOAT (5980s)` and advanced its Android-derived position correctly across polls.
- The live GOAT artwork URL was converted to Amazon's square `._SS192_.jpg` rendition and verified as a valid 192x192 baseline JPEG of 11,386 bytes. Its private URL is 129 characters, within the 200-byte dock field, and feeds the existing half-scale 96x96 RGB565 decoder and acknowledged ESP-NOW cache transfer. The sleeping remote rebooted onto a page without a visible Media widget, so it correctly did not request an artwork transfer during unattended testing; the fetch input and decoder-compatible dimensions were verified separately.
- Dock 1.72 built successfully for the unchanged Dock Rev6 target (1,411,493 bytes of program storage, 73,416 bytes of RAM) and was flashed over USB to dock MAC `40:4c:ca:f9:ef:54`; esptool verified every written image. The release carries `OPENREMOTE_DOCK_VERSION=1.72`. Byte-identical copies are `releases/dock-bin/OpenRemote_Dock_1.72.bin` and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.72.bin`, with SHA-256 `aedb9e3467afc31e613e6c6270ed0b88b873fe02a959dd322a0d6c470e83b30f`.

### 2026-09-11 — Media artwork scaling and transaction completion

- Live Prime posters for `Greenland` and `The Addams Family 2` appeared as the top-left 96x96 pixels of Amazon's 192x192 square source. The dock selected a half-size decode but passed numeric shift count `1` to JPEGDEC; JPEGDEC expects `JPEG_SCALE_HALF` (`2`). It decoded full size and the bounded 96x96 callback clipped the rest. The dock now maps shift levels to the explicit full/half/quarter/eighth flags. A desktop run of the exact callback against the live Addams JPEG produced the complete 96x96 poster.
- Prime and Stremio token-free cache identities now include `v4`, forcing the remote to ignore bad crops stored under earlier keys. The private fetch URL remains on the dock, and the 214-byte metadata packet plus the remote's RGB565 cache format remain unchanged.
- A remote wake could reach a rebooted dock before mDNS and a current Android owner query finished. The dock answered with an empty cached state, the remote closed ESP-NOW, and the resolved Prime record arrived after the radio went down. Dock 1.73 now discovers a missing saved target inside the explicit request and bypasses the two-second Android-owner cache. Remote 4.55 keeps that single request open for up to 45 seconds. Packet `valid=2` marks an explicit settled-empty result without changing its layout.
- Artwork transfer acknowledgements now tolerate eight 700 ms attempts per chunk. Duplicate chunks are safe because the remote returns the next sequence still required. After metadata and any CRC-confirmed cache write finish, the existing operation-complete path sends link-down and turns ESP-NOW off; no four-second polling was restored.
- Remote 4.55 and Dock 1.73 built and were flashed to the unchanged Rev6 boards over USB; esptool verified both images. Remote 4.55 is 2,666,304 bytes with SHA-256 `a4ebfe048d53477e053ee8ebd6fd9dd68767f25754ed228cfadffb67f303c4b3`. Dock 1.73 is 1,460,544 bytes with SHA-256 `9a507c15c8b270b7f8f392f51a59a87c170b71de989cc60781e851adf59b25f3`. Byte-identical copies are local `releases/remote-bin/OpenRemote_4.55.bin`, NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.55.bin`, local `releases/dock-bin/OpenRemote_Dock_1.73.bin`, and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.73.bin`.
- The first transaction test advanced farther after callback ACKs but still stopped at changing chunk numbers. The remote's three-second dock keepalive could collide with an artwork acknowledgement, enter its channel-recovery fallback, and interrupt the active stream. A failed stream also set `mediaArtRequestWanted` after the operation-busy check had run, so the following loop released ESP-NOW before the automatic retry. Remote 4.58 suppresses keepalives throughout media work, retains a callback-ACK fallback for a full Wi-Fi TX queue, treats a visible pending artwork retry as active work, and retries 500 ms after a stalled stream.
- Live Prime Video validation with `The Addams Family 2` completed without another wake or keypress. The first RF attempt stopped at chunk 74, the remote kept ESP-NOW active and requested it again automatically, and the second attempt transferred all 18,432 RGB565 bytes in 116 acknowledged chunks. The remote then logged `Artwork: cached /media/art/e91281ad.565 from the dock`, sent link-down, and stopped ESP-NOW; the dock LED and remote link state both went idle.
- Final release builds are Remote 4.58 (2,666,448 bytes, SHA-256 `90239427eda0976803baed6f46981eaa0a5c5201580503be2b3e9284c7e0c096`) and Dock 1.74 (1,460,544 bytes, SHA-256 `35a35486c56ee52a0aa9bfaedd56f85024a3e2b7d67fbec4e4a146af975bf820`). Their embedded version markers were verified. Byte-identical release copies are local `releases/remote-bin/OpenRemote_4.58.bin`, NAS `SOFTWARE/FIRMWARE/BIN/OpenRemote_4.58.bin`, local `releases/dock-bin/OpenRemote_Dock_1.74.bin`, and NAS `DOCK/FIRMWARE/BIN/OpenRemote_Dock_1.74.bin`.
