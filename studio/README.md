# OpenRemote Studio

Desktop companion app for the OpenRemote firmware in the parent folder. Builds an
IR code database (`OpenRemote.irdb`) for the remote's SD card, flashes firmware and
factory-prepares a fresh SD card over USB, and provides a USB recovery path when
the remote's own WebConfig page can't be reached.

- `openremote_studio.py` - the app: a local HTTP server plus the logic behind it
  (IRDB building, USB serial protocol to the remote, firmware flashing via esptool).
- `studio.html` - the UI, served by the app.
- `OpenRemote.png`, `openremote_product.png`, `OpenRemoteIcon.png`, `OpenRemoteIcon.icns`
  - app branding/icon assets.

## Running from source

```
pip install -r requirements.txt --target vendor
python3 openremote_studio.py
```

The app opens as a native window (via `pywebview`, using the OS's own web engine -
WKWebView on Mac, WebView2 on Windows) rather than a browser tab, with a graceful
fallback to a plain browser tab if the native window backend can't start on a given
machine.

Built app packages (`.app` for Mac, `.exe` for Windows) are not tracked in this repo
- only the source. See the project's Releases for compiled builds.
