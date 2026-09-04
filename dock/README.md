# OpenRemote Dock

A mains-powered companion to the OpenRemote remote. The remote is a battery
device that spends most of its life asleep; the dock is always on, sits where
the equipment is, and does the transmitting the remote cannot do from a
cabinet or another room.

It is a real ESP-NOW peer, not a stand-in: it speaks the wire format the remote
firmware implements, so it pairs and takes commands through the remote's
existing **Settings → Dock** screens and through WebConfig.

Current firmware: **1.30**. Source in [`firmware/`](firmware/).

[**Download the latest dock firmware →**](https://github.com/LORDSn1per/OpenRemote-Firmware/releases/latest/download/OpenRemote-Dock-Firmware-1.30.bin)

You do not need to build it. There are two ways to install it, and either is
fine: send it to a paired dock **wirelessly from WebConfig**, or flash it
**over USB from Studio** using *New Dock*. A dock that has never been flashed
needs the Studio route once; after that both work.

---

## What it does

**Relays IR.** Every IR command the remote can send, the dock can send instead
— or as well. The remote's **Transmit IR from** setting chooses: *Remote*,
*Dock*, or *Both*, the last covering a room from two directions at once.

**Sends real RF433.** A CC1101 transceiver captures a signal from an existing
gate, garage or socket remote and replays it. Captured as a raw OOK edge train
rather than a decoded protocol, which is what lets it reproduce codes nothing
recognises. Learning is driven from WebConfig; the dock opens a receive window,
captures the burst, and sends the timings back over ESP-NOW.

**Updates over the air.** New dock firmware is uploaded through WebConfig,
stored on the remote's SD card, then pushed to the dock over ESP-NOW in 160-byte
chunks. The dock writes to its spare flash slot and verifies the whole image
before switching to it, so a failed transfer leaves it running the firmware it
already had. No cable needed after the first pairing.

**Reports itself.** Firmware version, ESP-NOW address and link state appear in
WebConfig beside the remote's own status, and the remote's status pill shows a
blue outline whenever the dock is transmitting.

---

## Hardware

An **ESP32-C3 Super Mini** — single-core RISC-V, 4MB flash, 400KB SRAM, no
PSRAM. The firmware uses about 39KB of RAM and 77% of a 1280KB app partition.

Since 1.30 the same binary drives both the bare Super Mini and the **Rev 6
dock PCB**, which adds an external status LED and a menu button alongside the
board's own. Both LEDs show the same states despite opposite polarities, and
either button performs the same gestures.

| | Super Mini | Rev 6 PCB |
|---|---|---|
| Status LED | GPIO8, blue, **active low** | D5 on GPIO1, **active high** |
| Button | GPIO9 (BOOT) | SW1 on GPIO3 |
| IR emitter | GPIO0 | GPIO0, MOSFET gate |
| Serial | USB CDC at 115200 — no USB-serial bridge on this board |

### IR emitter

```
  GPIO0 ──[ 100Ω ]──►│──── GND
                     ▲
                  IR LED
             (long leg = anode,
              toward the resistor)
```

`IrSender.begin()` is not inverted, so the pin **sources** current and HIGH is
on. 100Ω gives about 20mA, comfortable for direct drive; do not go below 68Ω,
as the C3's absolute maximum is 40mA per pin. Good for a metre or three — a
bench figure, not room coverage. The Rev 6 board drives the LED through a
MOSFET instead and reaches much further.

### CC1101 433MHz module

Wire **VCC to 3V3 — never 5V**; the part is 1.8–3.6V and 5V destroys it.

| CC1101 pin | | C3 |
|---|---|---|
| 1 GND | → | GND |
| 2 VCC | → | 3V3 |
| 3 GDO0 | → | GPIO10 — data in/out, the pin that carries OOK |
| 4 CSN | → | GPIO7 |
| 5 SCK | → | GPIO4 |
| 6 MOSI | → | GPIO6 |
| 7 MISO/GDO1 | → | GPIO5 |
| 8 GDO2 | → | GPIO20, optional |

MISO doubling as GDO1 is normal for the CC1101. The SPI pins are the C3's
defaults, so the hardware peripheral does the work. GPIO2 is avoided
deliberately — it is a strapping pin read at reset.

At boot the dock reads the CC1101's part and version registers back over SPI,
so a missing or miswired module says so immediately rather than looking fine
until a capture silently returns nothing.

---

## Pairing and the LED

Hold the button **5 seconds**. The dock broadcasts itself on channels 1–13 for
30 seconds. On the remote, **Settings → Dock → Search for a dock**, then pick it.

| LED | Meaning |
|---|---|
| Off | Idle — no link, or the radio is asleep |
| Solid | Linked to the remote |
| Blink, 120ms | Pairing window open |
| Solid 10s | Just paired |
| Blink, 45ms | Pairing failed or timed out |
| Blink, 500ms | Receiving firmware |
| Brief flash | Transmitting IR or RF |

Hold the button **10 seconds** to forget the remote and unpair.

The LED mirrors the remote's status pill: both light on a confirmed link and
both go dark on the same event, because the remote sends an explicit link-down
packet before it powers its radio off rather than leaving the dock to time out.

---

## The link

ESP-NOW, on whichever Wi-Fi channel the remote is using. The remote does not
choose that channel — its router does — so the remote states its current
channel in every keepalive ping and the dock follows it. That matters more than
it sounds: adjacent 2.4GHz channels overlap heavily, so a dock one channel away
still hears short frames while long ones never arrive, which looks like
anything except a channel problem.

The radio is on demand. It comes up when something needs it and goes two
seconds after the last use, so an idle remote is not paying to hold a link open.

| Direction | Magic | Purpose |
|---|---|---|
| dock → remote | `OREN` | announce, while pairing |
| remote → dock | `ORPA` | pairing acknowledgement |
| remote → dock | `ORCM` | IR or RF command |
| remote → dock | `ORPG` | keepalive ping, carries the remote's channel |
| dock → remote | `ORDI` | dock info: firmware version |
| remote → dock | `ORDS` | dock settings: RF on/off, LED on transmit |
| remote → dock | `ORLD` | link going down |
| remote → dock | `ORLS` | open an RF433 receive window |
| dock → remote | `ORLR` | captured RF timings |
| remote → dock | `OROB` `OROD` `OROE` | firmware transfer: begin, data, end |
| dock → remote | `OROA` | transfer acknowledgement |

Every shared struct is pinned by `static_assert` on both sides, so a field added
to one firmware and not the other fails the build rather than letting the two
quietly misread each other on air. This is the main reason the dock and the
remote live in one repository.

---

## Building

```
cd dock/firmware
pio run                 # build
pio run -t upload       # flash over USB
```

The Super Mini has no USB-serial bridge — the C3's own USB port is the only one,
appearing as `/dev/cu.usbmodem*`. If it does not appear, hold **BOOT**, tap
**RESET**, release **BOOT** to force the bootloader.

A dock that has never been flashed needs one pass over USB - from Studio's
*New Dock* tab, or with `pio run -t upload` above. After that you can update it
either way: wirelessly from WebConfig, or over USB from Studio again.

---

## Notes worth keeping

**Serial output is deliberately lossy.** `Serial.setTxTimeoutMs(0)` is set at
startup because `HWCDC::write()` waits for a host to drain its buffer — up to
100ms per write with nothing attached. The dock prints two lines per IR command,
so with no monitor connected each command was stalling the loop ~200ms and
dropping ESP-NOW frames during the stall. It looked exactly like a power-saving
bug. Logging now never blocks, at the cost of dropping lines when nobody is
reading, so a missing line is not proof that something did not happen.

**Wi-Fi power save is off.** An ESP32 station duty-cycles its radio by default,
assuming an access point will buffer what arrives while it sleeps. ESP-NOW has
no access point and no buffering, so a frame arriving during a nap is simply
gone. The dock is mains powered and never sleeps its radio.

**The app partition is 1280KB and the image is at 77%.** Both slots must hold
it for OTA to work, so that is the ceiling, not the 4MB flash. There is 1408KB
of unused SPIFFS available if it ever needs repartitioning — which would have
to be flashed over USB at `0x0` and would erase the pairing.
