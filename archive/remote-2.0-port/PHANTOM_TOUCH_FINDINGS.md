# Phantom touch — investigation results

> **Read this correction first.** An earlier draft of this document claimed DMA
> was *the* root cause. That was over-claimed on a single 5-minute measurement.
> DMA is **necessary** — a static screen has never produced one ghost in
> 300,000+ polls — but it is **not sufficient**. Since that first evening burst
> (288 ghosts in 5 minutes) I have not been able to reproduce the high-ghost
> state at all: 500,000+ polls under maximum DMA load, held and on the desk,
> filtered and unfiltered, all essentially zero. The trigger that puts the
> sensor into the bad state is still unidentified.



Overnight investigation, 2 August 2026, on OpenRemote_2.0 (OMOTE Rev 5 / ESP32-S3).
Nothing in `OpenRemote_1.0` was touched.

---

## The answer

**The LCD's DMA traffic is what creates the ghost touches.** Not the backlight,
not GPIO 45, not I²C corruption. Measured directly, with nobody touching the
remote — so every touch counted below is by definition a ghost:

| Condition (5 minutes each, same build, same threshold) | Ghost touches | Polls | Rate |
|---|---:|---:|---:|
| Display under continuous DMA redraw | **288** | 7,287 | **3.95 %** |
| Display completely static | **0** | 9,960 | **0.00 %** |

Zero in ~10,000 polls with a static screen. The only difference between the two
phases is whether the panel was being repainted.

This finally explains every earlier result at once:

- Arduino_GFX "100 % effective" — it is synchronous GPIO, no DMA.
- Lower LCD clock helped partially — less switching, less coupling.
- Drive strength did nothing — it is not pad EMI, it is bus switching activity.
- Backlight PWM did nothing — never was the mechanism.
- **Stock OMOTE also ghosts** (you confirmed this) — it uses the same LovyanGFX
  8-bit parallel DMA path. It just repaints less often, so it ghosts less often.
  The old handoff doc's "OMOTE has zero phantom touches" comparison was wrong.

## What a ghost actually looks like

I read the FT6206's raw register block, including the two fields LovyanGFX
throws away (event flag and touch weight/area). Over 360 ghosts:

- **weight register is always exactly 16**, **area always 0**, always 1 point
- **~73 % exist for exactly one poll** and are gone by the next one
- position is *structured, not random*: **X is tightly clustered around the
  middle of the sense axis** (mean 106, stdev 5) while **Y is spread across the
  whole panel** (stdev 66)

That X/Y asymmetry is the giveaway. In a mutual-capacitance sensor, noise
coupling into *every sense line at once* puts the computed centroid in the
middle of that axis, while the drive line being scanned at that instant decides
the reported position on the other. This is textbook noise injection into the
sensor — the controller genuinely believes it is being touched, which is exactly
why 2.43's double-read validation could never have helped (LovyanGFX already
does that internally, and the bad value survives it).

The controller is never resetting — its ID registers read back stable
throughout, so this is not a brown-out.

## Important caveat: the rate drifts enormously

Same build, same settings, ~40 minutes apart: **3.95 % → 0.09 %**, a 44× drop.
Something environmental modulates it heavily. The two candidates I could not
separate overnight are **your proximity** (a hand near the panel couples
strongly into a capacitive sensor — and you were up for the first run, asleep
for the second) and **USB charging current** as the battery topped up.

This matters: it means the bug is worst exactly when you are holding the remote,
and it is why the threshold sweep below came out inconclusive.

## What I changed

`hardware/ESP32/lvgl_hal_esp32.cpp` — a phantom-touch rejection filter, on by
default (`PHANTOM_TOUCH_FILTER`). Because ghosts *only* appear shortly after a
panel flush, it:

- accepts a touch **immediately** when the screen has been quiet — so normal
  tapping keeps its usual latency, no sluggishness
- requires a **second, spatially consistent sample** only when the panel was
  redrawn in the last 150 ms — the only window where ghosts can occur

Tunables at the top of the file: `TOUCH_DMA_BUSY_MS` (150), `TOUCH_MAX_JUMP_PX` (30).

This is a mitigation, not a cure — it should remove the large majority of
ghosts, not all of them. **It needs your validation on real hardware.**

## The likely mechanism (found after you reported "ghosts come shortly after waking")

That single observation cracked it, and exposed a blind spot: **every soak test I
ran had sleep disabled**, so I could never have reproduced this.

`enterSleep()` sets `LCD_EN` high, cutting the LCD rail — **which also powers the
FT6206 touch controller**. So the touch controller is power-cycled on every
sleep/wake. On the way back up, `init_tft()` did:

