# OpenRemote

A universal remote built on the ESP32-S3, with a mains-powered dock that
extends its reach. A fork of the OMOTE hardware that has since grown its own
firmware, configurator and desktop tooling.

| Component | Version | Source |
|---|---|---|
| Remote firmware — ESP32-S3 | **4.02** | [`remote/`](remote/) |
| Dock firmware — ESP32-C3 | **1.30** | [`dock/`](dock/) |
| WebConfig — browser configurator | **2.56** | [`webconfig/`](webconfig/) |
| OpenRemote Studio — desktop app | **2.76** | [`studio/`](studio/) |

## Layout

    remote/       ESP32-S3 remote firmware (PlatformIO)
    dock/         ESP32-C3 dock firmware (PlatformIO) - see dock/README.md
    webconfig/    single-file HTML configurator, served off the remote's SD card
    studio/       desktop app for flashing and SD setup (Mac, Windows, Linux)
    sd-card/      template of the remote's SD card layout
    tools/        build helpers
    docs/         handoff notes and configuration backups
    releases/     built firmware images, one archived per version (not in git)
    archive/      superseded projects, kept rather than deleted

## Why one repository

The remote and the dock share an ESP-NOW wire format, and every shared struct
is pinned by `static_assert` in **both** firmwares. A field added to one and
not the other fails the build instead of letting the two misread each other on
air. Versioning them together is what makes that guarantee hold — they cannot
drift apart unnoticed.

## What is not in git, and why

**Compiled firmware** (`releases/`) — build products of the source here, about
1.8GB across every archived version, and they do not delta-compress. Each
version is archived on disk and mirrored to the NAS instead.

**Built desktop apps** (`studio/Mac/*.app`, `*.AppImage`) — 85MB per bundle,
32 of them. The previous standalone Studio repository tracked 23,392 files from
inside these bundles, which is why its history alone came to 237MB.

Every **WebConfig** version *is* tracked, because successive versions are nearly
identical and 120MB of them packs down to about 1MB.

## Getting started

    cd remote && pio run -t upload      # flash the remote
    cd dock/firmware && pio run -t upload   # flash a dock over USB

After a dock is paired once, later dock updates go over the air from WebConfig.

## Related

Upstream hardware: [OMOTE](https://github.com/OMOTE-Community/OMOTE-Firmware),
tracked as the `upstream` remote.
