/*
  OpenRemote Dock firmware change log (newest first)

  1.33 - 2026-09-06
    - Fixes the Homebridge relay failing with "login failed (HTTP -1)". The
      dock hardcoded "http://" in front of the configured address, so an
      address that already carried a scheme became
      "http://http://192.168.x.x:8581/..." and the dock tried to resolve a host
      literally named "http". Its own log said so plainly once both boards were
      captured on one clock:
        hostByName(): DNS Failed for 'http' with error '-54'
    - The address is now normalised exactly as the remote has always normalised
      it - scheme added only when missing, any trailing path or slash removed.
      The remote had this from the start; the dock simply never got it, which
      is what a second implementation of the same thing tends to cost.
    - Failures now name the URL they tried. A bare "HTTP -1" sent this
      investigation toward credentials and Wi-Fi when the address was malformed
      all along.

  1.32 - 2026-09-06
    - Keeps the Wi-Fi association awake, which is the entire point of relaying
      Homebridge through the dock. 1.31 set WIFI_PS_NONE at boot, but that is
      before the station associates - and associating puts the Arduino default
      WIFI_PS_MIN_MODEM back. A station in modem sleep parks its radio between
      beacons, which would have slowed every HTTP call and lost ESP-NOW frames
      arriving during a nap. The same fault the remote hit in 3.93, and it
      would have quietly undone the benefit the relay exists to provide.
    - Power save is now cleared when the association comes up AND re-asserted
      every five seconds while it holds, because a reconnect handled inside the
      SDK never comes back through the join branch and would silently restore
      modem sleep.
    - setAutoReconnect(true), so a brief access point outage is recovered by the
      SDK rather than waiting for the twenty second retry loop, which stays as
      the backstop for when it gives up entirely.

  1.31 - 2026-09-06
    - Relays Homebridge for the remote. The remote can now hand a Homebridge
      command to this dock instead of issuing it itself, chosen by a switch and
      off by default.
    - Worth doing because of what the dock is: mains powered, so it holds its
      Wi-Fi association open permanently and a command is one HTTP round trip
      on an already-warm connection. The remote has to power a radio and
      associate first, which costs seconds every single time. Toggle and step
      operations gain most, since those need a read before the write - two
      round trips on a warm link rather than an association plus two on a cold
      one.
    - The dock logs in and keeps its own bearer token, re-logging in whenever a
      request comes back 401 or 403. The remote never sends a token, so there
      is no shared expiry for the two ends to disagree about.
    - The login reply is parsed by hand rather than with a JSON library. It is
      one known field in one known response, and a parser would have cost far
      more flash than it saved on a part already at three quarters of its
      partition.
    - Joining Wi-Fi pins the radio to the router's channel, which is the same
      channel the remote already states in every keepalive ping, so the two
      stay in step. If the association moves the radio, the dock relocks
      ESP-NOW to wherever it landed.
    - Credentials arrive as two ESP-NOW frames - Wi-Fi and Homebridge details
      separately - because the pair does not fit in one 250 byte frame. They
      are stored in NVS and applied from loop(), never from the receive
      callback, which runs on the Wi-Fi task and must not write NVS.
    - Flash use rises from 77% to 90% of the app partition, almost entirely
      HTTPClient. It fits, and both OTA slots still hold it, but there is now
      about 128KB of headroom rather than 297KB. The unused 1408KB SPIFFS
      partition is the room to reclaim if that ever becomes tight.

  1.30 - 2026-09-02
    - Keeps the ESP32-C3 Super Mini's onboard GPIO8 status LED and GPIO9 BOOT
      button intact while adding the Rev 6 PCB's external D5 status LED and SW1
      menu button. Both LEDs now show the same link, pairing and transmit states,
      despite their opposite electrical polarities, and either button performs
      the existing tap / pair / forget gestures.
    - Moves only the configurable board pins to the Rev 6 schematic: the active-
      high IR MOSFET gate is GPIO0, CC1101 GDO0 is GPIO10 and GDO2 is GPIO20.
      The SPI pins remain GPIO4/5/6/7. No radio, command, pairing or OTA behaviour
      changes.

  1.29 - 2026-09-02
    - Fixes the dock firmware update. The dock was aborting the transfer five
      milliseconds after accepting it, long before the remote could physically
      have answered, and then reporting the remote as having stopped
      responding.
    - A dual capture of both boards on one clock showed it plainly: begin frame
      received at 28.890s, "firmware transfer starting" at 33.127s - 4.2 seconds
      of esp_ota_begin() erasing the partition - then "no data yet" at 33.131s
      and "aborted" at 33.132s. Five milliseconds.
    - The cause is that "now" is read once at the top of loop() and the erase
      happens inside that same iteration. Afterwards every timer here compared a
      four-second-stale "now" against timestamps taken AFTER the erase, and
      being unsigned the subtraction underflowed to an enormous value, so the
      stall timeout and the ack repeat both fired immediately.
    - The clock is now re-read after a handler runs, since these handlers can
      block for seconds, and the two timers use signed comparisons so a
      timestamp in the future can never read as a very old one.
    - Worth recording: the remote was never at fault. Its own log said "now is
      0ms behind millis" throughout, which is exactly why every fix aimed at
      that side - chunk size, retries, ack codes, channels - changed nothing.

  1.28 - 2026-09-02
    - Closes the window where a frame arriving mid-handler was refused. The
      pending flag was cleared AFTER the handler returned, and otaHandleBegin()
      sits inside esp_ota_begin() for seconds erasing the partition and then
      sends its acceptance as its last act - so everything arriving in that
      whole span, including the remote's reply a millisecond after the
      acceptance, hit the "still holding the last one" gate and was thrown
      away. One dropped chunk there is fatal when the sender does not retry.
    - The frame is now copied out and the flag cleared before the handler runs,
      so the receive callback can accept the next frame while this one is still
      being written to flash rather than turning it away.

  1.27 - 2026-09-02
    - The frame counters are cleared before esp_ota_begin() rather than after
      it. They were being reset once the erase had finished, so every frame
      dropped DURING the erase - the seconds when this dock is least able to
      listen and the remote is most likely to be sending - was counted and then
      wiped before anything could report it. The line read a confident "0
      frame(s) heard, 0 dropped" while the drops it existed to measure had
      already happened and been erased.
    - No behaviour change; the counting is simply honest now. The actual fault
      was on the remote and is fixed in 4.00: this dock has sent OTA_ACK_STALLED
      since 1.19 and the remote never knew that code, so it read the dock's
      abort as an acceptance and sent data at a dock that was still erasing.

  1.26 - 2026-09-02
    - Counts what actually reaches the dock during a firmware transfer, because
      the chunk size was not the answer. 1.25 dropped the frame from 250 bytes
      to 170 - inside everything else known to cross this link - and not one
      chunk still arrived, so a third guess at the size would be exactly that.
    - Two counters, reported in the "no data yet" line: how many frames of any
      kind were heard from the remote while the transfer was open, and how many
      OTA frames were refused by the "still holding the last one" gate. Those
      separate the only two explanations left. Frames heard but dropped as busy
      means the remote retries faster than this dock drains one, and the
      transfer starves while each end blames the other. Nothing heard at all
      means the chunks are not arriving, and the fault is below this firmware.
    - No behaviour change beyond the counting.

  1.25 - 2026-09-02
    - Matches remote 3.96: the firmware chunk is 160 bytes rather than 240. The
      two must agree, and the static_assert on the frame size fails the build
      here if they ever drift.
    - 240 plus the 10 byte header was exactly 250 - ESP_NOW_MAX_DATA_LEN - and
      at that size this dock's radio acknowledged the frame while its ESP-NOW
      stack discarded it before the receive callback ran. That is why every log
      from this side said "no data yet" while the remote was certain it had sent
      the chunk and had a MAC acknowledgement to prove it.

  1.24 - 2026-09-02
    - The dock follows the remote's channel when the remote says it has moved.
      Remote firmware 3.95 puts its current channel in every keepalive ping;
      this acts on it. Together they fix a dock that pairs, works, and then
      stops the moment WebConfig is opened.
    - The remote does not choose its channel. With Wi-Fi off it runs ESP-NOW
      standalone on the stored channel; the instant its station associates it
      moves to whatever the router dictates. Opening WebConfig brings that
      station up, so the remote moves and the dock, which cannot see it happen,
      stays behind. Paired and working, then dead, with a re-pair the only cure
      - exactly as reported.
    - It also explains pairing landing on the wrong channel in the first place.
      The dock recorded whichever channel it was sweeping when it heard the
      pair ack, and adjacent 2.4GHz channels overlap so heavily that the remote
      can be heard from one or two channels away: it locked to 7 with the
      remote on 8, and to 9 before that. It was never measuring the remote's
      channel, only its own position when a frame leaked through.
    - Believing the remote instead of guessing costs nothing and needs no
      sweeping - the same leakage that caused the confusion is enough to carry
      the correction. Unlike the sweep removed in 1.23, the dock never leaves
      its channel to do this, so a healthy link is never interrupted.
    - Applied from loop() rather than the receive callback, since relocking
      writes NVS and retunes the radio, and it stands down during a firmware
      transfer, an RF capture or pairing.

  1.23 - 2026-09-02
    - Removes the automatic channel resync added in 1.21 and tuned in 1.22. It
      made things worse, not better, and tuning it twice did not change that -
      so it comes out rather than getting a third attempt.
    - The arithmetic it was always going to lose to: sweeping puts the dock on
      its locked channel only 31% of the time, so roughly two commands in three
      arriving mid-sweep are simply missed. Worse, 1.22 began sweeping after 20
      seconds of not having heard the remote SINCE BOOT - which with an
      on-demand remote whose radio is off almost all the time is the completely
      normal state after any dock power-up. The dock therefore spent most of
      its life sweeping and deaf, which is exactly the "dock no longer connects
      or works" that was reported.
    - The problem it was aimed at is real: the remote does not choose its
      channel, its router does, and when that moves the dock is stranded. But a
      dock that is deaf most of the time to fix a fault that happens rarely is
      a bad trade, and re-pairing already fixes it in a few seconds with the
      user present and expecting it. Anything automatic here needs to cost
      nothing while the link is healthy, and this did not.
    - Behaviour is now identical to 1.20. Everything else from 1.21 and 1.22
      was part of the sweep and went with it; the CC1101 support, the serial
      blocking fix and the OTA acknowledgement retries are untouched.

  1.22 - 2026-09-02
    - A dock that has heard nothing at all since boot starts sweeping after 20
      seconds rather than 2 minutes. There is no established link to disturb in
      that case, so the caution the 2 minute figure exists for does not apply -
      and it is precisely the situation after a restart with the channel
      already wrong, where waiting is two minutes of guaranteed silence.
    - The resync sweep dwells 900ms on each candidate channel rather than
      200ms. Paired with the remote's faster ping while a link is unproven
      (3.94), a sweep now finds the remote within a single pass instead of
      taking minutes. At 200ms the dock sat on the right channel about three
      percent of the time, which was correct but slow enough to look broken.

  1.21 - 2026-09-02
    - The dock now finds the remote again by itself when the channel moves,
      instead of needing a manual re-pair. This was the real cause of "the dock
      stopped responding" during a firmware update: the remote was on channel 8
      and the dock on 9, and both logs said so plainly once they were read side
      by side.
    - It did not fail cleanly, which is why it cost so long. Adjacent 2.4GHz
      channels overlap heavily - 8 and 9 are 5MHz apart and 20MHz wide - so
      short frames still leaked through while long ones did not. The 20 byte
      begin frame crossed, the dock's 9 byte ack crossed back, IR commands
      mostly worked, and only the 250 byte firmware chunks - ten times longer
      on air - never arrived. The link looked alive and only the largest
      transfer failed, which pointed suspicion at everything except the
      channel.
    - The remote does not choose its channel, its router does, so the dock is
      the side that has to follow. Being mains powered it can afford to look;
      the remote, on a battery with a two second radio window, cannot.
    - The trigger is two minutes of silence, not twenty seconds. Under
      on-demand ESP-NOW the remote's radio is off almost all the time, so short
      silences are the normal state and reading them as a fault would leave the
      dock off-channel exactly when the next press arrived - breaking working
      setups in order to fix broken ones.
    - The sweep alternates between the locked channel and each candidate, so
      the dock spends most of it exactly where a working remote expects it. A
      press during a sweep still lands, and if it lands on the locked channel
      the sweep just stops with nothing changed. Only a press that arrives on a
      different channel causes a relock, which is the one case that means
      anything.
    - It stands down during pairing, a firmware transfer or an RF capture - a
      sweep in the middle of any of those would break the thing it is meant to
      protect.

  1.20 - 2026-09-02
    - Real RF433. A CC1101 is now driven over SPI and both halves of the RF
      feature work for the first time: capturing a signal from an existing
      remote, and transmitting it back. The protocol, the remote's API and
      WebConfig's learn flow were all built and waiting - the dock was the
      only stub, answering "no RF hardware is fitted yet".
    - Wiring, on the C3 Super Mini: SCK GPIO4, MISO GPIO5, MOSI GPIO6, CSN
      GPIO7, GDO0 GPIO3, GDO2 GPIO1 (optional), VCC to 3V3 - never 5V, the
      part is 1.8-3.6V. These are the C3's own SPI pins, so the hardware
      peripheral does the work. GPIO2, 8 and 9 are avoided deliberately: all
      three are strapping pins read at reset, and two already carry the LED
      and the button, which stay exactly where they were.
    - The chip runs in asynchronous serial mode rather than its packet engine.
      A gate or garage remote is a bare OOK edge train with no framing the
      CC1101 could parse, so GDO0 carries the raw data and this becomes the
      same problem as raw IR: capture edge timings, replay edge timings.
    - Capture is interrupt driven and IRAM-resident, because an ISR that had to
      fault in from flash would add exactly the jitter it is trying to measure.
      A burst is considered finished after 20ms of silence rather than when the
      window expires, so the answer comes back as soon as the button is
      released. 250 bytes is the ESP-NOW ceiling, so at two bytes a timing only
      121 fit in the reply; longer captures are truncated and say so.
    - Transmit repeats the burst three times with a gap, which is what a real
      remote does - many receivers ignore a single one outright.
    - The module is detected by reading its part and version registers back
      over SPI, not assumed. A missing or miswired CC1101 now says so at boot
      instead of looking identical to a working one until a capture silently
      returns nothing.

  1.19 - 2026-09-02
    - The ack that accepts a transfer is now retried, and repeated while the
      dock waits for the first chunk. It is sent at the worst possible moment
      for a radio send: straight out of esp_ota_begin(), which has just spent
      several seconds erasing a megabyte of flash with everything else starved
      behind it. It was fired and forgotten, so losing that single packet
      stalled the entire update - the remote never learned it could start
      sending, the dock waited out its fifteen second timeout for data nobody
      had been asked for, and both ends then reported the other as broken. The
      logs showed it plainly: the dock accepted the transfer, and the abort ack
      it sent fifteen seconds later arrived at the remote perfectly well, so
      the path was never the problem - only that one packet at that one moment.
    - A stall now reports itself as a stall. It borrowed the write-failed code,
      so a transfer that never delivered a single byte was announced as "the
      dock could not write the image to flash" - pointing at flash that had
      never been written to.

  1.18 - 2026-09-02
    - Fixes "Dock update failed: The dock stopped responding" - the dock was
      responding to nothing, because the busy gate discarded the request. The
      "still transmitting the last IR burst" check sat above the packet
      dispatch and returned for EVERY packet type, not just commands. Anything
      arriving during a burst - a firmware frame, a settings push, a ping, a
      pairing proof - was thrown away as though it were another command. For a
      firmware transfer that is fatal rather than merely lossy: the remote
      sends its begin frame, the dock silently drops it, no ack comes back,
      and the remote reports the dock as unreachable. Nothing was wrong with
      power, range or the radio. The gate is now scoped to IR command frames,
      which is the only thing it was ever about - the dock has one emitter and
      cannot start a second burst while the first is going out. A firmware
      transfer has nothing to do with the emitter.
    - Logs a line when an OTA begin frame arrives, so a capture can prove the
      frame reached the dock whatever happens after it.

  1.17 - 2026-09-01
    - The actual fix for "responsive only while the serial monitor is open".
      1.16 blamed WiFi power save and was wrong - disabling it changed nothing,
      because the dock was never sleeping. This board has no USB-serial bridge,
      so Serial is HWCDC, the C3's own USB Serial/JTAG peripheral, and
      HWCDC::write() WAITS for the host to drain its buffer - up to
      tx_timeout_ms, which defaults to 100. With a monitor attached the host
      drains instantly and writes return at once. With nothing attached there
      is no reader, and every write stalls for the full timeout.
    - The dock prints two lines for every IR command it receives, so with the
      monitor closed each command cost roughly 200ms of dead loop time, and
      ESP-NOW frames arriving during that stall hit the "still working on the
      last one" gate and were dropped. That is the whole of the sluggishness,
      the missed commands, and the apparent loss of range - none of it was the
      radio. Closing the monitor did not put the dock to sleep; it made every
      log line block.
    - Serial.setTxTimeoutMs(0) so a write never waits for a reader that is not
      there, and the two hot-path log lines are now skipped entirely unless a
      host is actually attached - so their cost with nobody listening is zero
      rather than merely small, independent of how the CDC driver behaves.
    - The WiFi power save and TX power settings from 1.16 are kept. They were
      not the cause, but they are still correct for a mains-powered ESP-NOW
      peer that should never duty-cycle its radio.

  1.16 - 2026-09-01
    - Turns off WiFi power save. This is the fix for the dock being responsive
      only while a USB serial monitor was attached. An ESP32 station defaults
      to WIFI_PS_MIN_MODEM, duty-cycling its radio on the assumption that an AP
      is buffering whatever arrives while it sleeps. ESP-NOW has no AP and no
      buffering, so a frame that arrives during a nap is simply lost - which is
      why commands were missed and why range collapsed. An attached USB CDC
      connection inhibits those sleep states, so the radio stayed awake for
      exactly as long as the monitor was open: the "bizarre" behaviour was the
      most useful clue in the whole investigation. The dock is mains powered,
      so power save is disabled outright.
    - States TX power explicitly (78 quarter-dBm units, 19.5 dBm) rather than
      inheriting it, and logs the value the driver actually accepted after
      clamping to the calibration data.
    - Pins the CPU at 160 MHz. Dynamic frequency scaling is the other thing an
      attached USB connection tends to hold up, and IR here is bit-banged at
      microsecond timings, so it wants a stable known clock.

  1.15 - 2026-09-01
    - Counts commands dropped because the previous burst was still going out.
      That gate sits before the log line, so anything lost there never appeared
      anywhere - a log could read "24 received, 24 sent" and look perfect while
      presses were visibly going missing. They are now reported.
    - Repeats the link and LED state every 10 seconds and on every change,
      rather than stating it once at boot. A captured boot log turned up with
      its pairing line missing entirely and a pin number of "GPIO30" where 8
      was printed - USB CDC on this chip overruns during the burst of output
      right after Serial.begin, so anything needed for diagnosis has to be said
      more than once. The line carries paired, linkUp, ledLit, ledOnTx, rf and
      channel together, so the LED's behaviour can be read off directly instead
      of inferred.
    - Waits 600ms rather than 200ms before the boot banner, giving the host time
      to open the port.
    - Casts the pin numbers in the boot line explicitly, which is what produced
      the impossible GPIO30.

  1.14 - 2026-09-01
    - Answers the remote's ping with its firmware version and name, so the
      remote can show what is actually on the other end rather than only that
      something is there. The ping already happens every 5 seconds while the
      link is up, so the reply costs nothing extra and is always current.
    - Replies are queued and sent from loop(), never from the receive callback,
      which runs on the Wi-Fi task.

  1.13 - 2026-09-01
    - The transmit indication no longer blocks. It delayed 240ms inside
      sendCommand, on top of the ~190ms the burst itself takes: 430ms per
      command, against a remote that repeats a held button every 111ms. Holding
      volume would have been visibly worse through the dock than pressing it on
      the remote, purely because of an LED. Nothing about an indicator justifies
      costing more time than the transmission it reports.
    - The LED is now marked "transmitting until" a moment ahead and blinked from
      loop(). A held button extends that window on every repeat, so it simply
      keeps blinking for as long as the stream lasts, and the dock's ceiling
      goes back to being the transmission itself - the same limit the remote's
      own emitter has, since both use the same library and the same timings.
    - Serial now prints the sum of the raw timings beside the measured time, and
      the difference. That answers by measurement what guessing cannot: a close
      match means the IR code is simply that long and only different codes would
      shorten it, while a large gap would mean library overhead worth attacking.

  1.12 - 2026-09-01
    - The LED now mirrors the remote's ESP-NOW radio rather than merely the
      pairing, so it is the dock's counterpart to the green pill outline and
      both ends say the same thing at the same moment. Lit while the radio is
      up, out when it is released.
    - The remote sends an explicit link-down as it releases the radio, which is
      what makes the LED go out at the same instant the pill does instead of
      some seconds later. A 9 second silence timeout backs it up: without one, a
      dock that missed that single packet would sit lit for ever, claiming a
      link that had ended.
    - Any packet at all from the paired remote counts as contact, recorded
      before the packet is even identified - the fact of the packet is the
      signal, whatever it turned out to be.
    - The LED is driven from loop(), never from the receive callback, which runs
      on the Wi-Fi task.
    - Serial now reports how long each IR burst actually took to transmit, in
      milliseconds to two decimals, measured around the transmission alone
      rather than everything surrounding it.

  1.11 - 2026-09-01
    - Fixes the LED never blinking on a command, and the flood of errors that
      came with it. The serial log named it exactly:
        [E][esp32-hal-gpio.c:185] IO 8 is not set as GPIO
      seven per command - one for each pulse step plus the settle. The pulse
      code was running correctly all along and simply never reached the pin.
    - IRremote blinks a feedback LED on every send, and given no pin it uses
      LED_BUILTIN, which this board's variant defines as GPIO8 - the very pin
      the dock LED is on. It drives that pin with direct register writes which
      leave it detached from the GPIO matrix, after which every digitalWrite()
      to it is refused. The collision only began at the first IR command, which
      is why the LED lit correctly at boot and then went dead.
    - IRremote's feedback is now switched off, and the boot line says so along
      with the pin it would have taken.
    - ledWrite() also re-asserts pinMode every time rather than trusting the
      one call at startup. Any library reconfiguring the pin behind our back
      would otherwise swallow every write for the rest of the run; this costs a
      register write and removes the whole class of failure.

  1.10 - 2026-09-01
    - Settles on one channel at pairing and stays there until the next pairing.
      The walk across channels 1-13 exists only to find the remote; once found,
      continuing to hop is exactly what puts the two out of step. The radio is
      now parked explicitly on the agreed channel the moment the remote is
      recognised, rather than being left wherever the sweep happened to have
      reached.
    - A pairing attempt that finds nothing no longer strands a working dock. The
      walk leaves the radio on an arbitrary channel, so a failed re-pairing on
      an already-paired dock would silently break it by parking the radio
      somewhere the remote never transmits - the dock would look paired, and
      never hear another command. It now returns to its remote's channel and
      says so.
    - The channel is the remote's, not an arbitrary fixed number, because it
      cannot be otherwise: while the remote is associated to Wi-Fi its radio is
      on the router's channel and ESP-NOW has to use that. The dock follows the
      remote; that is what keeps them in sync.

  1.09 - 2026-09-01
    - Version fix. 1.06, 1.07 and 1.08 all reported themselves as 1.05, because
      three consecutive version bumps silently did nothing: each searched for
      the previous version number in a line that had already moved on, matched
      nothing, and was not checked. The code changes landed - the binaries do
      contain them - but every one of them announced the wrong version on
      serial, which is precisely the kind of lie that makes debugging
      impossible. A flash reported as 1.08 came back saying 1.05 and looked
      like a failed write.
    - Nothing else changes here. 1.08's LED behaviour is unaltered and was
      already present in the 1.08 binary.

  1.08 - 2026-09-01
    - Fixes the LED doing nothing at all on a received command. 1.07 showed a
      transmission by driving the LED low - a dip out of the lit resting state -
      which is invisible when the dock is not paired, because the resting state
      is already dark. Both ends of the reported symptom, "it never stays on and
      it never flashes", are the same unpaired dock.
    - The transmit indication now inverts from whatever the LED was resting at,
      as a train of fast pulses over 240ms in 40ms steps - the same timing as
      the remote's pill outline, so one event looks like one event at both ends.
      Resting lit it reads as a flicker; resting dark it reads as a flash.
    - The pulse runs around the IR burst rather than during it. IrSender blocks
      for the whole transmission, so there is no way to pulse inside it; what a
      person sees is the LED reacting at the moment the command goes out.
    - Boot logging now says plainly whether the dock is paired, and what that
      means. An unpaired dock has no stored channel either, so it sits on the
      radio default while the remote transmits on its own - nothing arrives and
      the LED stays dark, which looks like broken hardware rather than an
      unpaired dock. Flashing over USB erases both, so this is the expected
      state immediately after a Studio install and the log now says so.

  1.07 - 2026-09-01
    - The LED now rests LIT whenever a remote is paired, and dips while a
      command is transmitting. Previously it rested dark and lit only during a
      transmission, which said nothing about whether the dock was actually
      usable.
    - "Paired" rather than "a live link" is deliberate. The remote brings
      ESP-NOW up only when it needs it, so there is no continuous connection to
      observe - the radio is off most of the time by design. Waiting for
      traffic would leave the LED dark nearly always and flicker it on for a
      second when a command arrived, which is worse than useless. Paired is the
      honest and stable reading of "connected to the remote" from this side.
    - A transmission is a dip out of that resting state, held to at least 120ms
      so it registers: a real IR burst is only tens of milliseconds, too brief
      to read as a deliberate signal. It works whether the dock is paired or
      not, where simply turning the LED on would have been invisible against an
      already-lit one.
    - Forgetting the pairing puts the LED out, since nothing is paired any more.

  1.06 - 2026-09-01
    - Takes its RF433 and LED settings from the remote, which is now the single
      place they are configured. Both are stored in NVS, so a dock behaves the
      same however it is woken rather than reverting to defaults on every
      reboot, and both are accepted only from the paired remote.
    - The LED switch silences the indicator only; the IR still goes out. A dock
      in a dark lounge should be able to stop blinking without also stopping
      working.
    - RF433 commands are gated on the RF switch and report that no RF hardware
      is fitted yet, so the setting means something the day the hardware lands
      instead of being quietly ignored.
    - Ignores the remote's periodic four byte ping. Its only purpose is to draw
      a MAC-layer ack, which is how the remote knows the dock is still there -
      there is nothing to do with the payload itself.

  1.05 - 2026-09-01
    - Receives new firmware over ESP-NOW from the remote, so the dock can be
      updated without unplugging it. The remote pushes 240 byte chunks - the
      largest that fits, since 4 magic + 4 seq + 2 len + 240 is exactly the 250
      byte ESP-NOW ceiling - and waits for an ack on each. Stop-and-wait keeps
      it simple and stops a sender running ahead of a dock busy writing flash.
    - Written straight into the inactive OTA partition through esp_ota_*, so a
      transfer that dies half way leaves the running firmware untouched and the
      dock just carries on with the old image. Nothing is made bootable until a
      CRC32 over the whole image matches the one declared at the start, checked
      before esp_ota_set_boot_partition rather than after.
    - Firmware is accepted from the paired remote and from nothing else. Any
      ESP-NOW device in range can address the dock, and unlike a command - which
      at worst blinks an LED - a firmware push replaces what the dock runs. So
      pairing is a precondition of updating rather than just the usual route to
      it: an unpaired dock cannot be updated over the air at all, only over USB.
    - Out-of-order chunks are handled rather than fatal: a chunk already written
      is re-acked (its ack was lost), and a chunk from too far ahead is answered
      with the sequence actually wanted, so the sender rewinds instead of
      leaving a hole in the image. A sender that vanishes is given 15 seconds
      before the transfer is abandoned.
    - Carries a literal OPENREMOTE_DOCK_VERSION= marker in the built image, the
      counterpart to the remote's OPENREMOTE_FIRMWARE_VERSION=. Two different
      strings on two different chips is what lets Studio refuse to write remote
      firmware into a dock, or dock firmware into a remote, from the file alone.
    - LED vocabulary, chosen so the three rates cannot be confused:
        slow   500ms  firmware transfer in progress
        medium 120ms  pairing window open
        fast    45ms  something failed - pairing or firmware alike
        solid         paired (10s), transmitting IR (the burst), or about to
                      reboot into new firmware (1.5s)
      Failure looks the same whatever failed, on purpose: from across the room
      what matters is that it did, and serial says which.
    - Three button gestures instead of one:
        tap (under 1s)  identify - three quick blinks, to pick one dock out of
                        several without disturbing its pairing
        hold 5s         open the pairing window
        hold 10s        forget the paired remote entirely
      Each fires once per press and the longer supersedes the shorter, so
      holding past five seconds closes the pairing window it just opened, which
      is what holding that long means.

  1.04 - 2026-09-01
    - Drives a real IR emitter on GPIO10, alongside the indicator LED. An
      SFH4346 - the same part the remote uses - straight off the pin through a
      series resistor: about 20mA at 100 ohm or 29mA at 68 ohm, against the
      C3's 40mA per-pin maximum. That is far below the several hundred
      milliamps a real blaster pulses, so the range is a metre or two rather
      than a room, which is the price of using no transistor.
    - The two LEDs deliberately get different signals. The emitter gets a
      properly modulated 38kHz carrier from IRremote, because a television's
      receiver demodulates the carrier and ignores a bare on/off envelope
      completely - flashing the emitter the way the indicator flashes would
      look convincing on a phone camera and do nothing whatsoever to a TV. The
      indicator is simply lit for the duration of the transmission, which is
      what "at the same time" can mean when one signal is modulated and the
      other cannot be.
    - The protocol dispatch mirrors transmitIrCommand() in the remote firmware
      and uses the same IRremote version, so anything the remote can send the
      dock can send: raw timings, NEC, NECext/NEC1 via Onkyo, Samsung32, RC5,
      RC5X, RC6 and the SIRC family. An unsupported protocol says so and holds
      the indicator briefly, so it cannot be mistaken for a successful send.
    - Set DOCK_IR_LED_PIN to -1 to disable the emitter and keep only the
      indicator.

  1.03 - 2026-09-01
    - Retargeted from the ESP32-S3 Super Mini to the ESP32-C3 Super Mini, which
      is the board actually being used. Different chip, not just a different
      pin: single-core RISC-V, and a pinout with nothing in common with the S3
      board. LED moves from GPIO48 to GPIO8, button from GPIO0 to GPIO9.
    - The LED is driven ACTIVE LOW here. The C3 Super Mini sits its built-in LED
      anode at 3V3 and sinks it through the pin, so LOW lights it - the opposite
      of the S3 board. Left at the S3's polarity every indication would have
      been exactly inverted: dark while pairing, lit when idle.
    - Nothing else changed. The ESP-NOW protocol, the pairing state machine and
      the IR envelope replay are chip-independent, and the firmware was already
      single-threaded so the move to one core costs nothing.

  1.02 - 2026-09-01
    - The board in hand has a plain blue LED on GPIO48, not a WS2812. The
      vendor's pin diagram shows a WS2818 RGB there and espboards.dev describes
      both sharing the pin, but the Super Mini ships in more than one form and
      the physical board wins. All WS2812 handling is removed.
    - This is the better hardware for the job, not a compromise. A plain LED can
      be switched at IR timings, so every command flash is now a real IR
      envelope - high for every mark, low for every space, at true microsecond
      timings - where 1.01 could only manage that by borrowing the pin back
      from the WS2812 driver and clearing it afterwards. That handover is gone.
    - The three states are told apart by blink pattern alone, which is what was
      asked for in the first place: blinking while pairing, solid for ten
      seconds once paired, much faster blinking on failure.
    - DOCK_LED_ACTIVE_LOW returns as a build flag. These boards wire the LED
      either way round and it cannot be detected in software; if the LED is lit
      when it should be dark, set it to 1 in platformio.ini.

  1.01 - 2026-09-01
    - Board confirmed from the vendor pin diagram and espboards.dev, so the
      guesswork in 1.00 is gone: ESP32S3FH4R2, 4MB flash, WS2812 on GPIO48
      ("WS2818 RGB LED - GP48 | DIN") with a plain LED sharing the same pin and
      unable to be used independently of it, BOOT on GPIO0.
    - That shared pin turns out to be an advantage. The pairing indications
      drive GPIO48 as a WS2812, which gives colour for nothing - blue while the
      window is open, green for the ten second paired hold, red for the failure
      burst - while an IR command drives the very same pin as a plain output at
      true microsecond timings. The envelope on the wire is therefore the real
      one, high for every mark and low for every space, exactly as an IR LED
      would be driven. 1.00 could only do one or the other.
    - After a burst the pin is handed back to the WS2812, which has just been
      fed a stream that is not valid data, so it is explicitly cleared rather
      than left to latch whatever it made of it.
    - DOCK_LED_IS_RGB and DOCK_LED_ACTIVE_LOW are gone. They existed to cover
      not knowing the board; the board is now known.

  1.00 - 2026-09-01
    - First release. Target is an ESP32-S3 Super Mini living in the OpenRemote
      dock. It is a real ESP-NOW peer of the remote, speaking the wire format
      the remote firmware already implements - not a simulation:
        announce   dock -> remote broadcast, magic "OREN" + 24 byte name
        command    remote -> dock, magic "ORCM" + EspNowCommandHeader
        RF learn   remote -> dock "ORLS", dock -> remote "ORLR"
      The structs below are byte-for-byte copies of the ones in
      OpenRemote_1.0.ino. They are packed and must stay that way; any change
      on one side has to be mirrored on the other or the two stop
      understanding each other silently.
    - Pairing: hold the onboard button for 5 seconds. The LED blinks fast
      while the pairing window is open and the dock broadcasts its announce
      packet. Paired -> LED solid for 10 seconds, then off. Timed out or
      failed -> a burst of much faster blinking, then off.
    - Channel discovery. The remote registers its peers with channel 0, which
      means "whatever channel the station connection is already on" - the
      router's channel, which the dock has no way to know in advance. So while
      the pairing window is open the dock walks channels 1-13, broadcasting on
      each, and the remote hears it when the walk reaches the remote's own
      channel. The channel the remote is eventually heard on is locked and
      saved to NVS, so a reboot goes straight back to it instead of walking
      again.
    - Pairing confirmation. The remote's current firmware stores a newly paired
      dock silently - it sends nothing back - so the dock cannot be told "you
      are paired" by any packet that exists today. It therefore treats the
      first valid packet addressed to it from a remote as confirmation, which
      is sound: the remote only ever unicasts to a peer it has already added.
      A dedicated ack ("ORPA") is also accepted, ready for a matching change on
      the remote side; until that exists, confirmation arrives with the first
      command rather than at the moment of pairing.
    - IR commands only flash the onboard LED for now, there being no IR
      hardware yet. With a plain LED the flash is the real thing: the decoded
      envelope is replayed at its true microsecond timings, so the LED is
      driven exactly as an IR LED would be. A WS2812 cannot be switched at
      those rates - each pixel update alone costs about 30us - so in RGB mode
      the flash keeps the signal's real duration but not its internal
      structure. Set DOCK_LED_IS_RGB to match the board.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <IRremote.hpp>
#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <HTTPClient.h>
#if DOCK_RF_CS_PIN >= 0
// Asynchronous serial mode, not the packet engine: a gate or garage remote is
// a raw OOK edge train with no framing the CC1101 could parse for us. GDO0
// carries the bare data both ways, which makes this the same problem as raw
// IR - capture the edge timings, replay the edge timings.
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#endif

// True only when a host is actually reading the USB serial port.
//
// Belt and braces alongside Serial.setTxTimeoutMs(0): the command path must
// not depend on how the CDC driver behaves with no reader attached. Nothing
// is formatted and nothing is written when nobody is listening, so the cost
// of logging in the hot path is not "small", it is zero. On a UART build
// there is no host to detect and no stall to avoid, so it is always true.
static inline bool serialHostAttached() {
#if ARDUINO_USB_CDC_ON_BOOT
  return (bool)Serial;
#else
  return true;
#endif
}


#define OPENREMOTE_DOCK_VERSION_STRING "1.33"

// A literal in the built image, so a tool holding the .bin can tell what it is
// without running it. The remote firmware carries the same idea under
// OPENREMOTE_FIRMWARE_VERSION=, and the two markers are deliberately different
// strings: that is what lets OpenRemote Studio refuse to write remote firmware
// into a dock, or dock firmware into a remote, from the file alone. The
// concatenation means the marker cannot drift from the version above.
static constexpr char OPENREMOTE_DOCK_MARKER[] =
  "OPENREMOTE_DOCK_VERSION=" OPENREMOTE_DOCK_VERSION_STRING;

// ---------------------------------------------------------------------------
// Board - ESP32-C3 Super Mini
// ---------------------------------------------------------------------------
//
// A C3, not an S3: single-core RISC-V, 4MB flash, 400KB SRAM, no PSRAM. None
// of that matters here - the firmware is small, single-threaded and uses about
// 45KB - but the pins are completely different from the S3 board this started
// on, so nothing about the pinout carries over.
//
//   GPIO8  built-in blue LED       GPIO1  PCB D5 status LED
//   GPIO9  BOOT button             GPIO3  PCB SW1 menu button
//   GPIO0  PCB IR MOSFET gate      GPIO10/GPIO20  CC1101 GDO0/GDO2
//
// The LED is a plain single-colour one and is driven ACTIVE LOW on this board:
// the anode sits at 3V3 and the pin sinks it, so LOW lights it. That is the
// usual wiring for the C3 Super Mini and the opposite of the S3 board's, which
// is exactly the sort of thing that silently inverts every indication if it is
// assumed rather than set.
//
// A plain LED is the right hardware for this job. It can be switched at IR
// timings, which is what makes a command flash a real IR envelope rather than
// an impression of one.

#ifndef DOCK_LED_PIN
#define DOCK_LED_PIN 8
#endif

// Optional second status LED on the dock PCB. It mirrors the onboard LED but
// has its own polarity because D5 is wired GPIO -> resistor -> LED -> GND.
#ifndef DOCK_AUX_LED_PIN
#define DOCK_AUX_LED_PIN -1
#endif

// BOOT. Held low when pressed, released high by the pull-up.
#ifndef DOCK_BUTTON_PIN
#define DOCK_BUTTON_PIN 9
#endif

// Optional second active-low button. Either button drives the same gesture
// state machine; holding both is still one logical press.
#ifndef DOCK_MENU_BUTTON_PIN
#define DOCK_MENU_BUTTON_PIN -1
#endif

// If the LED is lit when it should be dark, flip this in platformio.ini.
// It cannot be detected in software.
#ifndef DOCK_LED_ACTIVE_LOW
#define DOCK_LED_ACTIVE_LOW 1
#endif

#ifndef DOCK_AUX_LED_ACTIVE_LOW
#define DOCK_AUX_LED_ACTIVE_LOW 0
#endif

// Active-high IR drive. On Rev 6 this is the gate signal for the low-side
// FS8205A stage rather than LED current from the GPIO itself. Set to -1 to
// disable the emitter and leave only the status LEDs.
#ifndef DOCK_IR_LED_PIN
#define DOCK_IR_LED_PIN 10
#endif

// ---------------------------------------------------------------------------
// Timings, all from the specification for this firmware
// ---------------------------------------------------------------------------

static const uint32_t PAIR_HOLD_MS            = 5000;   // Hold to start pairing.
static const uint32_t PAIR_WINDOW_MS          = 30000;  // Then give up.
static const uint32_t PAIR_BLINK_MS           = 120;    // "Pairing" blink.
static const uint32_t PAIR_FAIL_BLINK_MS      = 45;     // "Failed" blink, faster.
static const uint32_t PAIR_FAIL_DURATION_MS   = 2500;   // How long it sulks.
static const uint32_t PAIR_SUCCESS_HOLD_MS    = 10000;  // Solid after pairing.
static const uint32_t ANNOUNCE_INTERVAL_MS    = 150;    // Broadcast rate.
static const uint32_t OTA_BLINK_MS           = 500;    // "Receiving firmware".
static const uint32_t OTA_SUCCESS_HOLD_MS    = 1500;   // Solid, then reboot.
static const uint32_t OTA_FAIL_DURATION_MS   = 3000;   // Fast blink, then idle.
static const uint32_t OTA_STALL_TIMEOUT_MS   = 15000;  // No chunk for this long.
static const uint32_t IDENTIFY_BLINKS        = 3;      // Short press feedback.
static const uint32_t FORGET_HOLD_MS         = 10000;  // Hold to unpair.
static const uint32_t CHANNEL_DWELL_MS        = 450;    // Per channel while walking.
static const uint8_t  CHANNEL_MIN             = 1;
static const uint8_t  CHANNEL_MAX             = 13;

// ---------------------------------------------------------------------------
// Wire format - must match OpenRemote_1.0.ino exactly
// ---------------------------------------------------------------------------

static const uint32_t ESPNOW_ANNOUNCE_MAGIC          = 0x4F52454EUL;  // "OREN"
static const uint32_t ESPNOW_COMMAND_MAGIC           = 0x4F52434DUL;  // "ORCM"
static const uint32_t ESPNOW_RF_LEARN_START_MAGIC    = 0x4F524C53UL;  // "ORLS"
static const uint32_t ESPNOW_RF_LEARN_RESULT_MAGIC   = 0x4F524C52UL;  // "ORLR"
// Not yet sent by any remote firmware. Accepted so that adding it on the
// remote side later needs no change here. See the 1.00 changelog note.
static const uint32_t ESPNOW_PAIR_ACK_MAGIC          = 0x4F525041UL;  // "ORPA"

// Firmware transfer, remote -> dock, with the dock acking every chunk.
static const uint32_t ESPNOW_OTA_BEGIN_MAGIC = 0x4F524F42UL;  // "OROB"
static const uint32_t ESPNOW_OTA_DATA_MAGIC  = 0x4F524F44UL;  // "OROD"
static const uint32_t ESPNOW_OTA_END_MAGIC   = 0x4F524F45UL;  // "OROE"
static const uint32_t ESPNOW_OTA_ACK_MAGIC   = 0x4F524F41UL;  // "OROA"

// 240 payload bytes is the largest that fits: 4 magic + 4 seq + 2 len + 240 is
// exactly ESP-NOW's 250 byte ceiling.
// 160, not 240. A 240 byte chunk plus its 10 byte header is exactly 250, which
// is ESP_NOW_MAX_DATA_LEN - the very top of what ESP-NOW will carry - and at
// that size the frame was being acknowledged by the receiving radio and then
// discarded by the receiving ESP-NOW stack before any callback ran. From the
// sending side it looked like a perfect send into silence, which is why it
// survived so many rounds of looking elsewhere.
//
// Measured on this link rather than guessed: a 5 byte ping, a 9 byte ack, a 20
// byte OTA begin and a 179 byte raw IR command all cross reliably, and only the
// 250 byte chunk never arrived. 160 + 10 = 170 sits inside everything proven to
// work. It costs about 35% more chunks on a transfer that happens rarely, which
// is a trade worth making for one that completes.
static const uint16_t ESPNOW_OTA_CHUNK_BYTES = 160;

struct __attribute__((packed)) EspNowOtaBeginPacket {
  uint32_t magic;
  uint32_t totalBytes;
  uint32_t crc32;
  char version[8];
};

struct __attribute__((packed)) EspNowOtaDataHeader {
  uint32_t magic;
  uint32_t seq;
  uint16_t len;
};

struct __attribute__((packed)) EspNowOtaEndPacket {
  uint32_t magic;
  uint32_t crc32;
};

// status: 0 ok, 1 begin refused, 2 write failed, 3 verify failed, 4 complete.
// seq is the next sequence the dock expects, so a sender that gets ahead is
// told exactly where to resume rather than having to guess.
struct __attribute__((packed)) EspNowOtaAckPacket {
  uint32_t magic;
  uint32_t seq;
  uint8_t status;
};

static const uint8_t OTA_ACK_OK = 0;
static const uint8_t OTA_ACK_BEGIN_REFUSED = 1;
static const uint8_t OTA_ACK_WRITE_FAILED = 2;
static const uint8_t OTA_ACK_VERIFY_FAILED = 3;
static const uint8_t OTA_ACK_COMPLETE = 4;
// Distinct from WRITE_FAILED, which the stall used to borrow - and which told
// the user the flash had failed when nothing had ever been written to it.
static const uint8_t OTA_ACK_STALLED = 5;

// Settings pushed by the remote, so the dock behaves the same however it is
// woken and the remote stays the single place they are configured.
static const uint32_t ESPNOW_DOCK_SETTINGS_MAGIC = 0x4F524453UL;  // "ORDS"
// A four byte nudge the remote sends purely to draw a MAC-layer ack, which is
// how it knows the dock is still there. Nothing to do but ignore it.
static const uint32_t ESPNOW_DOCK_PING_MAGIC     = 0x4F525047UL;  // "ORPG"
// The remote sends this as it releases the radio, so the LED can go out at the
// same moment the remote's pill outline does.
static const uint32_t ESPNOW_DOCK_LINK_DOWN_MAGIC = 0x4F524C44UL;  // "ORLD"
// Dock -> remote, in reply to a ping. The remote cannot know what firmware the
// dock is running or what it calls itself unless the dock says so, and a ping
// is already happening every 5 seconds while the link is up - so the answer
// costs nothing extra and is always current.
static const uint32_t ESPNOW_DOCK_INFO_MAGIC = 0x4F524449UL;  // "ORDI"
// Homebridge relayed through this dock instead of the remote. Byte-identical
// to the remote's definitions - the static_asserts below fail the build if
// either side drifts.
static const uint32_t ESPNOW_DOCK_WIFI_MAGIC   = 0x4F525743UL;  // "ORWC"
static const uint32_t ESPNOW_DOCK_HBCFG_MAGIC  = 0x4F524843UL;  // "ORHC"
static const uint32_t ESPNOW_HOMEBRIDGE_MAGIC  = 0x4F524842UL;  // "ORHB"
static const uint32_t ESPNOW_HOMEBRIDGE_RESULT_MAGIC = 0x4F524852UL;  // "ORHR"

struct __attribute__((packed)) EspNowDockInfoPacket {
  uint32_t magic;
  char version[8];
  char name[24];
};
static_assert(sizeof(EspNowDockInfoPacket) == 36, "dock info layout drifted from the remote");

struct __attribute__((packed)) EspNowDockSettingsPacket {
  uint32_t magic;
  uint8_t rfEnabled;
  uint8_t ledOnTransmit;
  uint8_t reserved[2];
};
static_assert(sizeof(EspNowDockSettingsPacket) == 8, "dock settings layout drifted from the remote");

struct __attribute__((packed)) EspNowAnnouncePacket {
  uint32_t magic;
  char name[24];
};

struct __attribute__((packed)) EspNowCommandHeader {
  uint32_t magic;
  uint8_t transport;
  uint8_t encoding;  // 0 = PARSED (protocol/address/command), 1 = RAW (timings)
  uint16_t frequencyKhz;
  uint32_t address;
  uint32_t command;
  uint8_t sonyBits;
  char protocol[16];
  uint16_t rawCount;
};


struct __attribute__((packed)) EspNowDockWifiPacket {
  uint32_t magic;
  char ssid[33];
  char password[65];
};

struct __attribute__((packed)) EspNowDockHomebridgePacket {
  uint32_t magic;
  char address[65];
  char username[33];
  char password[65];
};

struct __attribute__((packed)) EspNowHomebridgePacket {
  uint32_t magic;
  uint8_t operation;    // 0 = set, 1 = toggle, 2 = step
  uint8_t valueType;    // 0 = string, 1 = bool, 2 = number
  float value;
  float step;
  float minimum;
  float maximum;
  char accessoryId[72];
  char characteristic[40];
  char stringValue[28];
};

struct __attribute__((packed)) EspNowHomebridgeResultPacket {
  uint32_t magic;
  uint8_t ok;
  int16_t httpStatus;
  char error[64];
};

struct __attribute__((packed)) EspNowRfLearnStartPacket {
  uint32_t magic;
  uint32_t timeoutMs;
};

struct __attribute__((packed)) EspNowRfLearnResultHeader {
  uint32_t magic;
  uint8_t ok;
  uint16_t rawCount;
};

// If either side ever gains, loses or reorders a field, these fail the build
// here instead of the two firmwares quietly misreading each other's packets on
// air - which would look like "the dock ignores commands" and cost a day.
static_assert(sizeof(EspNowAnnouncePacket) == 28, "announce packet layout drifted from the remote");
static_assert(sizeof(EspNowCommandHeader) == 35, "command header layout drifted from the remote");
static_assert(sizeof(EspNowRfLearnStartPacket) == 8, "RF learn start layout drifted from the remote");
static_assert(sizeof(EspNowDockWifiPacket) == 102, "dock Wi-Fi config layout drifted from the remote");
static_assert(sizeof(EspNowDockHomebridgePacket) == 167, "dock Homebridge config layout drifted from the remote");
static_assert(sizeof(EspNowHomebridgePacket) == 162, "Homebridge command layout drifted from the remote");
static_assert(sizeof(EspNowHomebridgeResultPacket) == 71, "Homebridge result layout drifted from the remote");
static_assert(sizeof(EspNowRfLearnResultHeader) == 7, "RF learn result layout drifted from the remote");
static_assert(sizeof(EspNowOtaBeginPacket) == 20, "OTA begin layout drifted from the remote");
static_assert(sizeof(EspNowOtaDataHeader) == 10, "OTA data header layout drifted from the remote");
static_assert(sizeof(EspNowOtaEndPacket) == 8, "OTA end layout drifted from the remote");
static_assert(sizeof(EspNowOtaAckPacket) == 9, "OTA ack layout drifted from the remote");
static_assert(sizeof(EspNowOtaDataHeader) + ESPNOW_OTA_CHUNK_BYTES == 170,
              "OTA data frame must exactly fill the ESP-NOW payload");

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const size_t ESPNOW_MAX_PAYLOAD_BYTES = 250;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

// LED vocabulary, chosen so the three blink rates cannot be confused:
//
//   slow   500ms  firmware transfer in progress
//   medium 120ms  pairing window open
//   fast    45ms  something failed - pairing or firmware, the meaning is the
//                 same either way
//   solid          paired (10s), or transmitting IR (the length of the burst)
//
// Failure always looks the same on purpose. A person across the room needs to
// know something went wrong, not which subsystem it was; serial says which.
enum DockState : uint8_t {
  DOCK_IDLE = 0,      // Paired or not, waiting. LED off.
  DOCK_PAIRING,       // Window open, announcing, LED blinking at 120ms.
  DOCK_PAIR_SUCCESS,  // LED solid for PAIR_SUCCESS_HOLD_MS.
  DOCK_PAIR_FAILED,   // LED blinking at 45ms for PAIR_FAIL_DURATION_MS.
  DOCK_OTA,           // Receiving firmware. LED blinking at 500ms.
  DOCK_OTA_SUCCESS,   // LED solid, then reboot into the new image.
  DOCK_OTA_FAILED,    // LED blinking at 45ms, then back to idle on the old image.
};

DockState dockState = DOCK_IDLE;
unsigned long dockStateSinceMs = 0;

char dockName[24] = "OpenRemote Dock";

// Defaults chosen so a dock that has never been told otherwise behaves
// usefully: it transmits, and it shows that it is transmitting.
bool dockRfEnabled = true;

// ---------------------------------------------------------------------------
// CC1101 433MHz
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Homebridge relay
// ---------------------------------------------------------------------------
// The remote can hand Homebridge commands to this dock instead of issuing them
// itself. It is worth doing because the dock is mains powered: it holds its
// Wi-Fi association open permanently, so a command is one HTTP round trip on an
// already-warm connection, where the remote has to power a radio and associate
// first - seconds, every single time.
//
// The dock logs in and keeps its own token. The remote never sends one, so
// there is no shared expiry to keep in step.
String hbSsid, hbPassword, hbAddress, hbUser, hbPass, hbToken;
bool wifiConfigured = false;
bool wifiJoined = false;
unsigned long wifiNextAttemptMs = 0;
unsigned long wifiPsAssertMs = 0;
static const uint32_t WIFI_RETRY_MS = 20000;

volatile bool pendingHomebridge = false;
volatile bool pendingWifiConfigReady = false;
volatile bool pendingHbConfigReady = false;
EspNowDockWifiPacket pendingWifiConfig;
EspNowDockHomebridgePacket pendingHbConfig;
EspNowHomebridgePacket pendingHomebridgeCommand;
uint8_t pendingHomebridgeMac[6] = {0};

bool rfPresent = false;
// 250 bytes is the ESP-NOW ceiling and the result header eats 7, so at two
// bytes a timing only 121 can ever be sent back. Capturing more than that
// still helps: it lets a too-long burst be recognised as too long rather than
// silently truncated into something that would never replay.
static const uint16_t RF_CAPTURE_MAX = 300;
static const uint16_t RF_SEND_MAX = (250 - sizeof(EspNowRfLearnResultHeader)) / 2;
static const uint16_t RF_MIN_EDGES = 16;      // Fewer than this is noise, not a remote.
static const uint32_t RF_END_GAP_US = 20000;  // Silence that means the burst ended.
static const uint32_t RF_MAX_EDGE_US = 60000; // Clamp: uint16_t cannot hold more.

volatile uint16_t rfEdges[RF_CAPTURE_MAX];
volatile uint16_t rfEdgeCount = 0;
volatile uint32_t rfLastEdgeUs = 0;
volatile bool rfCapturing = false;
bool rfLearnActive = false;
unsigned long rfLearnEndsMs = 0;
uint8_t rfLearnMac[6] = {0};
volatile bool pendingRfLearn = false;
volatile uint32_t pendingRfLearnMs = 0;

#if DOCK_RF_CS_PIN >= 0
bool rfInit();
void rfIdle();
void rfStartCapture(uint32_t timeoutMs);
bool rfTransmitRaw(const uint16_t *timings, uint16_t count);
#endif
bool dockLedOnTransmit = true;

bool remoteKnown = false;
// Set from the receive callback on any packet from the paired remote, and
// cleared when it announces it is going away.
volatile unsigned long lastRemoteContactMs = 0;
// Set from the Wi-Fi task when the remote tells us it has moved; acted on in
// loop(), because relocking writes NVS and retunes the radio.
volatile uint8_t pendingChannelMove = 0;
// Two counters that separate the only remaining explanations for "no data yet".
// Either the chunks are not arriving at all - a radio or addressing problem -
// or they are arriving and this firmware is throwing them away. Guessing
// between those has already cost several rounds; counting settles it.
volatile uint32_t otaFramesSeen = 0;        // Anything at all from the remote mid-transfer.
volatile uint32_t otaFramesDroppedBusy = 0; // OTA frames refused by the pendingOta gate.
volatile bool remoteLinkDown = true;
uint8_t remoteMac[6] = {0};
uint8_t lockedChannel = 0;        // 0 = not yet known.
uint8_t walkChannel = CHANNEL_MIN;

unsigned long lastChannelHopMs = 0;
unsigned long lastAnnounceMs = 0;

// Button.
bool buttonDown = false;
unsigned long buttonDownAtMs = 0;
uint8_t buttonHoldConsumed = 0;  // 0 none, 1 pairing fired, 2 forget fired.

// LED.
bool ledOn = false;
unsigned long lastBlinkMs = 0;

// Set from the ESP-NOW receive callback, acted on in loop(). The callback runs
// on the Wi-Fi task, so it does no more than copy the frame and raise a flag -
// replaying an IR envelope or writing NVS from in there would be driving the
// stack from its own callback, which is exactly the class of bug that bit the
// remote firmware repeatedly.
volatile bool pendingCommand = false;
volatile bool pendingPairProof = false;
uint8_t pendingSrcMac[6] = {0};
uint8_t pendingChannel = 0;
EspNowCommandHeader pendingHeader = {};
uint16_t pendingTimings[120];
uint16_t pendingTimingCount = 0;

// Firmware transfer state. Only the receive callback and loop() touch these,
// and the callback does nothing but copy a chunk and raise a flag - writing
// flash from inside the Wi-Fi task's callback would block it for milliseconds
// at a time.
bool otaActive = false;
esp_ota_handle_t otaHandle = 0;
const esp_partition_t *otaPartition = nullptr;
uint32_t otaExpectedSeq = 0;
uint32_t otaTotalBytes = 0;
uint32_t otaWrittenBytes = 0;
uint32_t otaExpectedCrc = 0;
uint32_t otaRunningCrc = 0;
unsigned long otaLastChunkMs = 0;

volatile bool pendingOta = false;      // A chunk or control packet is waiting.
// Applied in loop(): the receive callback runs on the Wi-Fi task and NVS writes
// do not belong there.
volatile bool ledStateDirty = false;
volatile bool pendingInfoReply = false;
volatile uint32_t commandsDroppedBusy = 0;
unsigned long ledTxUntilMs = 0;
volatile bool pendingSettings = false;
volatile bool pendingSettingsRf = true;
volatile bool pendingSettingsLed = true;
uint8_t otaFrame[250];
volatile uint16_t otaFrameLen = 0;

Preferences prefs;

void enterState(DockState next);
String macToString(const uint8_t mac[6]);

// ---------------------------------------------------------------------------
// LED
// ---------------------------------------------------------------------------

void ledWrite(bool on) {
  ledOn = on;
  // Re-asserted every time rather than once at startup. Any library that
  // reconfigures this pin behind our back - IRremote's feedback LED did
  // exactly that - would otherwise silently swallow every write for the rest
  // of the run. It costs a register write and removes a whole class of
  // failure where the LED code is correct and simply never reaches the pin.
  pinMode(DOCK_LED_PIN, OUTPUT);
  digitalWrite(DOCK_LED_PIN, (on != (bool)DOCK_LED_ACTIVE_LOW) ? HIGH : LOW);
#if DOCK_AUX_LED_PIN >= 0
  pinMode(DOCK_AUX_LED_PIN, OUTPUT);
  digitalWrite(DOCK_AUX_LED_PIN,
               (on != (bool)DOCK_AUX_LED_ACTIVE_LOW) ? HIGH : LOW);
#endif
}

void ledSetup() {
  pinMode(DOCK_LED_PIN, OUTPUT);
#if DOCK_AUX_LED_PIN >= 0
  pinMode(DOCK_AUX_LED_PIN, OUTPUT);
#endif
  ledWrite(false);
}

// The resting state: lit whenever this dock has a remote paired to it.
//
// "Paired" rather than "a live link", because with the remote bringing ESP-NOW
// up only when it needs it there is no continuous connection for the dock to
// observe - the radio is off most of the time by design. Waiting for traffic
// would leave the LED dark almost always and flicker it on for a second when a
// command arrived, which says less than nothing. Paired is the honest and
// stable reading of "connected to the remote" from this side.
// Lit while the remote's ESP-NOW radio is up - the dock's counterpart to the
// green pill outline, so both ends say the same thing at the same time.
//
// The remote pings every 5 seconds while its radio is up and sends an explicit
// link-down as it releases, so this is normally exact. The silence timeout is
// the fallback for a link-down that never arrived: without it a dock that
// missed one packet would sit lit for ever, claiming a link that ended.
static const uint32_t REMOTE_LINK_IDLE_MS = 9000;

bool remoteLinkUp() {
  if (!remoteKnown || !lastRemoteContactMs) return false;
  if (remoteLinkDown) return false;
  return (millis() - lastRemoteContactMs) < REMOTE_LINK_IDLE_MS;
}

bool ledIdleShouldBeLit() {
  return remoteLinkUp();
}

void applyIdleLed() {
  ledWrite(ledIdleShouldBeLit());
}

// Blinks at a fixed period without blocking, so the button and the radio keep
// being serviced while the LED is doing its job.
void ledBlink(unsigned long now, uint32_t periodMs) {
  if (now - lastBlinkMs < periodMs) return;
  lastBlinkMs = now;
  ledWrite(!ledOn);
}

// ---------------------------------------------------------------------------
// IR envelope replay
// ---------------------------------------------------------------------------
//
// With a plain LED this is the real thing: the LED is switched on for every
// mark and off for every space, at the signal's true microsecond timings, so
// it is driven exactly as an IR LED would be. The whole burst lasts tens of
// milliseconds, so what a person sees is one short flash - which is correct,
// and is what an IR LED does too.
//
// A WS2812 cannot follow that. Each pixel update is itself about 30us of
// bit-banged signal with a reset gap after it, so marks shorter than that
// cannot be represented at all. In RGB mode the flash therefore preserves the
// signal's total duration but not its internal structure, with a floor so a
// short code is still visible.

// Sends the command on the IR emitter for real, with the indicator LED lit for
// exactly as long as the transmission lasts.
//
// The two LEDs are given deliberately different signals. The emitter gets a
// properly modulated carrier from IRremote, because a television's receiver
// demodulates 38kHz and ignores a bare on/off envelope entirely - flashing the
// emitter the way the indicator flashes would look right on a camera and do
// nothing at all to a TV. The indicator just goes on for the duration, which
// is what "flashes at the same time" actually means when one signal is
// modulated and the other cannot be.
//
// The dispatch below mirrors transmitIrCommand() in the remote firmware, using
// the same IRremote version, so a command the remote can send is a command the
// dock can send.
bool transmitIrInner(const EspNowCommandHeader &header, const uint16_t *timings,
                     uint16_t count);

// Times the transmission itself, so what is reported is the air time of the
// burst rather than everything around it.
bool transmitIr(const EspNowCommandHeader &header, const uint16_t *timings,
                uint16_t count) {
  // The sum of the timings is what the signal is *supposed* to take. Printing
  // it beside the measured time answers a question that guessing cannot: a
  // close match means the code is simply that long and the only way to shorten
  // it is to change the code, while a large gap would mean the library is
  // adding overhead worth attacking.
  uint32_t expectedUs = 0;
  if (header.encoding == 1 && timings) {
    for (uint16_t i = 0; i < count; i++) expectedUs += timings[i];
  }

  uint32_t began = micros();
  bool sent = transmitIrInner(header, timings, count);
  uint32_t tookUs = micros() - began;
  if (sent && serialHostAttached()) {
    Serial.printf("Dock: IR sent in %lu.%02lu ms (signal is %lu.%02lu ms, "
                  "overhead %ld us, %u kHz)\n",
                  (unsigned long)(tookUs / 1000UL),
                  (unsigned long)((tookUs % 1000UL) / 10UL),
                  (unsigned long)(expectedUs / 1000UL),
                  (unsigned long)((expectedUs % 1000UL) / 10UL),
                  (long)tookUs - (long)expectedUs,
                  (unsigned)(header.frequencyKhz ? header.frequencyKhz : 38));
  }
  return sent;
}

bool transmitIrInner(const EspNowCommandHeader &header, const uint16_t *timings,
                     uint16_t count) {
#if DOCK_IR_LED_PIN < 0
  (void)header; (void)timings; (void)count;
  return false;
#else
  uint16_t khz = header.frequencyKhz ? header.frequencyKhz : 38;

  if (header.transport == 1) {
    if (!dockRfEnabled) {
      Serial.println("Dock: RF433 command ignored - RF is switched off");
      return false;
    }
#if DOCK_RF_CS_PIN >= 0
    if (!rfPresent) {
      Serial.println("Dock: RF433 command ignored - no CC1101 answered on the SPI pins");
      return false;
    }
    // Only raw makes sense here. A "parsed" RF command would mean the remote
    // had decoded some protocol it could re-encode, and nothing in the chain
    // does that - a 433MHz capture is an edge train, start to finish.
    if (header.encoding != 1 || !timings || !count) {
      Serial.println("Dock: RF433 command ignored - only raw captures can be replayed");
      return false;
    }
    return rfTransmitRaw(timings, count);
#else
    Serial.println("Dock: RF433 command ignored - RF pins are disabled in this build");
    return false;
#endif
  }

  if (header.encoding == 1) {
    if (!timings || !count) return false;
    IrSender.sendRaw(timings, count, khz);
    return true;
  }

  if (strcmp(header.protocol, "NEC") == 0) {
    IrSender.sendNEC((uint16_t)header.address, (uint16_t)header.command, 0);
  } else if (strcmp(header.protocol, "NECext") == 0 ||
             strcmp(header.protocol, "NEC1") == 0) {
    IrSender.sendOnkyo((uint16_t)header.address, (uint16_t)header.command, 0);
  } else if (strcmp(header.protocol, "Samsung32") == 0) {
    IrSender.sendSamsung((uint16_t)header.address, (uint16_t)header.command, 0);
  } else if (strcmp(header.protocol, "RC5") == 0 ||
             strcmp(header.protocol, "RC5X") == 0) {
    IrSender.sendRC5((uint8_t)header.address, (uint8_t)header.command, 0);
  } else if (strcmp(header.protocol, "RC6") == 0) {
    IrSender.sendRC6((uint8_t)header.address, (uint8_t)header.command, 0);
  } else if (strncmp(header.protocol, "SIRC", 4) == 0) {
    IrSender.sendSony((uint16_t)header.address, (uint8_t)header.command, 2,
                      header.sonyBits ? header.sonyBits : 12);
  } else {
    Serial.printf("Dock: IR protocol not supported: %s\n", header.protocol);
    return false;
  }
  return true;
#endif
}

// Matches the remote's pill: a short train of fast pulses, same 240ms over
// 40ms steps, so the two are visibly the same event at both ends.
//
// It inverts from whatever the LED was resting at rather than simply turning
// on. Resting lit (paired) it reads as a flicker; resting dark (not paired
// yet) it reads as a flash. 1.07 only ever drove it low, which against an
// already-dark LED was completely invisible - the reported symptom.
static const uint32_t DOCK_TX_PULSE_MS = 240;
static const uint32_t DOCK_TX_STEP_MS = 40;

// Marks the LED as "transmitting" until a moment from now. Deliberately does
// not block.
//
// The blocking version delayed 240ms inside sendCommand, on top of the ~190ms
// the burst itself takes - 430ms per command against a remote that repeats a
// held button every 111ms. The dock could not keep up with its own indicator,
// and holding volume would have been visibly worse than the remote doing it
// directly. Nothing about an LED justifies costing more time than the
// transmission it is reporting.
//
// While a button is held each command extends the window, so the LED simply
// keeps blinking for as long as the stream lasts.
void noteTransmitForLed() {
  ledTxUntilMs = millis() + DOCK_TX_PULSE_MS;
}

void sendCommand(const EspNowCommandHeader &header, const uint16_t *timings,
                 uint16_t count) {
  // Transmit first, then note it for the LED. The whole command path is now
  // free of delay(): the only thing that costs time here is the burst itself,
  // so the dock's repeat ceiling is the same as the remote's own emitter -
  // they run the same library over the same timings.
  //
  // The switch silences the indication only; the IR still goes out. A dock in a
  // dark lounge should be able to stop blinking without stopping working.
  bool sent = transmitIr(header, timings, count);
  (void)sent;
  if (commandsDroppedBusy) {
    Serial.printf("Dock: %lu command(s) arrived while the last burst was still "
                  "going out and were dropped\n", (unsigned long)commandsDroppedBusy);
    commandsDroppedBusy = 0;
  }
  if (dockLedOnTransmit) noteTransmitForLed();
}

// ---------------------------------------------------------------------------
// Firmware transfer
// ---------------------------------------------------------------------------
//
// The remote pushes its stored dock image over ESP-NOW in 240 byte chunks and
// waits for an ack on each, so this is stop-and-wait: simple, and it cannot
// run ahead of a dock busy writing flash. About a megabyte takes well under a
// minute.
//
// Writing goes straight into the inactive OTA partition through esp_ota_*, so
// a transfer that dies half way leaves the running firmware untouched - the
// dock simply carries on with the old image. Nothing is made bootable until
// the CRC over the whole image matches.

uint32_t otaLastAckSeq = 0;
uint8_t otaLastAckStatus = 0;
unsigned long otaAckResentMs = 0;

void otaSendAck(uint32_t seq, uint8_t status) {
  if (!remoteKnown) return;
  otaLastAckSeq = seq;
  otaLastAckStatus = status;
  otaAckResentMs = millis();
  EspNowOtaAckPacket ack = {};
  ack.magic = ESPNOW_OTA_ACK_MAGIC;
  ack.seq = seq;
  ack.status = status;
  // Checked and retried rather than fired and forgotten. The ack that accepts
  // a transfer is sent in the worst possible moment for a radio send: straight
  // out of esp_ota_begin(), which has just spent several seconds erasing a
  // megabyte of flash with everything else starved behind it. Losing that one
  // packet stalls the whole update - the remote never learns it may start, the
  // dock waits fifteen seconds for data that was never asked for, and both
  // ends then report the other as broken.
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (esp_now_send(remoteMac, (const uint8_t *)&ack, sizeof(ack)) == ESP_OK) return;
    delay(2);
  }
  Serial.printf("Dock: could not send OTA ack (seq %lu, status %u)\n",
                (unsigned long)seq, (unsigned)status);
}

void otaAbort(const char *why, uint8_t status) {
  if (otaHandle) {
    esp_ota_abort(otaHandle);
    otaHandle = 0;
  }
  otaActive = false;
  Serial.printf("Dock: firmware transfer aborted - %s\n", why);
  otaSendAck(otaExpectedSeq, status);
}

void otaHandleBegin(const uint8_t *data, uint16_t len) {
  if (len < sizeof(EspNowOtaBeginPacket)) return;
  EspNowOtaBeginPacket begin;
  memcpy(&begin, data, sizeof(begin));

  // Cleared here, at the top, BEFORE esp_ota_begin() erases the partition.
  // They used to be cleared after it, which meant every frame the erase caused
  // to be dropped - the seconds when this dock is least able to listen and the
  // remote is most likely to be sending - was counted and then wiped before
  // anything could report it. The counters read a confident 0 and 0 while the
  // drops they existed to measure had already happened.
  otaFramesSeen = 0;
  otaFramesDroppedBusy = 0;

  if (otaHandle) {  // A previous attempt that never finished.
    esp_ota_abort(otaHandle);
    otaHandle = 0;
  }
  otaPartition = esp_ota_get_next_update_partition(nullptr);
  if (!otaPartition) {
    Serial.println("Dock: no OTA partition available");
    otaSendAck(0, OTA_ACK_BEGIN_REFUSED);
    return;
  }
  if (begin.totalBytes == 0 || begin.totalBytes > otaPartition->size) {
    Serial.printf("Dock: firmware refused - %lu bytes will not fit the %lu byte "
                  "partition\n", (unsigned long)begin.totalBytes,
                  (unsigned long)otaPartition->size);
    otaSendAck(0, OTA_ACK_BEGIN_REFUSED);
    return;
  }
  esp_err_t rc = esp_ota_begin(otaPartition, begin.totalBytes, &otaHandle);
  if (rc != ESP_OK) {
    Serial.printf("Dock: esp_ota_begin failed (%d)\n", (int)rc);
    otaHandle = 0;
    otaSendAck(0, OTA_ACK_BEGIN_REFUSED);
    return;
  }

  char version[9] = {0};
  memcpy(version, begin.version, sizeof(begin.version));
  otaActive = true;
  otaExpectedSeq = 0;
  otaTotalBytes = begin.totalBytes;
  otaWrittenBytes = 0;
  otaExpectedCrc = begin.crc32;
  otaRunningCrc = 0;
  otaLastChunkMs = millis();
  Serial.printf("Dock: firmware transfer starting - %lu bytes, version '%s', "
                "into %s\n", (unsigned long)otaTotalBytes, version,
                otaPartition->label);
  enterState(DOCK_OTA);
  otaSendAck(0, OTA_ACK_OK);
}

void otaHandleData(const uint8_t *data, uint16_t len) {
  if (!otaActive || len < sizeof(EspNowOtaDataHeader)) return;
  EspNowOtaDataHeader header;
  memcpy(&header, data, sizeof(header));
  otaLastChunkMs = millis();

  // A chunk already written. The ack that acknowledged it was lost, so say so
  // again rather than writing it twice.
  if (header.seq < otaExpectedSeq) {
    otaSendAck(otaExpectedSeq, OTA_ACK_OK);
    return;
  }
  // Ahead of where the dock is. Ack with what is actually wanted so the sender
  // rewinds instead of leaving a hole in the image.
  if (header.seq > otaExpectedSeq) {
    otaSendAck(otaExpectedSeq, OTA_ACK_OK);
    return;
  }
  uint16_t payload = header.len;
  if (payload > ESPNOW_OTA_CHUNK_BYTES ||
      (uint16_t)(sizeof(EspNowOtaDataHeader) + payload) > len) {
    otaAbort("malformed chunk", OTA_ACK_WRITE_FAILED);
    return;
  }
  const uint8_t *bytes = data + sizeof(EspNowOtaDataHeader);
  esp_err_t rc = esp_ota_write(otaHandle, bytes, payload);
  if (rc != ESP_OK) {
    Serial.printf("Dock: esp_ota_write failed (%d) at %lu bytes\n", (int)rc,
                  (unsigned long)otaWrittenBytes);
    otaAbort("flash write failed", OTA_ACK_WRITE_FAILED);
    return;
  }
  otaRunningCrc = esp_rom_crc32_le(otaRunningCrc, bytes, payload);
  otaWrittenBytes += payload;
  otaExpectedSeq++;
  otaSendAck(otaExpectedSeq, OTA_ACK_OK);

  if ((otaExpectedSeq % 200) == 0) {
    Serial.printf("Dock: firmware %lu / %lu bytes\n",
                  (unsigned long)otaWrittenBytes, (unsigned long)otaTotalBytes);
  }
}

void otaHandleEnd(const uint8_t *data, uint16_t len) {
  if (!otaActive || len < sizeof(EspNowOtaEndPacket)) return;
  EspNowOtaEndPacket end;
  memcpy(&end, data, sizeof(end));

  if (otaWrittenBytes != otaTotalBytes) {
    otaAbort("image is short", OTA_ACK_VERIFY_FAILED);
    enterState(DOCK_OTA_FAILED);
    return;
  }
  // Checked before the image is made bootable, so a corrupted transfer can
  // never become the firmware that runs.
  if (otaRunningCrc != end.crc32 || otaRunningCrc != otaExpectedCrc) {
    Serial.printf("Dock: CRC mismatch - got 0x%08lX, expected 0x%08lX\n",
                  (unsigned long)otaRunningCrc, (unsigned long)end.crc32);
    otaAbort("checksum mismatch", OTA_ACK_VERIFY_FAILED);
    enterState(DOCK_OTA_FAILED);
    return;
  }
  esp_err_t rc = esp_ota_end(otaHandle);
  otaHandle = 0;
  if (rc != ESP_OK) {
    Serial.printf("Dock: esp_ota_end failed (%d)\n", (int)rc);
    otaActive = false;
    otaSendAck(otaExpectedSeq, OTA_ACK_VERIFY_FAILED);
    enterState(DOCK_OTA_FAILED);
    return;
  }
  rc = esp_ota_set_boot_partition(otaPartition);
  if (rc != ESP_OK) {
    Serial.printf("Dock: set_boot_partition failed (%d)\n", (int)rc);
    otaActive = false;
    otaSendAck(otaExpectedSeq, OTA_ACK_VERIFY_FAILED);
    enterState(DOCK_OTA_FAILED);
    return;
  }
  otaActive = false;
  Serial.printf("Dock: firmware verified (%lu bytes, CRC 0x%08lX) - rebooting\n",
                (unsigned long)otaWrittenBytes, (unsigned long)otaRunningCrc);
  otaSendAck(otaExpectedSeq, OTA_ACK_COMPLETE);
  enterState(DOCK_OTA_SUCCESS);
}

// ---------------------------------------------------------------------------
// Radio
// ---------------------------------------------------------------------------

String macToString(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

#if DOCK_RF_CS_PIN >= 0
// Every edge, timed. Deltas only - the absolute time means nothing, the gap
// between transitions is the signal. Kept in IRAM because an ISR that has to
// fault a page in from flash would add exactly the jitter this is measuring.
void IRAM_ATTR rfEdgeIsr() {
  uint32_t now = micros();
  uint32_t delta = now - rfLastEdgeUs;
  rfLastEdgeUs = now;
  if (!rfCapturing) return;
  // The first edge's delta is the time since some unrelated earlier edge, so
  // it is noise by definition and is used only to start the clock.
  if (rfEdgeCount == 0 && delta > RF_END_GAP_US) return;
  if (delta > RF_MAX_EDGE_US) delta = RF_MAX_EDGE_US;
  if (rfEdgeCount < RF_CAPTURE_MAX) rfEdges[rfEdgeCount++] = (uint16_t)delta;
}

void rfIdle() {
  rfCapturing = false;
  detachInterrupt(digitalPinToInterrupt(DOCK_RF_GDO0_PIN));
  ELECHOUSE_cc1101.setSidle();
}

bool rfInit() {
  ELECHOUSE_cc1101.setSpiPin(DOCK_RF_SCK_PIN, DOCK_RF_MISO_PIN,
                             DOCK_RF_MOSI_PIN, DOCK_RF_CS_PIN);
#if DOCK_RF_GDO2_PIN >= 0
  ELECHOUSE_cc1101.setGDO(DOCK_RF_GDO0_PIN, DOCK_RF_GDO2_PIN);
#else
  ELECHOUSE_cc1101.setGDO0(DOCK_RF_GDO0_PIN);
#endif
  // Asked before anything is configured: getCC1101() reads the part and
  // version registers back over SPI, so it answers "is a CC1101 actually
  // wired to these pins" rather than "did we write some registers into
  // nothing". Without it a missing or miswired module looks identical to a
  // working one until a capture silently returns nothing.
  if (!ELECHOUSE_cc1101.getCC1101()) return false;
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(DOCK_RF_FREQ_MHZ);
  ELECHOUSE_cc1101.setModulation(2);   // 2 = ASK/OOK, what these remotes use.
  ELECHOUSE_cc1101.setCCMode(0);       // Raw, not the packet engine.
  ELECHOUSE_cc1101.setPktFormat(3);    // Asynchronous serial: GDO0 is the data.
  ELECHOUSE_cc1101.setRxBW(200);
  ELECHOUSE_cc1101.setPA(10);          // 10 dBm, the module's maximum.
  ELECHOUSE_cc1101.setSidle();
  return true;
}

void rfStartCapture(uint32_t timeoutMs) {
  rfEdgeCount = 0;
  rfLastEdgeUs = micros();
  pinMode(DOCK_RF_GDO0_PIN, INPUT);
  ELECHOUSE_cc1101.SetRx();
  rfCapturing = true;
  attachInterrupt(digitalPinToInterrupt(DOCK_RF_GDO0_PIN), rfEdgeIsr, CHANGE);
  rfLearnActive = true;
  rfLearnEndsMs = millis() + timeoutMs;
  Serial.printf("Dock: RF433 listening for %lu ms\n", (unsigned long)timeoutMs);
}

// delayMicroseconds is only reliable for short waits, and a gap between OOK
// bursts can be tens of milliseconds. Split so long waits stay accurate.
static inline void rfWaitUs(uint32_t us) {
  while (us > 8000UL) { delayMicroseconds(8000); us -= 8000UL; }
  if (us) delayMicroseconds(us);
}

bool rfTransmitRaw(const uint16_t *timings, uint16_t count) {
  if (!rfPresent || !timings || !count) return false;
  bool wasLearning = rfLearnActive;
  rfIdle();
  pinMode(DOCK_RF_GDO0_PIN, OUTPUT);
  digitalWrite(DOCK_RF_GDO0_PIN, LOW);
  ELECHOUSE_cc1101.SetTx();
  uint32_t began = micros();
  // Three times with a gap between, which is what a real remote does: these
  // receivers expect to hear the code repeated and many ignore a single
  // burst outright.
  for (uint8_t repeat = 0; repeat < 3; repeat++) {
    bool level = true;   // A capture always begins with the carrier keyed on.
    for (uint16_t i = 0; i < count; i++) {
      digitalWrite(DOCK_RF_GDO0_PIN, level ? HIGH : LOW);
      rfWaitUs(timings[i]);
      level = !level;
    }
    digitalWrite(DOCK_RF_GDO0_PIN, LOW);
    if (repeat < 2) rfWaitUs(15000);
  }
  uint32_t tookUs = micros() - began;
  ELECHOUSE_cc1101.setSidle();
  if (serialHostAttached()) {
    Serial.printf("Dock: RF433 sent %u timing(s) x3 in %lu.%02lu ms\n",
                  (unsigned)count, (unsigned long)(tookUs / 1000UL),
                  (unsigned long)((tookUs % 1000UL) / 10UL));
  }
  if (wasLearning) rfStartCapture(rfLearnEndsMs > millis() ? rfLearnEndsMs - millis() : 0);
  return true;
}
#endif  // DOCK_RF_CS_PIN >= 0

void setRadioChannel(uint8_t channel) {
  if (channel < CHANNEL_MIN || channel > CHANNEL_MAX) return;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

bool ensurePeer(const uint8_t mac[6]) {
  if (esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  // 0 means "use the interface's current channel", which is what the remote
  // does too. The dock sets that channel explicitly instead of joining an AP.
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

void rememberRemote(const uint8_t mac[6], uint8_t channel) {
  bool changed = !remoteKnown || memcmp(remoteMac, mac, 6) != 0 ||
                 lockedChannel != channel;
  memcpy(remoteMac, mac, 6);
  remoteKnown = true;
  lockedChannel = channel;
  // Settle on this channel and stay there until the next pairing. The walk
  // exists only to find the remote; once found, hopping is what would put the
  // two out of step, so it stops here and the radio is parked explicitly
  // rather than left wherever the sweep happened to be.
  setRadioChannel(lockedChannel);
  if (!changed) return;
  prefs.begin("dock", false);
  prefs.putBytes("remoteMac", remoteMac, 6);
  prefs.putUChar("channel", lockedChannel);
  prefs.end();
  Serial.printf("Dock: remote %s locked on channel %u (saved)\n",
                macToString(remoteMac).c_str(), lockedChannel);
}

void loadRemote() {
  prefs.begin("dock", true);
  size_t got = prefs.getBytesLength("remoteMac");
  if (got == 6) {
    prefs.getBytes("remoteMac", remoteMac, 6);
    remoteKnown = true;
  }
  lockedChannel = prefs.getUChar("channel", 0);
  dockRfEnabled = prefs.getBool("rf", true);
  dockLedOnTransmit = prefs.getBool("ledTx", true);
  hbSsid = prefs.getString("wifiSsid", "");
  hbPassword = prefs.getString("wifiPass", "");
  hbAddress = prefs.getString("hbAddr", "");
  hbUser = prefs.getString("hbUser", "");
  hbPass = prefs.getString("hbPass", "");
  wifiConfigured = hbSsid.length() > 0;
  prefs.end();
  if (wifiConfigured) {
    Serial.printf("Dock: Wi-Fi configured for '%s'%s\n", hbSsid.c_str(),
                  hbAddress.length() ? ", Homebridge relay ready" : "");
  }
  if (remoteKnown) {
    Serial.printf("Dock: paired with %s on channel %u - LED will rest lit\n",
                  macToString(remoteMac).c_str(), lockedChannel);
    if (!lockedChannel) {
      Serial.println("Dock: WARNING - paired but no channel stored. Re-pair so the "
                     "channel is learned, or commands will not be heard.");
    }
  } else {
    // Worth being blunt: an unpaired dock has no stored channel either, so it
    // sits on the radio default while the remote transmits on its own. Nothing
    // arrives and the LED stays dark - which looks like broken hardware rather
    // than an unpaired dock. Flashing over USB erases both, so this is the
    // expected state right after a Studio install.
    Serial.println("Dock: NOT PAIRED. The LED stays off and no commands will be "
                   "received until you pair.");
    Serial.println("Dock: hold the button for 5 seconds, then use Search for a "
                   "dock on the remote.");
  }
}

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!info || !info->src_addr || !data || len < (int)sizeof(uint32_t)) return;

  // Anything at all from our remote means its radio is up. Recorded before the
  // packet is even identified, because the fact of the packet is the signal.
  if (remoteKnown && memcmp(info->src_addr, remoteMac, 6) == 0) {
    lastRemoteContactMs = millis();
    if (remoteLinkDown) {
      remoteLinkDown = false;
      ledStateDirty = true;
    }
  }
  uint32_t magic = 0;
  memcpy(&magic, data, sizeof(magic));

  // Scoped to IR commands, which is the only thing it was ever about: the dock
  // has one emitter and cannot start a second burst while the first is still
  // going out. It used to sit above this point and return for EVERY packet
  // type, so a firmware frame, a settings push, a ping or a pairing proof that
  // happened to arrive during a burst was thrown away as though it were a
  // command. For an OTA that was fatal rather than merely lossy: the remote
  // sends a begin frame, the dock silently discards it, no ack ever comes
  // back, and the update fails with "the dock stopped responding" - blaming
  // power or range for what was really a busy IR emitter. A firmware transfer
  // has nothing to do with the emitter and must never be gated on it.
  if (otaActive && remoteKnown && memcmp(info->src_addr, remoteMac, 6) == 0) {
    otaFramesSeen++;
  }

  if (magic == ESPNOW_COMMAND_MAGIC && pendingCommand) {
    // Dropped because the previous burst is still going out. Counted rather
    // than silently discarded: this gate sits before the log line, so a command
    // lost here never appeared anywhere and "24 received, 24 sent" looked
    // perfect while the user was watching presses go missing.
    commandsDroppedBusy++;
    return;
  }

  uint8_t primary = 0;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&primary, &second);

  if (magic == ESPNOW_COMMAND_MAGIC && len >= (int)sizeof(EspNowCommandHeader)) {
    memcpy(&pendingHeader, data, sizeof(pendingHeader));
    pendingTimingCount = 0;
    if (pendingHeader.encoding == 1 && pendingHeader.rawCount) {
      uint16_t count = pendingHeader.rawCount;
      if (count > (uint16_t)(sizeof(pendingTimings) / sizeof(pendingTimings[0]))) {
        count = sizeof(pendingTimings) / sizeof(pendingTimings[0]);
      }
      size_t need = sizeof(EspNowCommandHeader) + (size_t)count * sizeof(uint16_t);
      if ((size_t)len >= need) {
        memcpy(pendingTimings, data + sizeof(EspNowCommandHeader),
               (size_t)count * sizeof(uint16_t));
        pendingTimingCount = count;
      }
    }
    memcpy(pendingSrcMac, info->src_addr, 6);
    pendingChannel = primary;
    pendingPairProof = true;
    pendingCommand = true;
    return;
  }

  // A unicast from a remote is itself proof of pairing: the remote only sends
  // to peers it has already added.
  if (magic == ESPNOW_OTA_BEGIN_MAGIC || magic == ESPNOW_OTA_DATA_MAGIC ||
      magic == ESPNOW_OTA_END_MAGIC) {
    // Firmware is accepted from the paired remote and from nothing else. Any
    // ESP-NOW device within range can address this dock, and unlike a command
    // - which at worst blinks an LED - a firmware push replaces what the dock
    // runs. Pairing is therefore a precondition of updating, not merely the
    // usual way of getting there: an unpaired dock cannot be updated over the
    // air at all, only over USB.
    if (!remoteKnown || memcmp(info->src_addr, remoteMac, 6) != 0) return;
    if (pendingOta) {
      // loop() has not taken the last one yet. Counted, because if the remote
      // retries a chunk faster than this dock drains one, every retry lands
      // here and the transfer starves while both ends believe the other is at
      // fault.
      otaFramesDroppedBusy++;
      return;
    }
    if (magic == ESPNOW_OTA_BEGIN_MAGIC) {
      Serial.printf("Dock: OTA begin frame received (%d bytes)\n", len);
    }
    uint16_t n = (uint16_t)len;
    if (n > sizeof(otaFrame)) n = sizeof(otaFrame);
    memcpy(otaFrame, data, n);
    otaFrameLen = n;
    pendingOta = true;
    return;
  }

  if (magic == ESPNOW_DOCK_WIFI_MAGIC && len >= (int)sizeof(EspNowDockWifiPacket)) {
    if (!remoteKnown || memcmp(info->src_addr, remoteMac, 6) != 0) return;
    EspNowDockWifiPacket packet;
    memcpy(&packet, data, sizeof(packet));
    packet.ssid[sizeof(packet.ssid) - 1] = '\0';
    packet.password[sizeof(packet.password) - 1] = '\0';
    memcpy(&pendingWifiConfig, &packet, sizeof(packet));
    pendingWifiConfigReady = true;   // Applied from loop(): this writes NVS.
    return;
  }

  if (magic == ESPNOW_DOCK_HBCFG_MAGIC && len >= (int)sizeof(EspNowDockHomebridgePacket)) {
    if (!remoteKnown || memcmp(info->src_addr, remoteMac, 6) != 0) return;
    EspNowDockHomebridgePacket packet;
    memcpy(&packet, data, sizeof(packet));
    packet.address[sizeof(packet.address) - 1] = '\0';
    packet.username[sizeof(packet.username) - 1] = '\0';
    packet.password[sizeof(packet.password) - 1] = '\0';
    memcpy(&pendingHbConfig, &packet, sizeof(packet));
    pendingHbConfigReady = true;
    return;
  }

  if (magic == ESPNOW_HOMEBRIDGE_MAGIC && len >= (int)sizeof(EspNowHomebridgePacket)) {
    if (!remoteKnown || memcmp(info->src_addr, remoteMac, 6) != 0) return;
    if (pendingHomebridge) return;   // loop() has not taken the last one yet.
    memcpy(&pendingHomebridgeCommand, data, sizeof(pendingHomebridgeCommand));
    pendingHomebridgeCommand.accessoryId[sizeof(pendingHomebridgeCommand.accessoryId) - 1] = '\0';
    pendingHomebridgeCommand.characteristic[sizeof(pendingHomebridgeCommand.characteristic) - 1] = '\0';
    pendingHomebridgeCommand.stringValue[sizeof(pendingHomebridgeCommand.stringValue) - 1] = '\0';
    memcpy(pendingHomebridgeMac, info->src_addr, 6);
    // Handled from loop(): an HTTP round trip cannot run on the Wi-Fi task.
    pendingHomebridge = true;
    return;
  }

  if (magic == ESPNOW_DOCK_SETTINGS_MAGIC && len >= (int)sizeof(EspNowDockSettingsPacket)) {
    if (!remoteKnown || memcmp(info->src_addr, remoteMac, 6) != 0) return;
    EspNowDockSettingsPacket packet;
    memcpy(&packet, data, sizeof(packet));
    pendingSettingsRf = packet.rfEnabled != 0;
    pendingSettingsLed = packet.ledOnTransmit != 0;
    pendingSettings = true;
    return;
  }
  if (magic == ESPNOW_DOCK_LINK_DOWN_MAGIC) {
    if (remoteKnown && memcmp(info->src_addr, remoteMac, 6) == 0) {
      remoteLinkDown = true;
      ledStateDirty = true;
    }
    return;
  }
  if (magic == ESPNOW_DOCK_PING_MAGIC) {
    // The MAC-layer ack was the ping's own purpose; the reply is how the remote
    // learns what is on the other end. Queued rather than sent from here - this
    // runs on the Wi-Fi task.
    if (remoteKnown && memcmp(info->src_addr, remoteMac, 6) == 0) {
      pendingInfoReply = true;
      // The remote states which channel it is on. Believe it over our own
      // guess: adjacent 2.4GHz channels overlap enough that this frame may
      // well have leaked in from a channel or two away, and that leakage is
      // precisely how the dock ended up locked to the wrong one. Applied from
      // loop() - this runs on the Wi-Fi task and rememberRemote() writes NVS.
      if (len >= (int)(sizeof(uint32_t) + 1)) {
        uint8_t stated = data[sizeof(uint32_t)];
        if (stated >= CHANNEL_MIN && stated <= CHANNEL_MAX && stated != lockedChannel) {
          pendingChannelMove = stated;
        }
      }
    }
    return;
  }

  if (magic == ESPNOW_RF_LEARN_START_MAGIC && len >= (int)sizeof(EspNowRfLearnStartPacket)) {
    if (remoteKnown && memcmp(info->src_addr, remoteMac, 6) == 0) {
      EspNowRfLearnStartPacket start;
      memcpy(&start, data, sizeof(start));
      memcpy(rfLearnMac, info->src_addr, 6);
      pendingRfLearnMs = start.timeoutMs ? start.timeoutMs : 15000UL;
      pendingRfLearn = true;   // Started from loop(): SPI work has no place in an ISR.
    }
    memcpy(pendingSrcMac, info->src_addr, 6);
    pendingChannel = primary;
    pendingPairProof = true;
    return;
  }

  if (magic == ESPNOW_PAIR_ACK_MAGIC) {
    memcpy(pendingSrcMac, info->src_addr, 6);
    pendingChannel = primary;
    pendingPairProof = true;
  }
}

void sendAnnounce() {
  EspNowAnnouncePacket packet = {};
  packet.magic = ESPNOW_ANNOUNCE_MAGIC;
  strlcpy(packet.name, dockName, sizeof(packet.name));
  esp_now_send(BROADCAST_MAC, (const uint8_t *)&packet, sizeof(packet));
}

void radioSetup() {
  WiFi.mode(WIFI_STA);
  // No AP to join. ESP-NOW only needs the interface up and parked on the right
  // channel, and disconnect() stops the station logic from retuning it.
  WiFi.disconnect();
  delay(50);

  // The single most important line in this file for responsiveness.
  //
  // An ESP32 station defaults to WIFI_PS_MIN_MODEM: the radio duty-cycles,
  // waking only periodically, because the usual assumption is a battery and an
  // AP buffering anything that arrives while it naps. ESP-NOW has no AP and no
  // buffering - a frame that arrives while the receiver is asleep is simply
  // gone. That is what made the dock sluggish and unresponsive at any real
  // distance, and it is why holding a USB serial connection open "fixed" it:
  // an active USB CDC connection inhibits those sleep states, so the radio
  // stayed awake for as long as the monitor was attached and every frame
  // landed.
  //
  // The dock is mains powered. There is nothing to save and no reason to
  // sleep, so power save is turned off outright rather than tuned.
  WiFi.setSleep(false);
  esp_err_t psResult = esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.printf("Dock: WiFi power save disabled (rc=%d) - radio stays awake\n",
                (int)psResult);

  // Range was the other complaint, and TX power is worth stating explicitly
  // rather than inheriting: 78 quarter-dBm units is 19.5 dBm, the maximum this
  // part will accept for normal operation. Reported back after the fact
  // because the driver clamps to what the calibration data allows, so the
  // value that matters is the one it actually took, not the one asked for.
  esp_wifi_set_max_tx_power(78);
  int8_t txPower = 0;
  if (esp_wifi_get_max_tx_power(&txPower) == ESP_OK) {
    Serial.printf("Dock: TX power %d (%.2f dBm)\n", (int)txPower, txPower / 4.0);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Dock: ESP-NOW init FAILED");
    return;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  ensurePeer(BROADCAST_MAC);
  if (remoteKnown) ensurePeer(remoteMac);
  if (lockedChannel) setRadioChannel(lockedChannel);

  Serial.printf("Dock: ESP-NOW ready, own MAC %s\n", WiFi.macAddress().c_str());
}

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------

void enterState(DockState next) {
  dockState = next;
  dockStateSinceMs = millis();
  lastBlinkMs = 0;
  switch (next) {
    case DOCK_PAIRING:
      Serial.println("Dock: pairing window OPEN - announcing on channels 1-13");
      walkChannel = CHANNEL_MIN;
      lastChannelHopMs = 0;
      lastAnnounceMs = 0;
      break;
    case DOCK_PAIR_SUCCESS:
      Serial.println("Dock: paired - LED solid for 10s");
      ledWrite(true);
      break;
    case DOCK_PAIR_FAILED:
      Serial.println("Dock: pairing failed or timed out");
      // The walk left the radio on an arbitrary channel. If this dock already
      // has a remote, go back to its channel - otherwise a failed re-pairing
      // would silently break a working pairing by stranding the radio
      // somewhere the remote never transmits.
      if (remoteKnown && lockedChannel) {
        setRadioChannel(lockedChannel);
        Serial.printf("Dock: back on channel %u with %s\n", lockedChannel,
                      macToString(remoteMac).c_str());
      }
      break;
    case DOCK_OTA:
      Serial.println("Dock: receiving firmware - LED blinking slowly");
      break;
    case DOCK_OTA_SUCCESS:
      ledWrite(true);
      break;
    case DOCK_OTA_FAILED:
      Serial.println("Dock: firmware transfer failed - keeping the current image");
      break;
    case DOCK_IDLE:
      applyIdleLed();
      // Back to the channel the remote is known to be on, so commands arrive.
      if (lockedChannel) setRadioChannel(lockedChannel);
      break;
  }
}

// Blocking on purpose and only ever a few hundred milliseconds, in response to
// a button the user is holding. Nothing is in flight at that moment.
void blinkTimes(uint8_t times, uint32_t onMs, uint32_t offMs) {
  for (uint8_t i = 0; i < times; i++) {
    ledWrite(true);
    delay(onMs);
    ledWrite(false);
    if (i + 1 < times) delay(offMs);
  }
}

void forgetRemote() {
  // The LED follows: nothing paired means nothing lit.
  prefs.begin("dock", false);
  prefs.clear();
  prefs.end();
  remoteKnown = false;
  lockedChannel = 0;
  memset(remoteMac, 0, sizeof(remoteMac));
  applyIdleLed();
  Serial.println("Dock: pairing forgotten - hold 5s to pair again");
}

// Three gestures shared by either button:
//
//   tap (under 1s)  identify - three quick blinks, so the right dock can be
//                   picked out of several without touching its pairing
//   hold 5s         open the pairing window
//   hold 10s        forget the paired remote entirely
//
// Each fires once per press, and the longer gesture supersedes the shorter:
// keep holding past 5s and the pairing window that just opened is closed again
// by the forget at 10s, which is what someone holding it that long means.
void serviceButton(unsigned long now) {
  bool pressed = digitalRead(DOCK_BUTTON_PIN) == LOW;
#if DOCK_MENU_BUTTON_PIN >= 0
  pressed = pressed || digitalRead(DOCK_MENU_BUTTON_PIN) == LOW;
#endif

  if (pressed && !buttonDown) {
    buttonDown = true;
    buttonDownAtMs = now;
    buttonHoldConsumed = 0;
    return;
  }
  if (!pressed && buttonDown) {
    buttonDown = false;
    uint32_t held = now - buttonDownAtMs;
    // A tap only means "identify" if no hold gesture already fired.
    if (!buttonHoldConsumed && held < 1000 && dockState == DOCK_IDLE) {
      Serial.printf("Dock: identify - %s, channel %u\n",
                    remoteKnown ? macToString(remoteMac).c_str() : "not paired",
                    lockedChannel);
      blinkTimes(IDENTIFY_BLINKS, 70, 120);
    }
    return;
  }
  if (!pressed || !buttonDown) return;

  uint32_t held = now - buttonDownAtMs;

  if (held >= FORGET_HOLD_MS && buttonHoldConsumed != 2) {
    buttonHoldConsumed = 2;  // Forget fired; nothing else can fire this press.
    forgetRemote();
    blinkTimes(6, 90, 90);
    enterState(DOCK_IDLE);
    return;
  }
  if (held >= PAIR_HOLD_MS && !buttonHoldConsumed) {
    buttonHoldConsumed = 1;
    if (dockState != DOCK_PAIRING && dockState != DOCK_OTA) enterState(DOCK_PAIRING);
  }
}

void servicePairing(unsigned long now) {
  // Leaving the pairing state means the channel is settled - never resume the
  // walk on a dock that already knows its remote.
  if (dockState != DOCK_PAIRING) return;

  ledBlink(now, PAIR_BLINK_MS);

  // Walk the channels. The remote sits on its router's channel and the dock
  // cannot know which that is, so it broadcasts on each in turn until one of
  // them is the right one.
  if (now - lastChannelHopMs >= CHANNEL_DWELL_MS) {
    lastChannelHopMs = now;
    setRadioChannel(walkChannel);
    walkChannel++;
    if (walkChannel > CHANNEL_MAX) walkChannel = CHANNEL_MIN;
  }
  if (now - lastAnnounceMs >= ANNOUNCE_INTERVAL_MS) {
    lastAnnounceMs = now;
    sendAnnounce();
  }
  if (now - dockStateSinceMs >= PAIR_WINDOW_MS) enterState(DOCK_PAIR_FAILED);
}

void serviceLedStates(unsigned long now) {
  switch (dockState) {
    case DOCK_PAIR_SUCCESS:
      if (now - dockStateSinceMs >= PAIR_SUCCESS_HOLD_MS) enterState(DOCK_IDLE);
      break;
    case DOCK_PAIR_FAILED:
      ledBlink(now, PAIR_FAIL_BLINK_MS);
      if (now - dockStateSinceMs >= PAIR_FAIL_DURATION_MS) enterState(DOCK_IDLE);
      break;
    case DOCK_OTA:
      ledBlink(now, OTA_BLINK_MS);
      break;
    case DOCK_OTA_SUCCESS:
      if (now - dockStateSinceMs >= OTA_SUCCESS_HOLD_MS) {
        Serial.println("Dock: restarting into the new firmware");
        Serial.flush();
        ESP.restart();
      }
      break;
    case DOCK_OTA_FAILED:
      // Same fast blink as a failed pairing, deliberately: what matters at a
      // distance is that something failed. Serial says which.
      ledBlink(now, PAIR_FAIL_BLINK_MS);
      if (now - dockStateSinceMs >= OTA_FAIL_DURATION_MS) enterState(DOCK_IDLE);
      break;
    default:
      break;
  }
}

// Keeps the LED in step with the link without the receive callback ever
// touching it, and catches the silence timeout that no packet will announce.
// The boot banner can be lost - USB CDC on this chip overruns during the burst
// of output right after Serial.begin, which is why a boot log turned up missing
// its pairing line and with a mangled pin number. Anything needed for
// diagnosis is therefore repeated while it matters rather than said once.
void serviceStateLog(unsigned long now) {
  static unsigned long nextMs = 0;
  static bool lastLit = false;
  static bool primed = false;
  bool lit = ledIdleShouldBeLit();
  bool due = (int32_t)(now - nextMs) >= 0;
  if (!primed || lit != lastLit || due) {
    primed = true;
    lastLit = lit;
    nextMs = now + 10000UL;
    Serial.printf("Dock: paired=%s linkUp=%s ledLit=%s ledOnTx=%s rf=%s channel=%u\n",
                  remoteKnown ? "yes" : "NO",
                  remoteLinkUp() ? "yes" : "no",
                  lit ? "yes" : "no",
                  dockLedOnTransmit ? "on" : "OFF",
                  dockRfEnabled ? "on" : "off",
                  (unsigned)lockedChannel);
  }
}

void serviceLinkLed(unsigned long now) {
  if (dockState != DOCK_IDLE) return;   // Pairing and OTA own the LED.
  static bool lastLit = false;
  static bool wasTransmitting = false;

  // Blinking while transmitting, inverted from the resting state so it reads
  // as a flicker when lit and a flash when dark.
  bool transmitting = (int32_t)(now - ledTxUntilMs) < 0;
  if (transmitting) {
    bool resting = ledIdleShouldBeLit();
    bool phase = ((now / DOCK_TX_STEP_MS) % 2) == 0;
    ledWrite(phase ? !resting : resting);
    wasTransmitting = true;
    return;
  }

  bool lit = ledIdleShouldBeLit();
  if (ledStateDirty || wasTransmitting || lit != lastLit) {
    ledStateDirty = false;
    wasTransmitting = false;
    lastLit = lit;
    ledWrite(lit);
  }
}

void serviceInfoReply() {
  if (!pendingInfoReply) return;
  pendingInfoReply = false;
  if (!remoteKnown) return;
  EspNowDockInfoPacket info = {};
  info.magic = ESPNOW_DOCK_INFO_MAGIC;
  strlcpy(info.version, OPENREMOTE_DOCK_VERSION_STRING, sizeof(info.version));
  // The dock's own name is whatever it announces when pairing; the remote may
  // have renamed its copy since, and the remote's name is the one a person
  // sees, so this is only a fallback for a dock the remote has no name for.
  strlcpy(info.name, dockName, sizeof(info.name));
  esp_now_send(remoteMac, (const uint8_t *)&info, sizeof(info));
}

void serviceSettings() {
  if (!pendingSettings) return;
  pendingSettings = false;
  bool rf = pendingSettingsRf, led = pendingSettingsLed;
  if (rf == dockRfEnabled && led == dockLedOnTransmit) return;
  dockRfEnabled = rf;
  dockLedOnTransmit = led;
  prefs.begin("dock", false);
  prefs.putBool("rf", dockRfEnabled);
  prefs.putBool("ledTx", dockLedOnTransmit);
  prefs.end();
  Serial.printf("Dock: settings from remote - RF %s, LED on transmit %s\n",
                dockRfEnabled ? "on" : "off", dockLedOnTransmit ? "on" : "off");
}

void rfSendLearnResult(bool ok, const uint16_t *timings, uint16_t count) {
  uint8_t frame[250];
  EspNowRfLearnResultHeader header = {};
  header.magic = ESPNOW_RF_LEARN_RESULT_MAGIC;
  header.ok = ok ? 1 : 0;
  header.rawCount = ok ? count : 0;
  memcpy(frame, &header, sizeof(header));
  size_t bytes = sizeof(header);
  if (ok && count) {
    memcpy(frame + bytes, timings, (size_t)count * sizeof(uint16_t));
    bytes += (size_t)count * sizeof(uint16_t);
  }
  // Retried for the same reason the OTA acceptance ack is: the remote is
  // sitting in a timed window waiting for exactly this, and losing it once
  // means the user is told the capture failed when it did not.
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (esp_now_send(rfLearnMac, frame, bytes) == ESP_OK) return;
    delay(2);
  }
  Serial.println("Dock: could not send the RF433 result back");
}

void serviceRfLearn(unsigned long now) {
#if DOCK_RF_CS_PIN >= 0
  if (pendingRfLearn) {
    pendingRfLearn = false;
    if (!dockRfEnabled) {
      Serial.println("Dock: RF433 learn refused - RF is switched off");
      rfSendLearnResult(false, nullptr, 0);
    } else if (!rfPresent) {
      Serial.println("Dock: RF433 learn refused - no CC1101 answered on the SPI pins");
      rfSendLearnResult(false, nullptr, 0);
    } else {
      rfStartCapture(pendingRfLearnMs);
    }
  }
  if (!rfLearnActive) return;

  // A burst has landed and the air has gone quiet again: that gap is the end
  // of the transmission, and waiting for the full window after it would just
  // make the user hold the button wondering whether it worked.
  uint16_t captured = rfEdgeCount;
  if (captured >= RF_MIN_EDGES && (micros() - rfLastEdgeUs) > RF_END_GAP_US) {
    rfIdle();
    rfLearnActive = false;
    uint16_t send = captured > RF_SEND_MAX ? RF_SEND_MAX : captured;
    // Copied out of the volatile ISR buffer before sending: the radio is
    // stopped by now, but the buffer is still shared state.
    static uint16_t out[RF_SEND_MAX];
    for (uint16_t i = 0; i < send; i++) out[i] = rfEdges[i];
    Serial.printf("Dock: RF433 captured %u edge(s)%s\n", (unsigned)captured,
                  captured > RF_SEND_MAX ? " - truncated to fit one ESP-NOW frame" : "");
    rfSendLearnResult(true, out, send);
    return;
  }

  if ((long)(now - rfLearnEndsMs) >= 0) {
    rfIdle();
    rfLearnActive = false;
    Serial.printf("Dock: RF433 window closed with %u edge(s) - nothing usable\n",
                  (unsigned)captured);
    rfSendLearnResult(false, nullptr, 0);
  }
#else
  (void)now;
  if (pendingRfLearn) {
    pendingRfLearn = false;
    rfSendLearnResult(false, nullptr, 0);
  }
#endif
}

// Follows the remote to whichever channel it says it is on.
void serviceChannelMove() {
  uint8_t target = pendingChannelMove;
  if (!target) return;
  pendingChannelMove = 0;
  if (!remoteKnown || target == lockedChannel) return;
  if (otaActive || rfLearnActive || dockState == DOCK_PAIRING) return;
  Serial.printf("Dock: remote says it is on channel %u, we are on %u - following\n",
                (unsigned)target, (unsigned)lockedChannel);
  rememberRemote(remoteMac, target);
}

// Joins the configured Wi-Fi and keeps it joined.
//
// ESP-NOW and the station share one radio and therefore one channel. Joining an
// access point pins that channel to the router's, which is exactly what the
// remote already states in every keepalive ping - so the two stay in step
// rather than fighting. Nothing here changes the ESP-NOW channel directly.
void serviceHomebridgeWifi(unsigned long now) {
  if (!wifiConfigured || otaActive) return;
  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wifiJoined) {
    wifiJoined = up;
    if (up) {
      // Power save OFF again, now that the station has associated.
      //
      // This is the whole point of relaying through the dock: the association
      // must stay up and awake so a command is one HTTP round trip with no
      // waiting. WIFI_PS_NONE was set at boot, but associating puts the
      // Arduino default WIFI_PS_MIN_MODEM back, and a station in modem sleep
      // parks its radio between beacons - which would both slow the HTTP call
      // and lose ESP-NOW frames arriving during a nap. Exactly the fault the
      // remote hit in 3.93, and it would have quietly undone the benefit here.
      WiFi.setSleep(false);
      esp_wifi_set_ps(WIFI_PS_NONE);
      Serial.printf("Dock: Wi-Fi joined '%s' as %s on channel %u, staying awake\n",
                    hbSsid.c_str(), WiFi.localIP().toString().c_str(),
                    (unsigned)WiFi.channel());
      // The association may have moved the radio. Whatever channel it landed
      // on is now the one ESP-NOW must use as well.
      uint8_t primary = WiFi.channel();
      if (primary >= CHANNEL_MIN && primary <= CHANNEL_MAX && primary != lockedChannel &&
          remoteKnown) {
        Serial.printf("Dock: following Wi-Fi onto channel %u (was %u)\n",
                      (unsigned)primary, (unsigned)lockedChannel);
        rememberRemote(remoteMac, primary);
      }
    } else {
      Serial.println("Dock: Wi-Fi lost - retrying");
      hbToken = "";
    }
  }
  if (!up && (long)(now - wifiNextAttemptMs) >= 0) {
    wifiNextAttemptMs = now + WIFI_RETRY_MS;
    // The SDK's own reconnect is faster than this retry loop and handles a
    // brief AP outage without help; this remains as the backstop for when it
    // gives up entirely.
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.begin(hbSsid.c_str(), hbPassword.c_str());
  }

  // Re-asserted while connected, not just at the moment of joining. A
  // reconnect handled inside the SDK does not come back through the branch
  // above, and would silently restore modem sleep.
  if (up && (long)(now - wifiPsAssertMs) > 5000) {
    wifiPsAssertMs = now;
    esp_wifi_set_ps(WIFI_PS_NONE);
  }
}

// Normalises the Homebridge address the same way the remote does.
//
// The address the user types may or may not already carry a scheme, and may
// have a trailing path or slash. Hardcoding "http://" in front of it produced
// "http://http://192.168.x.x:8581/..." for anyone who typed the scheme, and the
// dock then tried to resolve a host literally called "http":
//   hostByName(): DNS Failed for 'http' with error '-54'
// which surfaced as a bare "login failed (HTTP -1)" with no hint of the cause.
// The remote has always normalised this; the dock simply did not.
String normaliseHomebridgeAddress(String address) {
  address.trim();
  if (!address.length()) return "";
  if (!address.startsWith("http://") && !address.startsWith("https://")) {
    address = "http://" + address;
  }
  int schemeEnd = address.indexOf("://");
  int pathStart = address.indexOf('/', schemeEnd + 3);
  if (pathStart >= 0) address.remove(pathStart);
  while (address.endsWith("/")) address.remove(address.length() - 1);
  return address;
}

// Logs in and stores the token. Homebridge issues a bearer token that expires,
// so this is called again whenever a request comes back 401 or 403.
bool homebridgeLogin(String &error) {
  HTTPClient http;
  String url = normaliseHomebridgeAddress(hbAddress) + "/api/auth/login";
  if (!http.begin(url)) {
    error = String("Bad Homebridge URL: ") + url;
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"username\":\"") + hbUser + "\",\"password\":\"" + hbPass + "\"}";
  int code = http.POST(body);
  String reply = http.getString();
  http.end();
  if (code != 200 && code != 201) {
    // The URL is included because a bare status number sent the last
    // investigation looking at credentials when the address was malformed.
    error = String("Login failed HTTP ") + code + " at " + url;
    if (serialHostAttached()) Serial.printf("Dock: %s\n", error.c_str());
    return false;
  }
  // Pulled out by hand rather than with a JSON parser: this is one known field
  // in one known reply, and a parser would cost far more flash than it saves
  // on a part that is already at three quarters of its partition.
  int at = reply.indexOf("\"access_token\"");
  if (at < 0) { error = "Homebridge login returned no token"; return false; }
  int start = reply.indexOf('"', reply.indexOf(':', at)) + 1;
  int end = reply.indexOf('"', start);
  if (start <= 0 || end <= start) { error = "Homebridge token could not be read"; return false; }
  hbToken = reply.substring(start, end);
  return true;
}

int homebridgeRequest(const char *method, const String &path, const String &body,
                      String &reply) {
  HTTPClient http;
  String url = normaliseHomebridgeAddress(hbAddress) + path;
  if (!http.begin(url)) return -1;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + hbToken);
  int code = strcmp(method, "GET") == 0 ? http.GET() : http.PUT(body);
  reply = http.getString();
  http.end();
  return code;
}

// Reads one characteristic's current value, needed for toggle and step.
bool homebridgeReadValue(const EspNowHomebridgePacket &cmd, float &value, String &error) {
  String reply;
  int code = homebridgeRequest("GET", String("/api/accessories/") + cmd.accessoryId, "", reply);
  if (code == 401 || code == 403) {
    if (!homebridgeLogin(error)) return false;
    code = homebridgeRequest("GET", String("/api/accessories/") + cmd.accessoryId, "", reply);
  }
  if (code != 200) { error = String("Homebridge read failed (HTTP ") + code + ")"; return false; }
  int at = reply.indexOf(cmd.characteristic);
  if (at < 0) { error = "Characteristic not found on that accessory"; return false; }
  int vpos = reply.indexOf("\"value\"", at);
  if (vpos < 0) { error = "Characteristic had no value"; return false; }
  int colon = reply.indexOf(':', vpos);
  if (colon < 0) { error = "Characteristic value was malformed"; return false; }
  String raw = reply.substring(colon + 1, colon + 24);
  raw.trim();
  if (raw.startsWith("true")) value = 1.0f;
  else if (raw.startsWith("false")) value = 0.0f;
  else value = raw.toFloat();
  return true;
}

void sendHomebridgeResult(const uint8_t *mac, bool ok, int status, const char *error) {
  EspNowHomebridgeResultPacket result = {};
  result.magic = ESPNOW_HOMEBRIDGE_RESULT_MAGIC;
  result.ok = ok ? 1 : 0;
  result.httpStatus = (int16_t)status;
  if (error) strlcpy(result.error, error, sizeof(result.error));
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (esp_now_send(mac, (const uint8_t *)&result, sizeof(result)) == ESP_OK) return;
    delay(2);
  }
}

// Stores config pushed by the remote. Called from loop() because the receive
// callback runs on the Wi-Fi task and these write NVS.
void serviceHomebridgeConfig() {
  if (pendingWifiConfigReady) {
    pendingWifiConfigReady = false;
    String ssid = pendingWifiConfig.ssid;
    String pass = pendingWifiConfig.password;
    if (ssid != hbSsid || pass != hbPassword) {
      hbSsid = ssid; hbPassword = pass;
      wifiConfigured = hbSsid.length() > 0;
      prefs.begin("dock", false);
      prefs.putString("wifiSsid", hbSsid);
      prefs.putString("wifiPass", hbPassword);
      prefs.end();
      Serial.printf("Dock: Wi-Fi details received for '%s'\n", hbSsid.c_str());
      hbToken = "";
      wifiNextAttemptMs = 0;          // Join now rather than after the retry gap.
      if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
    }
  }
  if (pendingHbConfigReady) {
    pendingHbConfigReady = false;
    String addr = pendingHbConfig.address;
    String user = pendingHbConfig.username;
    String pass = pendingHbConfig.password;
    if (addr != hbAddress || user != hbUser || pass != hbPass) {
      hbAddress = addr; hbUser = user; hbPass = pass;
      hbToken = "";                   // Credentials changed; the old token is void.
      prefs.begin("dock", false);
      prefs.putString("hbAddr", hbAddress);
      prefs.putString("hbUser", hbUser);
      prefs.putString("hbPass", hbPass);
      prefs.end();
      Serial.printf("Dock: Homebridge details received for %s\n", hbAddress.c_str());
    }
  }
}

void serviceHomebridge(unsigned long now) {
  (void)now;
  if (!pendingHomebridge) return;
  pendingHomebridge = false;
  EspNowHomebridgePacket cmd = pendingHomebridgeCommand;
  uint8_t mac[6];
  memcpy(mac, pendingHomebridgeMac, 6);

  String error;
  if (!hbAddress.length()) {
    sendHomebridgeResult(mac, false, 0, "Dock has no Homebridge details");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    sendHomebridgeResult(mac, false, 0, "Dock is not on Wi-Fi");
    return;
  }
  if (!hbToken.length() && !homebridgeLogin(error)) {
    sendHomebridgeResult(mac, false, 0, error.c_str());
    return;
  }

  // Toggle and step need the current value first, which is the whole reason
  // this work belongs on the dock: two round trips on a warm connection rather
  // than an association plus two on a cold one.
  float target = cmd.value;
  if (cmd.operation == 1 || cmd.operation == 2) {
    float current = 0.0f;
    if (!homebridgeReadValue(cmd, current, error)) {
      sendHomebridgeResult(mac, false, 0, error.c_str());
      return;
    }
    if (cmd.operation == 1) {
      target = current >= 0.5f ? 0.0f : 1.0f;
    } else {
      target = current + cmd.step;
      if (target < cmd.minimum) target = cmd.minimum;
      if (target > cmd.maximum) target = cmd.maximum;
    }
  }

  String body = String("{\"characteristicType\":\"") + cmd.characteristic + "\",\"value\":";
  if (cmd.operation == 0 && cmd.valueType == 0) {
    body += String("\"") + cmd.stringValue + "\"";
  } else if (cmd.valueType == 1 || cmd.operation == 1) {
    body += (target >= 0.5f) ? "true" : "false";
  } else {
    body += String(target, 2);
  }
  body += "}";

  String reply;
  int code = homebridgeRequest("PUT", String("/api/accessories/") + cmd.accessoryId, body, reply);
  if (code == 401 || code == 403) {
    if (!homebridgeLogin(error)) { sendHomebridgeResult(mac, false, code, error.c_str()); return; }
    code = homebridgeRequest("PUT", String("/api/accessories/") + cmd.accessoryId, body, reply);
  }
  bool ok = code >= 200 && code < 300;
  if (serialHostAttached()) {
    Serial.printf("Dock: Homebridge %s %s -> HTTP %d\n", cmd.accessoryId,
                  cmd.characteristic, code);
  }
  sendHomebridgeResult(mac, ok, code,
                       ok ? "" : (String("Homebridge returned HTTP ") + code).c_str());
}

void serviceOta(unsigned long now) {
  if (pendingOta) {
    // Copied out and the flag cleared BEFORE the handler runs, not after.
    //
    // otaHandleBegin() sends its acceptance as its last act and can sit inside
    // esp_ota_begin() for seconds before that, and pendingOta stayed set for
    // all of it - so every frame arriving in that window was refused by the
    // "still holding the last one" gate in the receive callback. The remote
    // replies to the acceptance within a millisecond or two, which lands
    // exactly in the gap between the ack going out and this line clearing the
    // flag. One dropped chunk there is fatal when the sender does not retry.
    //
    // Taking a local copy first is what makes this safe: the callback is free
    // to fill otaFrame with the next frame while this one is still being
    // written to flash, instead of being turned away.
    uint8_t frame[sizeof(otaFrame)];
    uint16_t frameLen = otaFrameLen;
    if (frameLen > sizeof(frame)) frameLen = sizeof(frame);
    memcpy(frame, otaFrame, frameLen);
    pendingOta = false;
    uint32_t magic = 0;
    memcpy(&magic, frame, sizeof(magic));
    if (magic == ESPNOW_OTA_BEGIN_MAGIC) otaHandleBegin(frame, frameLen);
    else if (magic == ESPNOW_OTA_DATA_MAGIC) otaHandleData(frame, frameLen);
    else if (magic == ESPNOW_OTA_END_MAGIC) otaHandleEnd(frame, frameLen);

    // Re-read the clock. "now" was taken at the top of loop(), and
    // otaHandleBegin() has just spent SECONDS inside esp_ota_begin() erasing
    // the partition - measured at 4.2s on a dual capture. Every timer below
    // then compared a four-second-stale "now" against a timestamp taken after
    // the erase, and because these are unsigned the subtraction underflowed to
    // an enormous number: the transfer was declared stalled five milliseconds
    // after it was accepted, before the remote could physically have replied.
    // That is the whole dock update failure. The remote was never at fault -
    // its own log showed "now is 0ms behind millis" throughout.
    now = millis();
  }

  // Nothing has arrived yet, so the acceptance ack is the only thing the remote
  // could be waiting on - and it is the one most likely to have been lost, for
  // the reasons in otaSendAck(). Repeating it costs one small packet a second
  // against a transfer that would otherwise sit dead for the full stall
  // timeout and be reported as a failure of the dock. Once a single chunk has
  // landed the remote is plainly listening and this stops.
  if (otaActive && otaWrittenBytes == 0 && (int32_t)(now - otaAckResentMs) > 1000) {
    Serial.printf("Dock: no data yet - %lu frame(s) heard from the remote, %lu OTA "
                  "frame(s) dropped as busy - repeating the acceptance ack\n",
                  (unsigned long)otaFramesSeen, (unsigned long)otaFramesDroppedBusy);
    otaSendAck(otaLastAckSeq, otaLastAckStatus);
  }

  // A sender that vanishes mid-transfer would otherwise leave the dock blinking
  // at 500ms for ever with an open OTA handle.
  if (otaActive && (int32_t)(now - otaLastChunkMs) > (int32_t)OTA_STALL_TIMEOUT_MS) {
    otaAbort("sender stopped responding", OTA_ACK_STALLED);
    enterState(DOCK_OTA_FAILED);
  }
}

void serviceIncoming() {
  if (pendingPairProof) {
    pendingPairProof = false;
    bool wasPairing = dockState == DOCK_PAIRING;
    bool isNew = !remoteKnown || memcmp(remoteMac, pendingSrcMac, 6) != 0;
    rememberRemote(pendingSrcMac, pendingChannel);
    ensurePeer(remoteMac);
    // Not during a transfer: the OTA blink is the more useful thing to be
    // showing, and this would replace it with a ten second solid.
    if ((wasPairing || isNew) && dockState != DOCK_OTA) {
      enterState(DOCK_PAIR_SUCCESS);
    }
  }

  if (!pendingCommand) return;

  if (serialHostAttached()) {
    const char *what = pendingHeader.encoding == 1 ? "RAW" : "PARSED";
    Serial.printf("Dock: IR command from %s - %s", macToString(pendingSrcMac).c_str(), what);
    if (pendingHeader.encoding == 1) {
      Serial.printf(", %u timing(s), %u kHz\n", (unsigned)pendingTimingCount,
                    (unsigned)pendingHeader.frequencyKhz);
    } else {
      Serial.printf(" %s addr=0x%08lX cmd=0x%08lX\n", pendingHeader.protocol,
                    (unsigned long)pendingHeader.address,
                    (unsigned long)pendingHeader.command);
    }
  }

  // Not while the pairing LED is mid-story - the flash would be indistinguishable
  // from the blink and the 10 second solid is the more useful signal.
  if (dockState == DOCK_IDLE) {
    sendCommand(pendingHeader, pendingTimings, pendingTimingCount);
  }

  pendingCommand = false;
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  // THE fix for "responsive only while the serial monitor is open".
  //
  // This board has no USB-serial bridge, so Serial is HWCDC - the C3's own USB
  // Serial/JTAG peripheral - and HWCDC::write() WAITS for the host to drain
  // the buffer, up to tx_timeout_ms, which defaults to 100. With a monitor
  // attached the host drains instantly and every write returns at once. With
  // nothing attached there is no reader, so each write stalls for the full
  // timeout.
  //
  // The dock prints two lines for every IR command received, so with the
  // monitor closed each command cost roughly 200ms of dead loop time - and
  // ESP-NOW frames arriving during that stall hit the "still working on the
  // last one" gate in onEspNowRecv() and were dropped. That is the sluggish,
  // mostly-unresponsive behaviour, and it had nothing to do with radio power
  // save or range: closing the monitor did not put the dock to sleep, it made
  // every log line block.
  //
  // Zero means never wait: if no host is reading, the line is discarded and
  // the loop carries on. Logging is a diagnostic; responsiveness is the
  // product.
  Serial.setTxTimeoutMs(0);
  // USB CDC needs the host to have opened the port before it will keep
  // anything; 200ms was not always enough and the first lines were lost.
  delay(600);
  Serial.println();
  Serial.printf("OpenRemote Dock %s - ESP32-C3 Super Mini\n",
                OPENREMOTE_DOCK_VERSION_STRING);
  Serial.printf("%s\n", OPENREMOTE_DOCK_MARKER);
  Serial.printf("LED: GPIO%d (plain, active %s), button: GPIO%d\n",
                DOCK_LED_PIN, DOCK_LED_ACTIVE_LOW ? "low" : "high",
                DOCK_BUTTON_PIN);
#if DOCK_AUX_LED_PIN >= 0
  Serial.printf("PCB LED: GPIO%d (active %s), menu button: GPIO%d\n",
                DOCK_AUX_LED_PIN,
                DOCK_AUX_LED_ACTIVE_LOW ? "low" : "high",
                DOCK_MENU_BUTTON_PIN);
#endif

  // Full clock, always. The C3's dynamic frequency scaling can drop the CPU to
  // a low idle frequency, and IR transmission here is bit-banged at
  // microsecond timings - it wants a stable, known clock. Pinned before
  // anything time-sensitive runs.
  setCpuFrequencyMhz(160);
  Serial.printf("CPU: %u MHz (fixed)\n", (unsigned)getCpuFrequencyMhz());

  pinMode(DOCK_BUTTON_PIN, INPUT_PULLUP);
#if DOCK_MENU_BUTTON_PIN >= 0
  pinMode(DOCK_MENU_BUTTON_PIN, INPUT_PULLUP);
#endif
  ledSetup();
#if DOCK_IR_LED_PIN >= 0
  IrSender.begin(DOCK_IR_LED_PIN);
  // IRremote blinks a "feedback LED" on every send, and with no pin given it
  // uses LED_BUILTIN - which this board's variant defines as GPIO8, the very
  // pin the dock LED is on. Worse, it drives it with direct register writes
  // that leave the pin detached from the GPIO matrix, so every subsequent
  // digitalWrite() was rejected outright:
  //   [E][esp32-hal-gpio.c:185] IO 8 is not set as GPIO
  // Seven of those per command - one for each pulse step. The LED code was
  // running correctly the whole time and simply not reaching the pin.
  disableLEDFeedback();
  Serial.printf("IR emitter: GPIO%d (IRremote LED feedback disabled - it would "
                "claim GPIO%d)\n", (int)DOCK_IR_LED_PIN, (int)LED_BUILTIN);
#else
  Serial.println("IR emitter: disabled");
#endif
#if DOCK_RF_CS_PIN >= 0
  rfPresent = rfInit();
  if (rfPresent) {
    Serial.printf("RF433: CC1101 found - SCK %d, MISO %d, MOSI %d, CSN %d, GDO0 %d "
                  "at %.2f MHz\n", (int)DOCK_RF_SCK_PIN, (int)DOCK_RF_MISO_PIN,
                  (int)DOCK_RF_MOSI_PIN, (int)DOCK_RF_CS_PIN, (int)DOCK_RF_GDO0_PIN,
                  (double)DOCK_RF_FREQ_MHZ);
  } else {
    Serial.println("RF433: no CC1101 answered on the SPI pins - check wiring and "
                   "that VCC is 3V3, not 5V");
  }
#else
  Serial.println("RF433: disabled in this build");
#endif

  loadRemote();
  radioSetup();

  // One short blink says the firmware is alive and the LED pin is right.
  ledWrite(true);
  delay(120);
  ledWrite(false);
}

void loop() {
  unsigned long now = millis();
  serviceButton(now);
  servicePairing(now);
  serviceChannelMove();
  serviceHomebridgeConfig();
  serviceHomebridgeWifi(now);
  serviceHomebridge(now);
  serviceOta(now);
  serviceRfLearn(now);
  serviceSettings();
  serviceInfoReply();
  serviceLinkLed(now);
  serviceStateLog(now);
  serviceIncoming();
  serviceLedStates(now);
  delay(2);
}
