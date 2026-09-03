# OpenRemote — AI Handoff, 2026-08-22

Continuation of `OPENREMOTE_AI_HANDOFF_2026-08-21.md` — read that first if you need
earlier history (the public GitHub fork setup, the git-history name scrub, Studio's
universal2 build, Pronto Hex support). This one covers everything since: **ESP-NOW +
RF433, a new Dock firmware project, BuyDisplay panel support, and a large new physical-
button Settings navigation subsystem** — most of a day's work, all on real hardware
feedback rounds.

## Current versions

- **Firmware: 3.19** (`OpenRemote_1.0.ino`). Every version 3.07→3.19 has an archived
  `.bin` at `SOFTWARE/FIRMWARE/BIN/OpenRemote_X.XX.bin` and its own git commit on
  `main`. **3.09 through 3.19 are committed locally but NOT pushed to GitHub** — the
  last push was `aff4b25..fb7bf64` (through 3.08 / WebConfig 2.47), done when explicitly
  asked. Ask before pushing further; don't assume it's wanted just because it was last
  time.
- **WebConfig: 2.47** (`SOFTWARE/WebConfig/WebConfig 2.47.html`, mirrored into the
  firmware repo's `webconfig/` folder). Public on GitHub through 2.47.
- **Studio: 2.67**, unchanged this session — nothing here touched it.
- **Dock: 1.00** (`SOFTWARE/Dock/`) — new this session, see below. Local git repo, no
  remote, not pushed anywhere.

## 1. ESP-NOW transport + RF433 learning (firmware 3.07–3.08, WebConfig 2.41–2.47)

Built and already detailed in the previous handoff's successor commits — summary: the
remote can now pair with an ESP-NOW peer ("blaster dock"), send it IR/RF433 commands,
and relay RF433 learn requests to it. WebConfig has full pairing UI, a learn-mode
toggle (IR beam / RF433 waves animation, TV-remote vs garage-fob graphics), and the
whole thing round-trips through backups correctly. **Fully documented in its own
handoff** — see below.

**Real mistake made and fixed along the way**: bumping WebConfig's version with
`sed 's/v2.42/v2.43/'` (unescaped dot) matched a coincidental `v2X42` substring inside
the landing page's embedded base64 PNG and corrupted it, breaking the Home page image
in 2.43/2.44/2.45 (still live on GitHub, deliberately left as history rather than
rewritten in place). Fixed forward in 2.46. **Memory saved on this** — see
`webconfig-version-bump-never-use-regex.md`: version bumps now use exact literal
string replacement only, never a regex pass over the whole file.

## 2. Dock firmware — new project, `SOFTWARE/Dock/`

The blaster dock's own firmware, separate from the remote. First bring-up version
(1.00) runs on a bare ESP32 dev board with one LED standing in for real IR/RF433
hardware — proves the ESP-NOW protocol end-to-end before any dock hardware exists.
**Full detail in its own handoff**: `SOFTWARE/Dock/AI Handoff/DOCK_AI_HANDOFF_2026-08-22.md`
(that folder is gitignored, same convention as this one).

Key facts: git-initialized, versioned independently starting at 1.00 (+0.01 per bump,
**not** tied to the main firmware's version numbers), no remote configured. The four
ESP-NOW packet structs are copied by hand from `OpenRemote_1.0.ino` into `Dock.ino` -
no shared header exists yet, so a protocol change on either side needs the other
updated manually. User picked a **CC1101 module** (SPI, coil antenna) as the intended
real transceiver but hasn't received hardware yet.

## 3. BuyDisplay panel support (firmware 3.09, 3.15, 3.17)

User's new 2.8" 240×320 IPS panel from BuyDisplay arrived and got wired in.

- **Colours were inverted.** Root cause: `displayInverted` is persisted in *two*
  places - NVS and `settings{}` in `runtime.json` on the SD card - and
  `applySettingsJson()` treats the SD copy as authoritative at every boot, silently
  overwriting NVS. The new "Display Module" dropdown (Adafruit/BuyDisplay, Settings >
  Debug) sets a matching invert default when changed, and **must** call
  `persistSettingsToRuntimeConfig()` to write the SD copy too, not just NVS - 3.09
  missed that and the fix visibly didn't stick until 3.15. Any other setting that has
  this same dual-persistence shape will have the same trap.
- **Touch driver**: a third "BuyDisplay" option now exists in the Touch Driver
  dropdown, but it is **not a real distinct implementation** - every code path already
  branches specifically on `touchDriverChoice == 1` (FT5x06), so the new value 2 falls
  through to the exact same Wire-based 0x38/register-0x02 read as "Adafruit". This was
  a deliberate choice, not an oversight: BuyDisplay's product page sits behind
  Cloudflare and blocked every automated fetch attempt, so the real touch IC was never
  confirmed. Circumstantial evidence (an FT6206-family accessory SKU, matching I2C
  address) suggests it may already work as-is. **User has not yet reported back**
  whether touch actually works on the new panel - that's the open question.

## 4. Physical-button Settings navigation - the big new subsystem (firmware 3.10–3.19)

User's original request: hold **Stop + Forward** together to jump into Settings and
lock touch out; navigate with **D-pad/OK/Back**; no physical button should fire an
IR/BLE/RF command while Settings is showing. Built from scratch - this remote's UI was
entirely touch-first before today. **This was a long, iterative, real-hardware-driven
build** (ten firmware versions), and the two most important things learned about
LVGL's keypad indev are worth stating plainly so nobody re-derives them the hard way:

> **LVGL's keypad indev (`indev_keypad_proc` in `lv_indev.c`) only ever moves group
> focus on `LV_KEY_NEXT`/`LV_KEY_PREV`. Every other key - UP/DOWN/LEFT/RIGHT/ENTER
> included - goes straight to whatever object is currently focused via
> `lv_group_send_data()`, and never touches focus.**
>
> **It only ever fires `LV_EVENT_RELEASED` for `LV_KEY_ENTER`.** Any widget or app
> code that gates real work (saving a setting, rebuilding a display LUT, committing a
> value) behind `LV_EVENT_RELEASED` will silently never run that code for a keypad-
> driven LEFT/RIGHT/UP/DOWN change, no matter how correct the rest of the flow is.
> Both facts came from reading LVGL's actual source (`.pio/libdeps/openremote_rev5/
> lvgl/src/core/lv_indev.c`), not guessing, after multiple rounds of "traced this and
> it should work" turning out to be right about the mechanism but incomplete about
> where in the app it broke.

### What exists now

- **Entry**: holding physical Stop (button index 0) + Forward (index 3) calls
  `enterPhysicalSettingsNav()` - jumps to `SETTINGS_HOME`, locks touch
  (`physicalNavTouchLocked`, checked at the top of `lvTouchRead()`). Their own bound
  IR/BLE commands still fire individually on the way to triggering the combo -
  deliberate, documented tradeoff (buffering to prevent that would add latency to
  every ordinary Stop/Forward press).
- **Navigation**: a real `lv_group_t` (`physicalNavGroup`) + `LV_INDEV_TYPE_KEYPAD`
  indev (`physicalNavInputDevice`). D-pad Up/Down send `LV_KEY_PREV`/`NEXT` (moves
  group focus) *unless* the focused object is an open dropdown, in which case they
  send true `LV_KEY_UP`/`DOWN` so the dropdown scrolls its own list instead of losing
  focus (`physicalNavFocusedDropdownOpen()`). D-pad Left/Right send `LV_KEY_LEFT`/
  `RIGHT` *only* when the focused object is an open dropdown or a slider
  (`physicalNavFocusedWantsLeftRight()`) - everywhere else (a plain button, a switch)
  they fall back to the same PREV/NEXT group navigation Up/Down uses, because a plain
  object has no built-in meaning for LEFT/RIGHT at all.
- **Back** does double duty: one press steps a subpage back to Settings Home, a second
  press from Home itself exits to Activities (`exitPhysicalSettingsNav()`, restores
  touch). Menu and Return are deliberately unbound - Settings has no deeper hierarchy
  than one level today, so a second dedicated button added nothing.
- **Every physical button press resets the sleep timer** (`lastWakeMs`), not just ones
  that wake an already-sleeping display - that was a real, separate bug (only
  `wakeDisplay()` touched it before).
- **Focus indicator**: `addPhysicalNavFocusable()` is the single choke point every
  focusable widget must go through - it applies a shared outline style
  (`physicalNavFocusStyle`, red `#ff453a` matching WebConfig's own system red, 3px,
  applied to *both* `LV_PART_MAIN` and `LV_PART_INDICATOR` since LVGL's default theme's
  own blue focus outline lives on `LV_PART_INDICATOR` for switches/sliders and was
  winning there otherwise) and tracks the object in `physicalNavRegisteredObjs[]` for
  same-screen rebuild restoration (below). A "breathing" animation (2-6px outline
  width, 900ms each way, infinite) runs on whichever object is focused, driven by
  `lv_group_set_focus_cb()` → `physicalNavFocusChanged()`. **Auto-hides after 2s of no
  physical button activity** (`servicePhysicalNavIdleHide()`, tracked via
  `physicalNavLastKeyMs`) and comes straight back on the next press.
- **Same-screen rebuild preservation**: toggling a switch, moving a slider, the
  ESP-NOW device list refreshing - anything that calls `renderSettingsPage()` again
  for the screen already showing - used to reset focus to the first row and scroll to
  the top every time, since the whole screen's LVGL objects get destroyed and rebuilt.
  `renderSettingsPage()` now distinguishes that from a genuine navigation to a
  *different* screen (`settingsView` vs. `physicalNavLastRenderedView`) and only on a
  same-screen rebuild restores which registered index was focused and the scroll
  offset.
- **Coverage**: `makeSettingRow()`/`makeOmoteRow()` (all switches and clickable rows),
  every dropdown builder (the shared `makeOmoteDropdownRow()` plus 8 classic-style
  single-dropdown rows plus the row-calibration and microphone dropdowns), both shared
  slider builders (Display and Buttons pages), and the Soft/Hard reboot buttons
  (previously built with plain `makeButton()`/`makeOmoteButton()` outside those
  helpers and so were **completely unreachable by D-pad**, not just unstyled - found
  and fixed while working on outline thickness).
- **Modal support**: the reboot-confirmation popup (`lv_msgbox`) gets its own separate
  group (`physicalNavModalGroup`), swapped onto `physicalNavInputDevice` while it's
  open and back to `physicalNavGroup` when it closes - otherwise the settings page
  underneath would also stay reachable by D-pad while the popup was up. LVGL's msgbox
  has no public accessor for its internal button matrix in this version, so it's found
  by checking each child's type against `lv_btnmatrix_class`.

### Two real bugs found and fixed by reading LVGL source, not guessing

1. **Slider Left/Right silently reverted.** `scrollSafeSliderEvent()` - existing app
   code, built to tell a real slider drag apart from a touch-scroll gesture that
   happens to start on top of a slider - listens for `LV_EVENT_VALUE_CHANGED` and
   reverts the slider back to its last committed value unless `state->tracking`/
   `horizontal` (only ever set by a real `LV_EVENT_PRESSED`+drag) are true. A keypad
   LEFT/RIGHT press changes the value and fires that same event directly with no
   press/drag ever seen first, so every keypad-driven change landed in the "not a
   confirmed drag" branch and got undone immediately. Fixed by checking
   `lv_indev_get_type(lv_indev_get_act()) == LV_INDEV_TYPE_KEYPAD` and accepting the
   change outright when true - a discrete keypress has no scroll-vs-adjust ambiguity
   to resolve the way a touch drag does.
2. **Gamma (and every other Display/Buttons slider) not applying or saving.**
   `displaySliderEvent()`/`buttonSliderEvent()` both gated their real work
   (`rebuildDisplayColourLut()`, `saveSettings()`/`scheduleRuntimeSettingsSave()`)
   behind `LV_EVENT_RELEASED` - correct for touch, which fires that once per drag, but
   never fired at all for a keypad press per the LVGL fact stated above. Fixed the
   same way: also commit when the active indev is `LV_INDEV_TYPE_KEYPAD`.

### What's still genuinely not done

- **The ESP-NOW device management modal's buttons are not wired into physical nav at
  all.** It wants the same separate-group treatment given to the reboot popup, not a
  quick add to the page-level group (it rebuilds itself on every add/remove/scan-
  result, which would need the same careful handling `renderSettingsPage()` gets) -
  flagged, deliberately not done under time pressure rather than rushed.
- Not every settings screen has been individually walked on hardware - the ones built
  from the shared row/dropdown/slider helpers all got registration "for free," but
  that's coverage-by-construction, not verification. Expect more of this kind of
  real-hardware iteration if the user keeps testing screens one at a time.
- Whatever the user finds next - this feature had five distinct real-hardware
  iteration rounds today (3.10→3.19) and there's no reason to expect it's now bug-free
  everywhere; it just means the specific things reported were fixed.

## Other housekeeping this session

- Deleted a redundant 1.2GB copy of `SOFTWARE/` from the user's Synology NAS
  (`/Volumes/home/...`, AFP mount) at their request - they're keeping the Mac-local
  copy only now. Confirmed no BSD immutable flags were actually blocking it; Finder's
  "locked" complaint didn't reflect anything `rm -rf` from Terminal couldn't handle.
- Two new memory files saved this session:
  `webconfig-version-bump-never-use-regex.md` (the base64-image-corruption incident)
  and `archive-a-bin-for-every-firmware-version.md` (missed archiving 3.07/3.08's
  `.bin`s until asked where they were - both had to be reconstructed after the fact).

## Standing rules, restated because they matter

- **Real name must never appear anywhere** - commits, files, comments. Everything is
  authored as `LORDSn1per <112445119+LORDSn1per@users.noreply.github.com>`.
- **Every firmware version bump**: changelog entry (root-cause style, explain *why*)
  + version string + git commit + archived `.bin` in `SOFTWARE/FIRMWARE/BIN/`, never
  overwritten. All four, every time, in the same step - not remembered later.
- **WebConfig versions**: copy to a new file, never edit in place. Version-string
  updates are exact literal replacements only, never a regex/sed pass over the file
  (see the base64-corruption incident above).
- **Dock firmware**: same `.bin`-archiving discipline, its own version series starting
  at 1.00, independent of the main firmware's numbers.
- Don't push to GitHub, don't touch the backup remote, don't delete anything outside
  what's explicitly asked - all of this session's git/NAS actions were explicit,
  individual requests, not standing permission for the next one.
