# OpenRemote Software

This is the canonical home for OpenRemote software and release files.

## Folder policy

- `FIRMWARE/BIN`: firmware and generated binary images only.
- `FIRMWARE/Other`: source release archives, runtime snapshots and old build output.
- `Platformio:Arduino`: active firmware source projects and reference projects.
- `OpenRemote Studio/Mac`: one ready-to-run `.app` for each retained Mac release.
- `OpenRemote Studio/Windows`: the current `.exe` plus its required portable runtime.
- `OpenRemote Studio/Other`: Studio source, packaging work, old releases and the IRDB.
- `WebConfig`: standalone HTML releases named `WebConfig X.XX.html` (for example, `WebConfig 2.04.html`).
- `Sensor_Test`: the active sensor-test source project.
- `SD Card Structure`: clean folders and factory assets for preparing an SD card.
- `Archive`: historical SD-card packages and retired folder layouts.

New release files should be placed according to this policy so the project root stays clean.
New WebConfig releases must use only the simple versioned filename; feature descriptions belong in the changelog, not the filename.