```c
digitalWrite(LCD_EN_GPIO, LOW);   // powers LCD *and* touch controller
delay(5);                          // sized for the LCD driver, not the touch IC
tft.init();                        // inits touch, then immediately drives the panel
```

A FocalTech FT6x06 needs roughly **300 ms** from power-up to valid operation. At
5 ms it was being initialised mid-boot, and it then established its capacitive
zero-reference while:

1. the LCD had already started its DMA traffic, and
2. because waking is triggered by *picking the remote up*, a hand was usually
   still near the glass.

A baseline captured under those conditions is simply wrong. Once the hand moves
away the sensor is left hypersensitive, and normal DMA noise starts tripping it —
producing a burst of ghosts after a wake that fades as the baseline re-tracks.

This fits every surviving observation: episodic and roughly "1 in 500" (depends
on hand position at the moment of wake), independent of table/couch/plugged, and
requiring DMA to actually trip. It also explains why **stock OMOTE ghosts too** —
this is stock OMOTE code, unchanged.

**Fix applied:** the settle delay is now `TOUCH_POWER_ON_SETTLE_MS` (300 ms) and
the panel stays quiet across it, giving the controller a clean window to
calibrate in. Costs ~300 ms on a wake that already takes seconds.

**This needs your real-world validation** — it is a strong hypothesis with a
mechanism, not something I could prove on the bench, because reproducing it needs
a hand near the glass at the moment of wake.

## Theories killed this session (do not revisit)

| Theory | How it died |
|---|---|
| Backlight PWM harmonics | Ghosts persist at 100 % brightness where PWM stops switching entirely |
| GPIO 45 blink (mic power) | Blink stopped; ghosts remained |
| Stock OMOTE is immune | You see ghosts on stock OMOTE too — this had anchored the whole prior investigation on a false premise |
| I²C corruption | LovyanGFX already double-reads and compares; ghosts survive it. The controller genuinely believes it is touched |
| Weight/area filtering | Real fingertips read `weight=16, area=0` — **identical** to ghosts. No register-based discriminator exists |
| Your proximity | 0 ghosts in 7,491 polls under DMA load while you held it; and you see ghosts with it sitting on a table |
| USB charging current | You see ghosts both plugged and unplugged |
| Ghost→UI→more-DMA feedback loop | Filter compiled out, same desk: 0 ghosts in 7,465 polls |
| WiFi retry storm timing | Apparent correlation was an artifact of when the serial capture attached |

## Two things I could not finish, and how to finish them

**1. Does a real finger read a different weight/area?** Every ghost reads
weight=16, area=0. If a real touch reads anything else, a *perfect zero-latency*
filter is possible — just reject weight==16 && area==0. I could not test this
because it needs a finger. **This is a 30-second job for you:**

```bash
cd "…/OpenRemote_2.0"
~/.platformio/penv/bin/platformio run -e esp32-s3-soaktest -t upload --upload-port /dev/cu.usbserial-1330
```
then tap the screen a few times and capture the serial log — every touch prints
its full register block. If real taps show weight≠16 or area≠0, tell me and I
will make the filter exact.

**2. Raising the controller's touch threshold** (`ID_G_THGROUP`, register 0x80).
Writes are verified working (readback confirmed). The sweep env
`esp32-s3-sweeptest` steps it every 3 minutes under DMA load — but the ghost
rate had already collapsed to near zero, so it proved nothing. Re-run it when
the rate is high (i.e. while handling the remote).

## Current state of the remote

It is still running the **soak diagnostic build** overnight, logging to
`overnight.log`, cycling DMA-stress on/off every 5 minutes and counting how many
ghosts the new filter suppresses. It never sleeps and constantly repaints — that
is deliberate, not a bug.

To put it back to normal firmware:

```bash
~/.platformio/penv/bin/platformio run -e esp32-s3-Rev5andHigher -t upload --upload-port /dev/cu.usbserial-1330
```

The normal build already includes the filter. The diagnostic code is entirely
behind `PHANTOM_TOUCH_SOAK` and does not exist in it.

## If you want the ghosts gone completely

The filter is the cheap fix. If it is not enough, the honest options are, in
order of preference:

1. **Perfect the filter** using the weight/area test above — costs nothing.
2. **Raise `ID_G_THGROUP`** — makes the sensor deaf to weak signals. Costs some
   touch sensitivity; needs the sweep re-run while the rate is high.
3. **Lower the LCD clock** to 16–20 MHz — you already know this helps and costs
   some refresh speed.
4. **Hardware**: a grounded shield layer between LCD and touch sensor, or better
   bulk decoupling on the shared rail. This is the only true fix — everything
   above is working around a layout that lets the panel talk to the sensor.
