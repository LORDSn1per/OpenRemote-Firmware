#!/usr/bin/env python3
from __future__ import annotations
import contextlib, datetime as dt, glob, hashlib, io, json, os, re, select, shutil, socket, sqlite3, struct, subprocess, sys, threading, time, urllib.parse, urllib.request, webbrowser, zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

APP_VERSION="2.77"
SERIAL_BAUD=460800
# 2.68 adds Linux as a third supported platform. Until now every non-Windows
# branch in this file assumed macOS outright - AppleScript dialogs, diskutil,
# ~/Library paths, /dev/cu.* port globs. Those are now selected by platform
# rather than by "not Windows", with Linux equivalents beside them. The macOS
# and Windows paths are unchanged.
IS_WINDOWS=os.name=="nt"
IS_MAC=sys.platform=="darwin"
IS_LINUX=sys.platform.startswith("linux")
APP_DIR=Path(getattr(sys,"_MEIPASS",Path(__file__).resolve().parent))
VENDOR_DIR=APP_DIR/"vendor"
if VENDOR_DIR.exists(): sys.path.insert(0,str(VENDOR_DIR))
FACTORY_DIR=APP_DIR/"factory"
FACTORY_MANIFEST_PATH=FACTORY_DIR/"manifest.json"
FACTORY_SD_TEMPLATE=FACTORY_DIR/"sd_template"

if os.name=="nt":
    import msvcrt
    try:
        import serial
        import serial.tools.list_ports
    except ImportError:
        serial=None
    DATA_DIR=Path(os.environ.get("APPDATA",str(Path.home())))/"OpenRemote Studio"
    CACHE_DIR=Path(os.environ.get("LOCALAPPDATA",str(DATA_DIR)))/"OpenRemote Studio"/"Cache"
elif IS_LINUX:
    import fcntl
    # The POSIX serial path below talks to the port through raw termios, not
    # pyserial, so this stays None on Linux exactly as it does on macOS.
    # pyserial is still vendored because esptool imports it directly.
    serial=None
    # XDG Base Directory spec, with the documented fallbacks. A distro that
    # sets neither variable still lands on the standard ~/.local/share and
    # ~/.cache locations.
    DATA_DIR=Path(os.environ.get("XDG_DATA_HOME") or Path.home()/".local"/"share")/"OpenRemoteStudio"
    CACHE_DIR=Path(os.environ.get("XDG_CACHE_HOME") or Path.home()/".cache")/"OpenRemoteStudio"
else:
    import fcntl
    serial=None
    DATA_DIR=Path.home()/"Library"/"Application Support"/"OpenRemote Studio"
    CACHE_DIR=Path.home()/"Library"/"Caches"/"OpenRemoteStudio"
DATA_DIR.mkdir(parents=True,exist_ok=True)
ACTIVE_DB=DATA_DIR/"OpenRemote.irdb"
INSTANCE_STATE=DATA_DIR/"instance.json"
INSTANCE_LOCK_PATH=DATA_DIR/"instance.lock"

REPOS=[
{"name":"Lucaslhm-Flipper-IRDB","owner":"Lucaslhm","repo":"Flipper-IRDB","branch":"main","type":"flipper_ir"},
{"name":"probonopd-irdb","owner":"probonopd","repo":"irdb","branch":"master","type":"irdb"},
{"name":"Flipper-Firmware-Official","owner":"flipperdevices","repo":"flipperzero-firmware","branch":"dev","type":"flipper_ir"},
{"name":"probonopd-lirc-remotes","owner":"probonopd","repo":"lirc-remotes","branch":"master","type":"lirc"}]
IGNORE={".git",".github","__pycache__","docs","documentation","images","img","metadata","tools","scripts","test","tests"}
ALIASES={"tv":"TV","tvs":"TV","television":"TV","televisions":"TV","audio":"Audio","receiver":"Audio","receivers":"Audio","avr":"Audio","soundbar":"Audio","soundbars":"Audio","projector":"Projector","projectors":"Projector","set top box":"Set-top Box","set_top_box":"Set-top Box","stb":"Set-top Box","air conditioner":"Air Conditioner","air_conditioner":"Air Conditioner","ac":"Air Conditioner","dvd":"Disc Player","bluray":"Disc Player","blu-ray":"Disc Player","streaming":"Streaming"}
STATE={"running":False,"done":False,"error":"","progress":0,"downloaded_mb":0.0,"current":"Ready.","log":["Ready."],"records":0,"bin_mb":0.0,"release_path":"","db_path":str(ACTIVE_DB) if ACTIVE_DB.exists() else ""}
USB_WEBCONFIG_CACHE={"html":b"","loaded_at":0,"port":"","size":0}
USB_TRANSFER_STATE={"active":False,"received":0,"total":0,"error":""}
FACTORY_STATE={"running":False,"operation":"","view":"setup","progress":0,"status":"Ready to set up a new remote.","error":"","done":False,"log":[]}
# Progress for "Install WebConfig over USB". A WebConfig HTML is well over a
# megabyte and the USB transfer is acknowledged window-by-window, so it needs
# real byte progress rather than a spinner.
FACTORY_THREAD=None
WEBCONFIG_STATE={"running":False,"done":False,"error":"","sent":0,"total":0,"status":"Ready."}
SETUP_SELECTION={"boardDetected":False,"detectedPort":"","firmwareInstalled":False,
                 "firmware":None,"webConfig":None}
USB_LOCK=threading.Lock()
INSTANCE_LOCK=None
APP_SERVER=None
CLIENT_LAST_SEEN=time.monotonic()
CLIENT_SEEN=False

def log(msg):
    print(msg,flush=True); STATE["current"]=msg; STATE["log"].append(msg)
    if len(STATE["log"])>1000: STATE["log"]=STATE["log"][-1000:]
def nice(s): return re.sub(r"\s+"," ",Path(s).stem.replace("_"," ").replace("-"," ")).strip() or "Unknown"
def norm_cat(s): return ALIASES.get(nice(s).lower(),nice(s).title())
def hash_file(path,algo="sha1"):
    h=getattr(hashlib,algo)()
    with open(path,"rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""): h.update(chunk)
    return h.hexdigest()

# The two devices Studio can flash, and how to tell them apart. Everything that
# decides "may this image go on this board" reads from here.
CHIP_ID_S3=9
CHIP_ID_C3=5
CHIP_NAMES={CHIP_ID_S3:"ESP32-S3",CHIP_ID_C3:"ESP32-C3"}
DEVICE_SPECS={
    "remote":{"label":"OpenRemote remote","chip":"ESP32-S3","esptool_chip":"esp32s3",
              "chip_id":CHIP_ID_S3,"app_partition":0x330000,
              "marker":"OPENREMOTE_FIRMWARE_VERSION"},
    "dock":  {"label":"OpenRemote dock","chip":"ESP32-C3","esptool_chip":"esp32c3",
              "chip_id":CHIP_ID_C3,"app_partition":0x140000,
              "marker":"OPENREMOTE_DOCK_VERSION"},
}

def factory_manifest():
    if not FACTORY_MANIFEST_PATH.exists():
        raise RuntimeError("The bundled OpenRemote factory manifest is missing.")
    return json.loads(FACTORY_MANIFEST_PATH.read_text(encoding="utf-8"))

def inspect_firmware_file(path):
    path=Path(path).expanduser().resolve()
    if path.suffix.lower()!=".bin" or not path.is_file():
        raise RuntimeError("Choose an OpenRemote .bin firmware file.")
    data=path.read_bytes()
    merged=(len(data)>0x10020 and data[0]==0xE9 and
            data[0x8000:0x8002]==b"\xAA\x50" and data[0x10000]==0xE9)
    app_offset=0x10000 if merged else 0
    if len(data)<app_offset+24 or data[app_offset]!=0xE9:
        raise RuntimeError("The selected file is not an ESP32 firmware image.")
    # Which device this image is for, decided from the image itself rather than
    # its filename. Two independent signals have to agree: the chip the image
    # was built for, which is in the ESP32 header, and the version marker the
    # firmware embeds. The remote is an ESP32-S3 carrying
    # OPENREMOTE_FIRMWARE_VERSION=, the dock an ESP32-C3 carrying
    # OPENREMOTE_DOCK_VERSION=. Neither can be faked by renaming a file, which
    # is the point: writing remote firmware into a dock would brick it until it
    # was re-flashed over USB.
    chip_id=int.from_bytes(data[app_offset+12:app_offset+14],"little")
    dock_marker=re.search(rb"OPENREMOTE_DOCK_VERSION=([0-9]+(?:\.[0-9]+)+)",data)
    remote_marker=re.search(rb"OPENREMOTE_FIRMWARE_VERSION=([0-9]+(?:\.[0-9]+)+)",data)
    if dock_marker and remote_marker:
        raise RuntimeError("This file carries both a remote and a dock version marker; Studio cannot tell what it is.")
    if dock_marker: device="dock"
    elif remote_marker: device="remote"
    elif chip_id==CHIP_ID_C3: device="dock"
    elif chip_id==CHIP_ID_S3: device="remote"
    else:
        raise RuntimeError("The selected firmware is not built for an ESP32-S3 or ESP32-C3.")
    spec=DEVICE_SPECS[device]
    if chip_id!=spec["chip_id"]:
        raise RuntimeError("This looks like "+spec["label"]+" firmware but it is built for "
                           +(CHIP_NAMES.get(chip_id) or ("chip id "+str(chip_id)))
                           +" rather than "+spec["chip"]+". Studio will not install it.")
    app_size=len(data)-app_offset
    if app_size>spec["app_partition"]:
        raise RuntimeError("The selected firmware is too large for the "+spec["label"]+" application partition.")
    marker=dock_marker or remote_marker
    versions=[] if marker else [m.decode("ascii") for m in re.findall(rb"(?:^|\x00)([0-9]+\.[0-9]{2})(?=\x00|$)",data)]
    filename_match=re.search(r"(?:openremote|firmware)[^0-9]*([0-9]+(?:\.[0-9]+)+)",path.stem,re.I)
    version=(marker.group(1).decode("ascii") if marker else
             (versions[-1] if versions else (filename_match.group(1) if filename_match else "")))
    if not version:
        raise RuntimeError("Studio could not find an OpenRemote version inside this firmware file.")
    return {"path":str(path),"name":path.name,"version":version,
            "kind":"factory" if merged else "application","bytes":len(data),
            "appBytes":app_size,"sha256":hashlib.sha256(data).hexdigest(),"bundled":False,
            "device":device,"deviceLabel":spec["label"],"chip":spec["chip"]}

def inspect_webconfig_file(path):
    path=Path(path).expanduser().resolve()
    if path.suffix.lower() not in (".html",".htm") or not path.is_file():
        raise RuntimeError("Choose an OpenRemote WebConfig .html file.")
    size=path.stat().st_size
    if size<256 or size>4*1024*1024:
        raise RuntimeError("The selected WebConfig file has an unexpected size.")
    data=path.read_bytes()
    text=data[:256*1024].decode("utf-8","ignore")
    match=re.search(r'<meta\s+name=["\']openremote-webconfig-version["\']\s+content=["\']([^"\']+)',text,re.I)
    if not match:
        match=re.search(r'OpenRemote\s+Web\s+Config\s+v([0-9]+(?:\.[0-9]+)+)',text,re.I)
    if not match or "<html" not in text.lower():
        raise RuntimeError("This is not a versioned OpenRemote WebConfig HTML file.")
    return {"path":str(path),"name":path.name,"version":match.group(1),
            "bytes":size,"sha256":hashlib.sha256(data).hexdigest(),"bundled":False}

def require_setup_files(include_webconfig=False):
    if not SETUP_SELECTION.get("firmware"):
        raise RuntimeError("Choose the OpenRemote firmware .bin file first.")
    if include_webconfig and not SETUP_SELECTION.get("webConfig"):
        raise RuntimeError("Choose the OpenRemote WebConfig .html file first.")
    return SETUP_SELECTION

def setup_info():
    return {"ok":True,**factory_manifest(),"selection":SETUP_SELECTION}

def selected_application_bytes():
    require_setup_files()
    info=SETUP_SELECTION["firmware"]
    data=Path(info["path"]).read_bytes()
    return data[0x10000:] if info["kind"]=="factory" else data

def selected_factory_image():
    require_setup_files()
    info=inspect_firmware_file(SETUP_SELECTION["firmware"]["path"])
    expected=SETUP_SELECTION["firmware"].get("sha256","")
    if info["sha256"]!=expected:
        raise RuntimeError("The selected firmware changed after it was chosen. Choose it again.")
    if info["kind"]=="factory": return info,Path(info["path"]),"0x0"
    # Each device gets its OWN bootstrap. The remote's is an ESP32-S3
    # bootloader with a 16MB partition map; prepending that to a dock image
    # would produce an ESP32-C3 that does not boot at all.
    if info.get("device")=="dock":
        _,dock_bootstrap=verify_dock_bootstrap()
        prefix=dock_bootstrap.read_bytes()[:0x10000]
        image_path=CACHE_DIR/("OpenRemote_Dock_"+re.sub(r"[^0-9A-Za-z._-]","_",info["version"])+"_factory.bin")
        image_path.parent.mkdir(parents=True,exist_ok=True)
        image_path.write_bytes(prefix+Path(info["path"]).read_bytes())
        return info,image_path,"0x0"
    _,bootstrap=verify_factory_image()
    prefix=bootstrap.read_bytes()[:0x10000]
    image_path=CACHE_DIR/("OpenRemote_"+re.sub(r"[^0-9A-Za-z._-]","_",info["version"])+"_factory.bin")
    image_path.parent.mkdir(parents=True,exist_ok=True)
    image_path.write_bytes(prefix+Path(info["path"]).read_bytes())
    # Bootstrap prepended, so this one starts at 0x0 like a factory image.
    return info,image_path,"0x0"

def factory_job_running():
    """True only if a setup job is genuinely still executing.

    FACTORY_STATE["running"] used to be trusted on its own, so a worker that
    died without clearing it latched the flag on forever: every later attempt
    was refused with 409 and the front end's poller kept re-disabling the
    Install button, which is why a failed run could only be recovered by
    restarting Studio. Checking the thread is alive makes a stale flag
    self-healing.
    """
    if not FACTORY_STATE.get("running"): return False
    if FACTORY_THREAD is None: return False
    if not FACTORY_THREAD.is_alive():
        FACTORY_STATE.update({"running":False})
        return False
    return True

def start_factory_job(target,args):
    global FACTORY_THREAD
    FACTORY_THREAD=threading.Thread(target=target,args=args,daemon=True)
    FACTORY_THREAD.start()
    return FACTORY_THREAD

def reset_factory_state(operation,status,view="setup"):
    """view is the tab that started the job.

    Setup and Recovery share this one job channel, and the UI used to guess
    which tab's widgets to update - which leaked a Recovery job's progress
    bar, status line and log into the Setup tab three separate times. The
    owner is now recorded here and the front end renders only that view.
    """
    FACTORY_STATE.update({
        "running":True,"operation":operation,"progress":0,"status":status,
        "view":view,"error":"","done":False,"log":[],
        "filesInstalled":0,"bytesInstalled":0,"filesSkipped":0,
        "bytesSent":0,"bytesTotal":0,
        "foldersCreated":0,"foldersTotal":0,"cardMb":0
    })

def factory_log(message):
    message=re.sub(r"\x1b\[[0-9;?]*[A-Za-z]","",str(message or "")).strip()
    if not message: return
    FACTORY_STATE["status"]=message
    FACTORY_STATE["log"].append(message)
    FACTORY_STATE["log"]=FACTORY_STATE["log"][-120:]
    matches=re.findall(r"(?:\(|\s)(\d{1,3})\s*%",message)
    if matches:
        value=max(0,min(100,int(matches[-1])))
        FACTORY_STATE["progress"]=max(FACTORY_STATE["progress"],5+int(value*0.9))

class FactoryConsole(io.TextIOBase):
    def __init__(self): self.pending=""
    def writable(self): return True
    def isatty(self): return False
    def write(self,text):
        self.pending+=str(text).replace("\r","\n")
        parts=self.pending.split("\n")
        self.pending=parts.pop()
        for part in parts: factory_log(part)
        return len(text)
    def flush(self):
        if self.pending:
            factory_log(self.pending)
            self.pending=""

# esptool's output has to reach the live console AND be readable afterwards.
class Tee:
    def __init__(self,*streams): self.streams=streams
    def write(self,text):
        for stream in self.streams: stream.write(text)
        return len(text)
    def flush(self):
        for stream in self.streams:
            if hasattr(stream,"flush"): stream.flush()


def run_esptool(arguments):
    try:
        import esptool
    except Exception as error:
        raise RuntimeError("The bundled ESP32 flashing engine could not load: "+str(error))
    output=FactoryConsole()
    captured=io.StringIO()
    try:
        with contextlib.redirect_stdout(Tee(output,captured)),contextlib.redirect_stderr(Tee(output,captured)):
            esptool.main(arguments)
    except SystemExit as error:
        code=error.code if isinstance(error.code,int) else 1
        if code: raise RuntimeError("ESP32 flashing engine stopped with error "+str(code)+".")
    finally:
        output.flush()
    return captured.getvalue()


# esptool announces the part it found as "Chip is ESP32-C3 (revision ...)".
# Reading that back is how Studio knows whether a remote or a dock is plugged
# in, which is what makes refusing the wrong firmware possible at all.
def identify_chip(esptool_output):
    text=esptool_output or ""
    for device,spec in DEVICE_SPECS.items():
        if re.search(r"Chip is\s+"+re.escape(spec["chip"])+r"\b",text,re.I):
            return device,spec["chip"]
    found=re.search(r"Chip is\s+([A-Za-z0-9\-]+)",text,re.I)
    name=found.group(1) if found else "an unrecognised chip"
    raise RuntimeError("Studio found "+name+" on this port. It flashes the OpenRemote "
                       "remote (ESP32-S3) and dock (ESP32-C3) only.")

def verify_factory_image():
    manifest=factory_manifest()
    image=FACTORY_DIR/str(manifest.get("factoryImage", ""))
    if not image.is_file(): raise RuntimeError("The bundled OpenRemote factory image is missing.")
    expected=str(manifest.get("factoryImageSha256","")).lower()
    actual=hash_file(image,"sha256")
    if not expected or actual!=expected:
        raise RuntimeError("The bundled firmware failed its safety checksum and will not be flashed.")
    return manifest,image

def verify_dock_bootstrap():
    """The bootloader, partition table and boot_app0 a blank ESP32-C3 needs.

    Held separately so the dock, like the remote, ships as a single
    application-only .bin. Studio prepends this for a USB install, which sets
    up a blank board; the same file sent over ESP-NOW goes into the dock's OTA
    partition and keeps its pairing. One file, two outcomes - exactly how the
    remote already behaves."""
    manifest=factory_manifest()
    image=FACTORY_DIR/str(manifest.get("dockBootstrapImage",""))
    if not image.is_file(): raise RuntimeError("The bundled dock bootstrap is missing.")
    expected=str(manifest.get("dockBootstrapSha256","")).lower()
    actual=hash_file(image,"sha256")
    if not expected or actual!=expected:
        raise RuntimeError("The bundled dock bootstrap failed its safety checksum and will not be flashed.")
    return manifest,image

def verify_sensor_test_image():
    manifest=factory_manifest()
    image=FACTORY_DIR/str(manifest.get("sensorTestImage", ""))
    if not image.is_file():
        raise RuntimeError("The bundled Sensor Test image is missing.")
    expected=str(manifest.get("sensorTestImageSha256","")).lower()
    actual=hash_file(image,"sha256")
    if not expected or actual!=expected:
        raise RuntimeError("The bundled Sensor Test failed its safety checksum and will not be flashed.")
    expected_bytes=int(manifest.get("sensorTestImageBytes",0) or 0)
    if expected_bytes and image.stat().st_size!=expected_bytes:
        raise RuntimeError("The bundled Sensor Test size does not match its manifest.")
    return manifest,image

def detect_factory_board(port):
    port=normalize_usb_port((port or "").strip())
    if not port: raise RuntimeError("Choose the ESP32 USB serial port first.")
    SETUP_SELECTION.update({"boardDetected":False,"detectedPort":"","firmwareInstalled":False,
                            "detectedDevice":"","detectedChip":""})
    reset_factory_state("detect","Checking the USB board...","setup")
    try:
        # Ask the board what it is rather than telling it. Studio flashes two
        # different chips now, and hardcoding esp32s3 here meant a dock either
        # failed to detect or - worse - detected as something it is not.
        with USB_LOCK:
            output=run_esptool(["--chip","auto","--port",port,"--baud","115200",
                                "--before","default_reset","--after","no_reset","chip_id"])
        device,chip=identify_chip(output)
        SETUP_SELECTION.update({"boardDetected":True,"detectedPort":port,
                                "detectedDevice":device,"detectedChip":chip})
        label=DEVICE_SPECS[device]["label"]
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,
                              "view":"dock" if device=="dock" else "setup",
                              "status":chip+" detected - this is "+label+"."})
        return {"ok":True,"port":port,"chip":chip,"device":device,
                "deviceLabel":label,"selection":SETUP_SELECTION}
    except Exception as error:
        message=(str(error)+" Studio could not enter programming mode automatically. "
                 "Tap the remote's Reset button once, wait a moment, then press Detect again.")
        FACTORY_STATE.update({"running":False,"done":False,"error":message,"status":"Board not detected."})
        raise RuntimeError(message)

def flash_remote_application(port, payload):
    """Update a remote's firmware over USB WITHOUT touching its settings.

    The distinction from flash_factory_board() is the whole point of this
    function. Setting up a blank board writes a merged image from 0x0, which
    runs straight through the NVS partition at 0x9000-0xE000 and erases every
    setting, saved Wi-Fi network and Bluetooth bond on the way past. That is
    correct for a bare board and wrong for an update, and it is exactly how an
    update here used to cost a full reconfiguration.

    Writing the application alone at 0x10000 leaves the bootloader, the
    partition table and NVS untouched, so the remote comes back up with
    everything it had. Same sequence the NAS flash-firmware.command has been
    using successfully.
    """
    port=normalize_usb_port((port or "").strip())
    if not port: raise RuntimeError("Choose the remote's USB serial port first.")

    CACHE_DIR.mkdir(parents=True,exist_ok=True)
    staged=CACHE_DIR/"recovery-firmware.bin"
    staged.write_bytes(payload)
    try:
        info=inspect_firmware_file(staged)
    except Exception:
        staged.unlink(missing_ok=True)
        raise
    # A dock image is an ESP32-C3 build and would leave the remote dead until
    # someone re-flashed it. The two .bin files look alike in a file picker,
    # which is precisely why this is checked rather than trusted.
    if info.get("device")!="remote":
        staged.unlink(missing_ok=True)
        raise RuntimeError("That is "+DEVICE_SPECS[info.get("device","dock")]["label"]
                           +" firmware. This installs remote firmware only.")
    # A merged image carries bootloader and partition table in its first
    # 0x10000 bytes. Those are the parts that must NOT be rewritten here, so
    # the application is lifted out and the rest discarded - the user gets the
    # settings-preserving install they asked for whichever of the two files
    # they happened to pick.
    data=staged.read_bytes()
    if info.get("kind")=="factory":
        data=data[0x10000:]
        staged.write_bytes(data)

    reset_factory_state("flash-app","Connecting to the ESP32-S3 bootloader...","recovery")
    FACTORY_STATE["progress"]=2
    try:
        manifest=factory_manifest()
        with USB_LOCK:
            run_esptool([
                "--chip","esp32s3","--port",port,"--baud",str(SERIAL_BAUD),
                "--before","default_reset","--after","hard_reset","write_flash",
                "--flash_mode",str(manifest.get("flashMode","dio")),
                "--flash_freq",str(manifest.get("flashFrequency","80m")),
                "--flash_size",str(manifest.get("flashSize","16MB")),
                "0x10000",str(staged)
            ])
            # Without this the bootloader keeps booting whichever slot otadata
            # still points at, and the freshly written image is ignored - the
            # remote restarts on its old firmware and the update looks like it
            # silently did nothing.
            factory_log("Clearing otadata so the board boots the image just written...")
            run_esptool([
                "--chip","esp32s3","--port",port,"--baud",str(SERIAL_BAUD),
                "--before","default_reset","--after","hard_reset",
                "erase_region","0xe000","0x2000"
            ])
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,
            "status":"Firmware "+str(info.get("version"))+" installed. Settings, Wi-Fi "
                     "and Bluetooth pairings were not touched."})
    except Exception as error:
        message=(str(error)+" Keep the USB cable connected. Tap the remote's Reset "
                 "button once, wait a moment, then try again.")
        FACTORY_STATE.update({"running":False,"done":False,"error":message,
                              "status":"Firmware installation failed."})
    finally:
        staged.unlink(missing_ok=True)

def flash_factory_board(port):
    port=normalize_usb_port((port or "").strip())
    if not port: raise RuntimeError("Choose the ESP32 USB serial port first.")
    if not SETUP_SELECTION.get("boardDetected") or normalize_usb_port(SETUP_SELECTION.get("detectedPort",""))!=port:
        raise RuntimeError("Check this board before installing firmware.")
    firmware,image,offset=selected_factory_image()
    # The check the whole exercise exists for. Both halves were established
    # independently - the board said what chip it is, the image says what it is
    # built for - so a mismatch is refused before a single byte is written.
    # Writing remote firmware into a dock leaves it dead until someone
    # re-flashes it over USB, and the two .bin files look alike in a file
    # picker.
    board_device=SETUP_SELECTION.get("detectedDevice") or "remote"
    image_device=firmware.get("device") or "remote"
    if board_device!=image_device:
        raise RuntimeError(
            "That is "+DEVICE_SPECS[image_device]["label"]+" firmware ("
            +DEVICE_SPECS[image_device]["chip"]+") and the board plugged in is "
            +DEVICE_SPECS[board_device]["label"]+" ("+DEVICE_SPECS[board_device]["chip"]
            +"). Studio will not install it. Choose the "
            +DEVICE_SPECS[board_device]["label"]+" firmware instead.")
    spec=DEVICE_SPECS[board_device]
    reset_factory_state("flash","Connecting to the "+spec["chip"]+" bootloader...",
                        "dock" if board_device=="dock" else "setup")
    FACTORY_STATE["progress"]=2
    try:
        manifest=factory_manifest()
        # The offset comes from selected_factory_image(), which is the only
        # place that knows what it actually built: 0x0 for anything carrying a
        # bootloader, 0x10000 for a bare dock application image. That one goes
        # above the bootloader and partition table and therefore above NVS at
        # 0x9000, so an update keeps the dock's pairing - which matters now that
        # pairing is a precondition of updating over the air.
        flash_size="4MB" if board_device=="dock" else str(manifest.get("flashSize","16MB"))
        flash_freq="80m" if board_device=="dock" else str(manifest.get("flashFrequency","80m"))
        with USB_LOCK:
            run_esptool([
                "--chip",spec["esptool_chip"],"--port",port,"--baud",str(SERIAL_BAUD),
                "--before","default_reset","--after","hard_reset","write_flash",
                "--flash_mode",str(manifest.get("flashMode","dio")),
                "--flash_freq",flash_freq,
                "--flash_size",flash_size,
                offset,str(image)
            ])
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,
            "status":spec["label"].capitalize()+" firmware "+str(firmware.get("version"))
                     +" installed and verified."})
        SETUP_SELECTION["firmwareInstalled"]=True
    except Exception as error:
        SETUP_SELECTION["firmwareInstalled"]=False
        message=(str(error)+" Keep the USB cable connected. Tap the remote's Reset button once, "
                 "wait a moment, then press Install again.")
        FACTORY_STATE.update({"running":False,"done":False,"error":message,"status":"Firmware installation failed."})

def flash_sensor_test_board(port):
    port=normalize_usb_port((port or "").strip())
    if not port: raise RuntimeError("Choose the ESP32 USB serial port first.")
    if not SETUP_SELECTION.get("boardDetected") or normalize_usb_port(SETUP_SELECTION.get("detectedPort",""))!=port:
        raise RuntimeError("Check this board before installing Sensor Test.")
    # Sensor Test exercises the remote's display, SD card and microphone. None
    # of that exists on a dock, and the image is built for the wrong chip.
    if (SETUP_SELECTION.get("detectedDevice") or "remote")!="remote":
        raise RuntimeError("Sensor Test is for the OpenRemote remote. The board plugged in "
                           "is the dock, which has none of the hardware it tests.")
    reset_factory_state("sensor","Connecting to the ESP32-S3 bootloader...","setup")
    FACTORY_STATE["progress"]=2
    try:
        manifest,image=verify_sensor_test_image()
        with USB_LOCK:
            run_esptool([
                "--chip","esp32s3","--port",port,"--baud",str(SERIAL_BAUD),
                "--before","default_reset","--after","hard_reset","write_flash",
                "--flash_mode",str(manifest.get("flashMode","dio")),
                "--flash_freq",str(manifest.get("flashFrequency","80m")),
                "--flash_size",str(manifest.get("flashSize","16MB")),
                "0x0",str(image)
            ])
        SETUP_SELECTION["firmwareInstalled"]=False
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,
            "status":"Sensor Test installed and verified. The board is ready for hardware testing."})
    except Exception as error:
        message=(str(error)+" Keep the USB cable connected. Tap the remote's Reset button once, "
                 "wait a moment, then press Install Sensor Test again.")
        FACTORY_STATE.update({"running":False,"done":False,"error":message,
                              "status":"Sensor Test installation failed."})

def open_https_url(req,timeout):
    # urlopen() with no explicit context relies on Python's own default CA
    # bundle, which a frozen PyInstaller build has no way to populate on a
    # machine that never ran python.org's "Install Certificates.command" (or
    # any machine at all - PyInstaller does not run it either) - the result
    # is "CERTIFICATE_VERIFY_FAILED: unable to get local issuer certificate"
    # even though the connection itself is fine.
    #
    # certifi ships its own CA bundle as package data (vendored the same way
    # esptool/pyserial already are), so this works the same on every machine
    # regardless of what the OS or an install script did or didn't set up -
    # that is what today's actual failure was and this closes it completely.
    #
    # But certifi's bundle is a snapshot frozen at whatever version was
    # installed when this build was made - it cannot update itself the way a
    # live OS or browser's trust store does, so it is not a permanent
    # guarantee on its own over a many-year lifetime. If the certifi bundle
    # ever fails verification (as opposed to a network/DNS/timeout error,
    # which retrying with different trust roots cannot fix and should not
    # mask), fall back to the machine's own current trust store - whatever
    # that machine has by then is more likely to be current than a bundle
    # frozen at build time.
    import certifi,ssl
    try:
        return urllib.request.urlopen(req,timeout=timeout,
                                      context=ssl.create_default_context(cafile=certifi.where()))
    except urllib.error.URLError as error:
        if not isinstance(error.reason,ssl.SSLCertVerificationError): raise
        return urllib.request.urlopen(req,timeout=timeout,context=ssl.create_default_context())

def download_repo(repo,cache):
    cache.mkdir(parents=True,exist_ok=True)
    zp=cache/f"{repo['owner']}_{repo['repo']}_{repo['branch']}.zip"
    if zp.exists(): zp.unlink()
    url=f"https://codeload.github.com/{repo['owner']}/{repo['repo']}/zip/refs/heads/{repo['branch']}"
    log(f"Downloading {repo['name']}...")
    req=urllib.request.Request(url,headers={"User-Agent":"OpenRemoteStudio"})
    with open_https_url(req,timeout=180) as r, open(zp,"wb") as f:
        total=r.headers.get("Content-Length"); total=int(total) if total and total.isdigit() else 0; got=0
        while True:
            chunk=r.read(128*1024)
            if not chunk: break
            f.write(chunk); got+=len(chunk); STATE["downloaded_mb"]+=len(chunk)/(1024*1024)
            STATE["current"]=f"Downloading {repo['name']}: {got/1024/1024:.1f} MB"+(f" of {total/1024/1024:.1f} MB" if total else "")
    log(f"Downloaded {repo['name']} ({zp.stat().st_size/1024/1024:.1f} MB)")
    return zp
def extract_zip(zp,dest):
    if dest.exists(): shutil.rmtree(dest)
    dest.mkdir(parents=True,exist_ok=True); log(f"Extracting {zp.name}...")
    with zipfile.ZipFile(zp) as z: z.extractall(dest)
    roots=[p for p in dest.iterdir() if p.is_dir() and p.name!="__MACOSX"]
    if not roots: raise RuntimeError("No extracted root folder found.")
    return roots[0]
def parse_ir(path):
    buttons=[]; protocols=set(); entries=[]; current={}
    try: text=path.read_text(encoding="utf-8",errors="ignore")
    except Exception: return [],[],[]
    for line in text.splitlines():
        line=line.strip(); low=line.lower()
        if not line or line.startswith("#"): continue
        if low.startswith("name:"):
            if current: entries.append(current); current={}
            b=line.split(":",1)[1].strip(); current["name"]=b
            if b: buttons.append(b)
        elif ":" in line and current is not None:
            k,v=line.split(":",1); key=k.strip().lower().replace(" ","_"); current[key]=v.strip()
            if key=="protocol" and v.strip(): protocols.add(v.strip())
    if current: entries.append(current)
    seen=set(); uniq=[]
    for b in buttons:
        k=b.lower()
        if k not in seen: uniq.append(b); seen.add(k)
    return uniq, sorted(protocols), entries
def parse_lirc(path):
    try: text=path.read_text(encoding="utf-8",errors="ignore")
    except Exception: return [],[],[]
    buttons=[]; entries=[]; in_codes=False; remote_name=""
    m=re.search(r"name\s+([^\s]+)",text)
    if m: remote_name=m.group(1)
    for line in text.splitlines():
        s=line.strip()
        if s.startswith("begin codes"): in_codes=True; continue
        if s.startswith("end codes"): in_codes=False; continue
        if in_codes and s and not s.startswith("#"):
            parts=s.split()
            if len(parts)>=2:
                buttons.append(parts[0]); entries.append({"name":parts[0],"code":parts[1],"remote":remote_name})
    return buttons, ["LIRC"], entries
def guess_names(root,fp):
    parts=list(fp.relative_to(root).parts); filt=[p for p in parts[:-1] if p.lower() not in IGNORE]
    cat=norm_cat(filt[0]) if len(filt)>=1 else "Other"; brand=nice(filt[1]).title() if len(filt)>=2 else "Unknown"
    bits=[nice(x) for x in filt[2:]] if len(filt)>=3 else []; bits.append(nice(fp.name))
    model=" ".join([b for b in bits if b.lower() not in ("ir","infrared")]).strip() or nice(fp.name)
    return cat,brand,re.sub(r"\s+"," ",model)
def create_db(records, db_path, metadata):
    if db_path.exists(): db_path.unlink()
    con=sqlite3.connect(db_path); cur=con.cursor()
    cur.execute("PRAGMA journal_mode=OFF"); cur.execute("PRAGMA synchronous=OFF")
    cur.execute("CREATE TABLE metadata(key TEXT PRIMARY KEY,value TEXT)")
    cur.execute("""CREATE TABLE remotes(id TEXT PRIMARY KEY,category TEXT,brand TEXT,model TEXT,source TEXT,format TEXT,original_path TEXT,button_count INTEGER,buttons_json TEXT,protocols_json TEXT,ir_json TEXT,search TEXT)""")
    cur.execute("CREATE INDEX idx_search ON remotes(search)"); cur.execute("CREATE INDEX idx_brand_model ON remotes(brand,model)")
    for k,v in metadata.items(): cur.execute("INSERT INTO metadata VALUES(?,?)",(k,json.dumps(v) if not isinstance(v,str) else v))
    rows=[(r["id"],r["category"],r["brand"],r["model"],r["source"],r["format"],r["original_path"],r["button_count"],json.dumps(r["buttons"],ensure_ascii=False),json.dumps(r["protocols"],ensure_ascii=False),json.dumps(r["ir"],ensure_ascii=False),r["search"]) for r in records]
    cur.executemany("INSERT INTO remotes VALUES(?,?,?,?,?,?,?,?,?,?,?,?)",rows); con.commit(); con.close()
def create_search_indexes(records, search_path, details_dir):
    if search_path.exists(): search_path.unlink()
    if details_dir.exists(): shutil.rmtree(details_dir)
    details_dir.mkdir(parents=True,exist_ok=True)
    search_file=search_path.open("w",encoding="utf-8")
    try:
        for r in records:
            commands=r["buttons"]
            protocols=r["protocols"]
            shared={
                "id":r["id"],"category":r["category"],"type":r["category"],
                "brand":r["brand"],"model":r["model"],"source":r["source"],
                "format":r["format"],"originalPath":r["original_path"],
                "buttonCount":r["button_count"],"commands":commands,
                "protocols":protocols,"protocol":", ".join(protocols),"search":r["search"]
            }
            search_file.write(json.dumps(shared,ensure_ascii=False,separators=(",",":"))+"\n")
            detail={**shared,"irJson":json.dumps(r["ir"],ensure_ascii=False)}
            prefix=str(r["id"])[:2] or "xx"
            folder=details_dir/prefix
            folder.mkdir(parents=True,exist_ok=True)
            (folder/(str(r["id"])+".json")).write_text(json.dumps(detail,ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    finally:
        search_file.close()
def create_search_index(records, index_path):
    if index_path.exists(): index_path.unlink()
    with index_path.open("w",encoding="utf-8") as f:
        for r in records:
            payload={
                "id":r["id"],"category":r["category"],"type":r["category"],
                "brand":r["brand"],"model":r["model"],"source":r["source"],
                "format":r["format"],"originalPath":r["original_path"],
                "buttonCount":r["button_count"],"commands":r["buttons"],
                "protocols":r["protocols"],"protocol":", ".join(r["protocols"]),
                "irJson":json.dumps(r["ir"],ensure_ascii=False),"search":r["search"]
            }
            f.write(json.dumps(payload,ensure_ascii=False,separators=(",",":"))+"\n")
def linux_dialog(prompt,directory=False,file_filter=None):
    # Linux has no single system file dialog the way macOS and Windows do, so
    # this uses whichever of the two near-universal desktop helpers is present:
    # zenity (GTK/GNOME) or kdialog (KDE). Both ship as separate packages, so
    # neither is guaranteed - if the user has neither, that is reported plainly
    # rather than failing with a bare FileNotFoundError, and the message names
    # the packages to install.
    zenity=shutil.which("zenity")
    if zenity:
        command=[zenity,"--file-selection","--title",prompt]
        if directory: command.append("--directory")
        elif file_filter: command.extend(["--file-filter",file_filter])
    else:
        kdialog=shutil.which("kdialog")
        if not kdialog:
            raise RuntimeError("No file chooser is available. Install 'zenity' (GNOME/GTK desktops) "
                               "or 'kdialog' (KDE), then try again.")
        command=[kdialog,"--title",prompt,
                 "--getexistingdirectory" if directory else "--getopenfilename",str(Path.home())]
        if not directory and file_filter: command.append(file_filter)
    # A cancelled dialog exits non-zero on both tools. That is an ordinary
    # outcome, not an error, and must come back as "" like the macOS
    # AppleScript branches do.
    result=subprocess.run(command,capture_output=True,text=True)
    return result.stdout.strip() if result.returncode==0 else ""

def windows_dialog(create, result_property):
    """Show a Windows common dialog and return the chosen path, or "".

    The dialog is given an explicit owner window, and that is the whole point
    of this helper. ShowDialog() with no owner parents itself to the calling
    thread's active window - but that thread belongs to a PowerShell process
    started with CREATE_NO_WINDOW, which has no window at all. Windows then has
    nothing to place the dialog above, so it opens BEHIND the browser and the
    user has to minimise the browser to find a dialog they did not know was
    waiting.

    A TopMost owner fixes it: a modal dialog inherits its owner's z-order band,
    so it is drawn above ordinary windows whether or not this process is
    allowed to take focus. That last part matters - a background process cannot
    steal foreground on Windows, so Activate() alone would not be enough and is
    only a best effort here. TopMost is what actually does the work.
    """
    script = ("Add-Type -AssemblyName System.Windows.Forms; " + create + " "
              "$owner=New-Object System.Windows.Forms.Form; "
              "$owner.TopMost=$true; $owner.ShowInTaskbar=$false; $owner.Opacity=0; "
              "$owner.Show(); $owner.Activate(); "
              "$result=$dialog.ShowDialog($owner); $owner.Close(); "
              "if($result -eq [System.Windows.Forms.DialogResult]::OK){$dialog." + result_property + "}")
    flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    return subprocess.check_output(
        ["powershell.exe", "-NoProfile", "-STA", "-Command", script],
        text=True, creationflags=flags).strip()

def choose_folder():
    if IS_LINUX:
        return linux_dialog("Choose where to save the OpenRemote.irdb release folder",directory=True)
    if os.name=="nt":
        return windows_dialog(
            "$dialog=New-Object System.Windows.Forms.FolderBrowserDialog; "
            "$dialog.Description='Choose where to save the OpenRemote.irdb release folder';",
            "SelectedPath")
    script='try\nset f to choose folder with prompt "Choose where to save the OpenRemote.irdb release folder:"\nPOSIX path of f\non error\nreturn ""\nend try'
    return subprocess.check_output(["/usr/bin/osascript","-e",script],text=True).strip()

def choose_setup_file(kind):
    if kind not in ("firmware","webconfig"): raise RuntimeError("Unknown setup file type.")
    title="Choose OpenRemote Firmware" if kind=="firmware" else "Choose OpenRemote WebConfig"
    if IS_LINUX:
        return linux_dialog(title,file_filter=("*.bin" if kind=="firmware" else "*.html *.htm"))
    if os.name=="nt":
        file_filter=("ESP32 firmware (*.bin)|*.bin" if kind=="firmware" else
                     "OpenRemote WebConfig (*.html;*.htm)|*.html;*.htm")
        return windows_dialog(
            "$dialog=New-Object System.Windows.Forms.OpenFileDialog; "
            "$dialog.Title='"+title+"'; $dialog.Filter='"+file_filter+"';",
            "FileName")
    script='try\nset f to choose file with prompt "'+title+'"\nPOSIX path of f\non error\nreturn ""\nend try'
    return subprocess.check_output(["/usr/bin/osascript","-e",script],text=True).strip()

def choose_sd_folder():
    if IS_LINUX:
        return linux_dialog("Choose the OpenRemote SD card (FAT32)",directory=True)
    if os.name=="nt":
        return windows_dialog(
            "$dialog=New-Object System.Windows.Forms.FolderBrowserDialog; "
            "$dialog.Description='Choose the root of the mounted OpenRemote SD card';",
            "SelectedPath")
    script='try\nset f to choose folder with prompt "Choose the OpenRemote SD card (MS-DOS FAT / FAT32):"\nPOSIX path of f\non error\nreturn ""\nend try'
    return subprocess.check_output(["/usr/bin/osascript","-e",script],text=True).strip()

def sd_volume_info(target):
    target=Path(target).expanduser().resolve()
    if os.name=="nt":
        import ctypes
        root=Path(target.anchor)
        if not root.anchor: raise RuntimeError("Choose the root of the SD card, such as E:\\.")
        fs=ctypes.create_unicode_buffer(64)
        volume=ctypes.create_unicode_buffer(256)
        serial=ctypes.c_ulong(); max_component=ctypes.c_ulong(); flags=ctypes.c_ulong()
        ok=ctypes.windll.kernel32.GetVolumeInformationW(
            str(root),volume,len(volume),ctypes.byref(serial),ctypes.byref(max_component),
            ctypes.byref(flags),fs,len(fs))
        if not ok: raise RuntimeError("Windows could not inspect the selected SD card.")
        drive_type=ctypes.windll.kernel32.GetDriveTypeW(str(root))
        return {"root":str(root),"filesystem":fs.value,"removable":drive_type==2,"scheme":""}
    if IS_LINUX:
        # findmnt reports the filesystem type and backing device for a mount
        # point; it is part of util-linux, so it is present on effectively
        # every Linux system without adding a dependency.
        findmnt=shutil.which("findmnt")
        if not findmnt:
            raise RuntimeError("'findmnt' is not available. Install the 'util-linux' package, then try again.")
        found=subprocess.run([findmnt,"-n","-o","SOURCE,FSTYPE,TARGET","--target",str(target)],
                             capture_output=True,text=True,timeout=8)
        if found.returncode or not found.stdout.strip():
            raise RuntimeError("Linux could not inspect the selected SD card.")
        fields=found.stdout.split()
        if len(fields)<3: raise RuntimeError("Linux could not inspect the selected SD card.")
        source,fstype,mountpoint=fields[0],fields[1],fields[2]
        # findmnt walks up to the enclosing mount, so a folder inside the card
        # resolves to the card's own mount point rather than failing. Anything
        # below that mount point is a folder on the card, not the card itself -
        # the same mistake the macOS branch rejects via its /Volumes check.
        if Path(mountpoint).resolve()!=target:
            raise RuntimeError("Choose the SD card's mount point itself, not a folder inside it.")
        removable=False
        # /sys/block/<disk>/removable is the kernel's own flag. The partition
        # device (e.g. /dev/sdb1) has to be reduced to its parent disk (sdb)
        # first, since only whole disks carry that attribute.
        try:
            name=Path(source).name
            parent=re.sub(r"p?\d+$","",name)
            flag=Path("/sys/block")/parent/"removable"
            if flag.exists(): removable=flag.read_text().strip()=="1"
        except Exception:
            pass
        if not removable:
            # USB card readers frequently report removable=0 on the disk while
            # still sitting on a USB bus. Treat a USB-attached device as
            # removable too rather than rejecting a genuine SD card.
            try:
                link=os.path.realpath(Path("/sys/class/block")/Path(source).name)
                removable="/usb" in link or "usb" in link.split("/devices/")[-1][:40]
            except Exception:
                pass
        scheme=""
        lsblk=shutil.which("lsblk")
        if lsblk:
            try:
                pt=subprocess.run([lsblk,"-n","-o","PTTYPE",source],capture_output=True,text=True,timeout=8)
                scheme=pt.stdout.strip().splitlines()[0].strip() if pt.stdout.strip() else ""
            except Exception:
                pass
        return {"root":str(target),"filesystem":fstype,"removable":removable,"scheme":scheme}
    import plistlib
    if not str(target).startswith("/Volumes/") or len(target.parts)!=3:
        raise RuntimeError("Choose the SD card itself under /Volumes, not a folder inside it.")
    result=subprocess.run(["/usr/sbin/diskutil","info","-plist",str(target)],capture_output=True,timeout=8)
    if result.returncode: raise RuntimeError("macOS could not inspect the selected SD card.")
    info=plistlib.loads(result.stdout)
    scheme=""
    parent=info.get("ParentWholeDisk")
    if parent:
        whole=subprocess.run(["/usr/sbin/diskutil","info","-plist","/dev/"+str(parent)],capture_output=True,timeout=8)
        if whole.returncode==0:
            whole_info=plistlib.loads(whole.stdout)
            scheme=str(whole_info.get("Content") or whole_info.get("PartitionMapScheme") or "")
    return {
        "root":str(target),
        "filesystem":str(info.get("FilesystemType") or info.get("FilesystemName") or info.get("Content") or ""),
        "removable":bool(info.get("RemovableMedia") or info.get("Ejectable")),
        "scheme":scheme
    }

def validate_sd_target(target,require_removable=True):
    target=Path(target).expanduser().resolve()
    unsafe={Path(target.anchor or "/").resolve(),Path.home().resolve(),APP_DIR.resolve(),DATA_DIR.resolve()}
    if target in unsafe or len(target.parts)<2:
        raise RuntimeError("That location is not safe to use as an SD card.")
    if not target.is_dir(): raise RuntimeError("The selected SD card is not mounted.")
    if not os.access(target,os.W_OK): raise RuntimeError("The selected SD card is read-only.")
    if not require_removable:
        return {"root":str(target),"filesystem":"test","removable":True,"scheme":"test"}
    info=sd_volume_info(target)
    fs=info["filesystem"].lower().replace("-","").replace(" ","")
    if not any(token in fs for token in ("fat32","msdos","fat")) or "exfat" in fs:
        raise RuntimeError("The SD card must be FAT32 / MS-DOS (FAT), not exFAT. Format it with a Master Boot Record partition map, then try again.")
    if not info["removable"]:
        raise RuntimeError("The selected location does not appear to be a removable SD card.")
    scheme=info.get("scheme","").lower()
    if scheme and any(token in scheme for token in ("gpt","guid")):
        raise RuntimeError("This SD card uses a GUID partition map. Reformat it as MS-DOS (FAT) with Master Boot Record, then try again.")
    return info

def prepare_sd_card(target,require_removable=True):
    if not FACTORY_SD_TEMPLATE.is_dir():
        raise RuntimeError("The bundled OpenRemote SD card template is missing.")
    require_setup_files(include_webconfig=True)
    firmware=inspect_firmware_file(SETUP_SELECTION["firmware"]["path"])
    if firmware["sha256"]!=SETUP_SELECTION["firmware"].get("sha256"):
        raise RuntimeError("The selected firmware changed after it was chosen. Choose it again.")
    webconfig=inspect_webconfig_file(SETUP_SELECTION["webConfig"]["path"])
    if webconfig["sha256"]!=SETUP_SELECTION["webConfig"].get("sha256"):
        raise RuntimeError("The selected WebConfig changed after it was chosen. Choose it again.")
    info=validate_sd_target(target,require_removable)
    target=Path(info["root"])
    reset_factory_state("sd","Checking the SD card...","setup")
    FACTORY_STATE["progress"]=4
    try:
        files=[path for path in FACTORY_SD_TEMPLATE.rglob("*") if path.is_file() and path.name!=".keep.txt"]
        selected_webconfig=Path(webconfig["path"]).read_bytes()
        selected_firmware=selected_application_bytes()
        total=sum(path.stat().st_size for path in files)+len(selected_webconfig)+len(selected_firmware)
        copied=0; written=0
        preserve_roots=("activities/","backups/","devices/","macros/","icons/Custom/","themes/Custom/","irdb/")
        preserve_files={"config/runtime.json"}
        existing_user_files=[]
        for path in target.rglob("*"):
            if not path.is_file(): continue
            relative=path.relative_to(target).as_posix()
            if relative in preserve_files or relative.startswith(preserve_roots): existing_user_files.append(relative)
        preserved=len(existing_user_files)
        for source in files:
            relative=source.relative_to(FACTORY_SD_TEMPLATE).as_posix()
            destination=target/Path(relative)
            destination.parent.mkdir(parents=True,exist_ok=True)
            keep_existing=destination.exists() and (relative in preserve_files or relative.startswith(preserve_roots))
            if not keep_existing:
                shutil.copy2(source,destination)
                written+=1
            copied+=source.stat().st_size
            FACTORY_STATE["progress"]=5+int((copied/total)*94)
            FACTORY_STATE["status"]="Installing "+relative
        for folder in ("activities","backups","config","devices","firmware","icons/Default","icons/Custom","irdb","logs","macros","themes/Default","themes/Custom","tmp","www"):
            (target/folder).mkdir(parents=True,exist_ok=True)

        FACTORY_STATE["status"]="Installing selected WebConfig as www/index.html"
        (target/"www"/"index.html").write_bytes(selected_webconfig)
        copied+=len(selected_webconfig); written+=1
        FACTORY_STATE["progress"]=5+int((copied/total)*94)

        firmware_name="OpenRemote_"+re.sub(r"[^0-9A-Za-z._-]","_",firmware["version"])+".bin"
        FACTORY_STATE["status"]="Installing recovery firmware "+firmware_name
        (target/"firmware"/firmware_name).write_bytes(selected_firmware)
        copied+=len(selected_firmware); written+=1
        FACTORY_STATE["progress"]=5+int((copied/total)*94)

        version_payload={
            "firmwareVersion":firmware["version"],
            "webConfigVersion":webconfig["version"],
            "preparedBy":"OpenRemote Studio "+APP_VERSION
        }
        (target/"config"/"version.json").write_text(
            json.dumps(version_payload,indent=2)+"\n",encoding="utf-8")
        written+=1
        message=("SD card ready with firmware "+firmware["version"]+
                 " and WebConfig "+webconfig["version"]+".")
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,"status":message,"error":""})
        factory_log(str(written)+" factory file(s) installed; "+str(preserved)+" existing user file(s) preserved.")
        return {"ok":True,"path":str(target),"written":written,"preserved":preserved,
                "firmwareVersion":firmware["version"],"webConfigVersion":webconfig["version"]}
    except Exception as error:
        message=str(error)
        FACTORY_STATE.update({"running":False,"done":False,"error":message,"status":"SD card setup failed."})
        raise
def build_database(save_parent, selected):
    STATE.update({"running":True,"done":False,"error":"","progress":0,"downloaded_mb":0.0,"records":0,"bin_mb":0.0,"release_path":""})
    repos=[r for r in REPOS if r["name"] in selected]
    if not repos: raise RuntimeError("No sources selected.")
    work=CACHE_DIR; cache=work/"cache"; release=work/"release_v1"
    if release.exists(): shutil.rmtree(release)
    release.mkdir(parents=True,exist_ok=True)
    records=[]; seen=set()
    for i,repo in enumerate(repos,1):
        STATE["progress"]=int((i-1)/len(repos)*62)
        root=extract_zip(download_repo(repo,cache),cache/f"extract_{repo['owner']}_{repo['repo']}_{repo['branch']}")
        log(f"Scanning {repo['name']}...")
        if repo["type"]=="flipper_ir":
            for fp in root.rglob("*.ir"):
                if any(part.lower() in IGNORE for part in fp.parts): continue
                dg=hash_file(fp,"sha1")
                if dg in seen: continue
                seen.add(dg); cat,brand,model=guess_names(root,fp); buttons,protocols,entries=parse_ir(fp)
                records.append({"id":dg[:16],"category":cat,"brand":brand,"model":model,"source":repo["name"],"format":"flipper_ir","original_path":str(fp.relative_to(root)).replace(os.sep,"/"),"button_count":len(buttons),"buttons":buttons,"protocols":protocols,"ir":entries,"search":" ".join([cat,brand,model,fp.stem," ".join(buttons)," ".join(protocols),repo["name"]]).lower()})
        elif repo["type"]=="irdb":
            codes=root/"codes"
            if codes.exists():
                for fp in codes.rglob("*"):
                    if not fp.is_file() or fp.suffix.lower() in {".md",".txt",".json"} or fp.name.startswith("."): continue
                    dg=hash_file(fp,"sha1")
                    if dg in seen: continue
                    seen.add(dg); rel=fp.relative_to(codes); parts=rel.parts; brand=nice(parts[0]).title() if parts else "Unknown"; model=" ".join(nice(p) for p in parts[1:]) or nice(fp.name)
                    try: raw=fp.read_text(encoding="utf-8",errors="ignore")
                    except Exception: raw=""
                    records.append({"id":dg[:16],"category":"IRDB Compact","brand":brand,"model":model,"source":repo["name"],"format":"probonopd_irdb_compact","original_path":str(fp.relative_to(root)).replace(os.sep,"/"),"button_count":0,"buttons":[],"protocols":[],"ir":[{"raw":raw}],"search":" ".join(["IRDB Compact",brand,model,repo["name"]]).lower()})
        elif repo["type"]=="lirc":
            for fp in root.rglob("*"):
                if not fp.is_file() or fp.suffix.lower() in {".md",".txt",".json"} or fp.name.startswith("."): continue
                try: sample=fp.read_text(encoding="utf-8",errors="ignore")[:2000]
                except Exception: continue
                if "begin remote" not in sample: continue
                dg=hash_file(fp,"sha1")
                if dg in seen: continue
                seen.add(dg); cat,brand,model=guess_names(root,fp); buttons,protocols,entries=parse_lirc(fp)
                records.append({"id":dg[:16],"category":cat,"brand":brand,"model":model,"source":repo["name"],"format":"lirc","original_path":str(fp.relative_to(root)).replace(os.sep,"/"),"button_count":len(buttons),"buttons":buttons,"protocols":protocols,"ir":entries,"search":" ".join([cat,brand,model,fp.stem," ".join(buttons),repo["name"],"lirc"]).lower()})
        STATE["records"]=len(records); log(f"Records so far: {len(records)}"); STATE["progress"]=int(i/len(repos)*62)
    log("Creating OpenRemote.irdb database...")
    STATE["progress"]=78
    records.sort(key=lambda r:(r["category"].lower(),r["brand"].lower(),r["model"].lower()))
    version=dt.datetime.now().astimezone().strftime("%Y.%m.%d.%H%M")
    created=dt.datetime.now().astimezone().isoformat(timespec="seconds")
    brands=len(set(r["brand"] for r in records)); buttons=sum(r["button_count"] for r in records)
    metadata={"database_name":"OpenRemote IRDB","database_version":version,"created_date":created,"scrape_date_local":created,"device_count":len(records),"brand_count":brands,"button_count":buttons,"builder_version":APP_VERSION,"format":"sqlite-irdb","sources":[r["name"] for r in repos]}
    db_path=release/"OpenRemote.irdb"; create_db(records,db_path,metadata)
    index_path=release/"search.jsonl"; details_dir=release/"details"; create_search_indexes(records,index_path,details_dir)
    manifest={**metadata,"irdb_sha256":hash_file(db_path,"sha256"),"irdb_size_bytes":db_path.stat().st_size,"search_index_file":"search.jsonl","search_index_sha256":hash_file(index_path,"sha256"),"search_index_size_bytes":index_path.stat().st_size,"detail_dir":"details","detail_file_count":len(records)}
    (release/"Database Manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding="utf-8")
    (release/"Release Notes.txt").write_text(f"OpenRemote.irdb\nVersion: {version}\nCreated: {created}\nDevices: {len(records)}\nBrands: {brands}\nButtons: {buttons}\n\nCopy OpenRemote.irdb to the remote SD card.\n",encoding="utf-8")
    shutil.copy2(db_path,ACTIVE_DB); STATE["db_path"]=str(ACTIVE_DB); STATE["bin_mb"]=db_path.stat().st_size/1024/1024
    dest=Path(save_parent)/f"OpenRemote.irdb v{version}"
    if dest.exists(): shutil.rmtree(dest)
    shutil.copytree(release,dest)
    STATE.update({"running":False,"done":True,"progress":100,"release_path":str(dest),"records":len(records)})
    log(f"Finished. OpenRemote.irdb release saved to {dest}")
def db_info():
    if not ACTIVE_DB.exists():
        return {"error":"No OpenRemote.irdb loaded yet."}
    try:
        con=sqlite3.connect(ACTIVE_DB)
        cur=con.cursor()
        devices=cur.execute("SELECT COUNT(*) FROM remotes").fetchone()[0]
        try:
            brands=cur.execute("SELECT COUNT(DISTINCT brand) FROM remotes").fetchone()[0]
        except Exception:
            brands=0
        try:
            btns=cur.execute("SELECT SUM(button_count) FROM remotes").fetchone()[0] or 0
        except Exception:
            btns=0

        def meta(k,d=""):
            try:
                r=cur.execute("SELECT value FROM metadata WHERE key=?",(k,)).fetchone()
                return r[0] if r else d
            except Exception:
                return d

        version=meta("database_version","")
        created=meta("created_date",meta("scrape_date_local",meta("scrape_date_utc","")))
        con.close()

        if not created:
            try:
                created=dt.datetime.fromtimestamp(ACTIVE_DB.stat().st_mtime).astimezone().isoformat(timespec="seconds")
            except Exception:
                created=""

        return {
            "path":str(ACTIVE_DB),
            "devices":devices,
            "brands":brands,
            "buttons":btns,
            "version":version,
            "build_date":created,
            "build_date_short":created[:10] if created else "",
            "size_mb":ACTIVE_DB.stat().st_size/1024/1024
        }
    except Exception as e:
        return {"error":"OpenRemote.irdb could not be read: "+str(e)}

def search_db(query):
    if not ACTIVE_DB.exists(): return {"error":"No OpenRemote.irdb loaded yet.","results":[],"total":0}
    terms=[t for t in query.lower().split() if t]; where=" AND ".join(["search LIKE ?" for _ in terms]) or "1=1"; params=[f"%{t}%" for t in terms]
    con=sqlite3.connect(ACTIVE_DB); con.row_factory=sqlite3.Row; cur=con.cursor()
    total=cur.execute(f"SELECT COUNT(*) FROM remotes WHERE {where}",params).fetchone()[0]
    rows=cur.execute(f"SELECT id,category,brand,model,source,format,button_count,buttons_json,protocols_json FROM remotes WHERE {where} ORDER BY brand,model LIMIT 100",params).fetchall(); con.close()
    results=[]
    for row in rows:
        d=dict(row); d["buttons"]=json.loads(d.pop("buttons_json") or "[]"); d["protocols"]=json.loads(d.pop("protocols_json") or "[]"); results.append(d)
    return {"results":results,"total":total,"shown":len(results)}
def device_detail(id):
    if not ACTIVE_DB.exists(): return {"error":"No OpenRemote.irdb loaded yet."}
    con=sqlite3.connect(ACTIVE_DB); con.row_factory=sqlite3.Row; row=con.execute("SELECT * FROM remotes WHERE id=?",(id,)).fetchone(); con.close()
    if not row: return {"error":"Device not found."}
    d=dict(row); d["buttons"]=json.loads(d.pop("buttons_json") or "[]"); d["protocols"]=json.loads(d.pop("protocols_json") or "[]"); d["ir"]=json.loads(d.pop("ir_json") or "[]"); return d

SUPPORTED_PROTOCOLS={"NEC","NECext","NEC1","Samsung32","RC5","RC5X","RC6","SIRC","SIRC15","SIRC20"}
def command_id(name,idx):
    base=re.sub(r"[^a-z0-9]+","_",str(name).lower()).strip("_") or "command"
    return f"cmd_{idx}_{base}"[:44]
def compact_runtime_commands(ir_rows):
    raw=(ir_rows[0] or {}).get("raw","") if ir_rows else ""
    commands=[]
    for idx,line in enumerate(raw.splitlines()[1:]):
        cells=line.split(",")
        if len(cells)<5: continue
        try:
            dev=int(cells[2]); sub=int(cells[3]); fn=int(cells[4])
        except Exception:
            continue
        proto=(cells[1] or "").strip()
        runtime_proto=proto
        address=dev
        if proto.upper().startswith("NEC"):
            if sub>=0:
                runtime_proto="NECext"; address=((dev&255)<<8)|(sub&255)
            else:
                runtime_proto="NEC"
        if runtime_proto not in SUPPORTED_PROTOCOLS: continue
        name=(cells[0] or "Command").strip()
        commands.append({"id":command_id(name,len(commands)),"name":name,"ir":{"type":"parsed","protocol":runtime_proto,"addressValue":address,"commandValue":fn}})
        if len(commands)>=24: break
    return commands
def flipper_runtime_commands(ir_rows):
    commands=[]
    for entry in ir_rows:
        name=entry.get("name")
        if not name: continue
        typ=entry.get("type","")
        if typ=="raw":
            data=str(entry.get("data",""))
            if not data: continue
            commands.append({"id":command_id(name,len(commands)),"name":str(name),"ir":{"type":"raw","frequency":int(entry.get("frequency") or 38000),"dutyCycle":float(entry.get("duty_cycle") or 0.33),"data":data}})
        elif typ=="parsed":
            proto=str(entry.get("protocol",""))
            if proto not in SUPPORTED_PROTOCOLS: continue
            ir={"type":"parsed","protocol":proto,"address":str(entry.get("address","")),"command":str(entry.get("command",""))}
            if proto.upper()=="SIRC20": ir["bits"]=20
            elif proto.upper()=="SIRC15": ir["bits"]=15
            elif proto.upper()=="SIRC": ir["bits"]=12
            commands.append({"id":command_id(name,len(commands)),"name":str(name),"ir":ir})
        if len(commands)>=24: break
    return commands
def runtime_device_from_db(id):
    d=device_detail(id)
    if d.get("error"): return d
    fmt=d.get("format","")
    if fmt=="flipper_ir": commands=flipper_runtime_commands(d.get("ir") or [])
    elif fmt=="probonopd_irdb_compact": commands=compact_runtime_commands(d.get("ir") or [])
    else: commands=[]
    if not commands:
        return {"error":"This IRDB record does not contain commands the current remote firmware can transmit yet."}
    return {
        "id":"irdb_"+d["id"],
        "name":re.sub(r"\s+"," ",(d.get("brand","")+" "+d.get("model","")).strip())[:40] or "IR Device",
        "protocol":"IR",
        "source":"IRDB",
        "type":d.get("category","IR"),
        "irdbRemoteId":d["id"],
        "irdbFormat":fmt,
        "commands":commands
    }
def safe_ir_filename(d):
    base=re.sub(r"[^A-Za-z0-9_-]+","_",(str(d.get("brand",""))+"_"+str(d.get("model",""))).strip("_"))
    base=re.sub(r"_+","_",base).strip("_") or "ir_device"
    return (base[:48]+"_"+str(d.get("id","device"))[:8]+".ir")
def ir_signal_name(name,idx):
    text=re.sub(r"\s+","_",str(name or f"Command_{idx+1}").strip())
    text=re.sub(r"[^A-Za-z0-9_+-]+","_",text).strip("_")
    return text or f"Command_{idx+1}"
def flipper_ir_text(d):
    lines=["Filetype: IR signals file","Version: 1"]
    count=0
    for idx,entry in enumerate(d.get("ir") or []):
        name=entry.get("name")
        if not name: continue
        typ=str(entry.get("type","parsed") or "parsed")
        lines.extend(["#","name: "+ir_signal_name(name,count),"type: "+typ])
        if typ=="raw":
            lines.append("frequency: "+str(entry.get("frequency") or 38000))
            lines.append("duty_cycle: "+str(entry.get("duty_cycle") or 0.33))
            data=str(entry.get("data","")).strip()
            if data: lines.append("data: "+data)
        else:
            if entry.get("protocol") is not None: lines.append("protocol: "+str(entry.get("protocol")))
            if entry.get("address") is not None: lines.append("address: "+str(entry.get("address")))
            if entry.get("command") is not None: lines.append("command: "+str(entry.get("command")))
        count+=1
    return "\n".join(lines)+"\n", count
def compact_ir_text(d):
    commands=compact_runtime_commands(d.get("ir") or [])
    lines=["Filetype: IR signals file","Version: 1"]
    for idx,command in enumerate(commands):
        ir=command["ir"]
        lines.extend([
            "#",
            "name: "+ir_signal_name(command.get("name"),idx),
            "type: parsed",
            "protocol: "+str(ir.get("protocol","")),
            "address: "+str(ir.get("addressValue",0)),
            "command: "+str(ir.get("commandValue",0))
        ])
    return "\n".join(lines)+"\n", len(commands)
def ir_file_from_db(id):
    d=device_detail(id)
    if d.get("error"): return d
    fmt=d.get("format","")
    if fmt=="flipper_ir":
        text,count=flipper_ir_text(d)
    elif fmt=="probonopd_irdb_compact":
        text,count=compact_ir_text(d)
    else:
        return {"error":"This IRDB record cannot be exported as a .ir file yet."}
    if count<=0:
        return {"error":"This IRDB record does not contain any usable IR commands."}
    return {
        "filename":safe_ir_filename(d),
        "text":text,
        "device":re.sub(r"\s+"," ",(d.get("brand","")+" "+d.get("model","")).strip()) or "IR Device",
        "commands":count
    }
def usb_ports():
    if os.name=="nt":
        if serial is None: return []
        return [item.device for item in serial.tools.list_ports.comports()]
    if IS_LINUX:
        # The ESP32-S3's native USB CDC enumerates as /dev/ttyACM*; a board
        # behind a CP210x/CH34x/FTDI bridge appears as /dev/ttyUSB*. There is
        # no /dev/cu.* callout-device split on Linux, so both are listed
        # directly with no normalisation step needed afterwards.
        ports=[]
        for pat in ("/dev/ttyACM*","/dev/ttyUSB*"):
            ports.extend(glob.glob(pat))
        return sorted(dict.fromkeys(ports))
    cu_ports=[]
    for pat in ("/dev/cu.usbmodem*","/dev/cu.usbserial*","/dev/cu.SLAB_USBtoUART*"):
        cu_ports.extend(glob.glob(pat))
    if cu_ports:
        return sorted(dict.fromkeys(cu_ports))
    tty_ports=[]
    for pat in ("/dev/tty.usbmodem*","/dev/tty.usbserial*"):
        tty_ports.extend(glob.glob(pat))
    return sorted(dict.fromkeys(tty_ports))
def normalize_usb_port(port):
    # macOS only: /dev/tty.* is the dial-in device and blocks on open until
    # carrier detect, /dev/cu.* is the callout device that does not. Linux
    # has no such pair, so its port names pass through untouched.
    if IS_MAC and port.startswith("/dev/tty."):
        cu="/dev/cu."+port[len("/dev/tty."):]
        if Path(cu).exists(): return cu
    return port
def busy_port_hint(port):
    hint="USB serial port is busy. Close Arduino Serial Monitor, Arduino upload, or any other app using the port, then try again."
    if os.name!="nt":
        # lsof lives in /usr/sbin on macOS and /usr/bin on most Linux distros,
        # and is not installed by default on all of them - look it up rather
        # than hardcoding either path. Its absence just means no owner names in
        # the hint, which is why the whole block is best-effort already.
        lsof=shutil.which("lsof") or "/usr/sbin/lsof"
        try:
            r=subprocess.run([lsof,normalize_usb_port(port)],capture_output=True,text=True,timeout=2)
            lines=[ln for ln in r.stdout.splitlines()[1:] if ln.strip()]
            if lines:
                owners=[]
                for ln in lines[:3]:
                    parts=ln.split()
                    if len(parts)>=2: owners.append(parts[0]+" PID "+parts[1])
                if owners: hint+=" Currently using it: "+", ".join(owners)+"."
        except Exception:
            pass
    return hint
def open_serial_port(port):
    port=normalize_usb_port(port)
    if os.name=="nt":
        if serial is None:
            raise RuntimeError("The bundled Windows serial driver is missing.")
        connection=serial.Serial()
        connection.port=port
        connection.baudrate=SERIAL_BAUD
        connection.bytesize=serial.EIGHTBITS
        connection.parity=serial.PARITY_NONE
        connection.stopbits=serial.STOPBITS_ONE
        connection.timeout=0
        connection.write_timeout=8
        connection.rtscts=False
        connection.dsrdtr=False
        connection.dtr=False
        connection.rts=False
        connection.open()
        connection.reset_input_buffer()
        return connection, None
    import termios
    try:
        fd=os.open(port,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
    except OSError as e:
        if getattr(e,"errno",None)==16:
            raise RuntimeError(busy_port_hint(port))
        # EACCES on Linux almost always means the user is not in the group
        # that owns /dev/tty*, not that anything is actually wrong. Distros
        # differ on the name ("dialout" on Debian/Ubuntu, "uucp" on Arch and
        # Fedora), so the real owning group is read off the device itself
        # rather than guessed at.
        if IS_LINUX and getattr(e,"errno",None)==13:
            group="dialout"
            try:
                import grp
                group=grp.getgrgid(os.stat(port).st_gid).gr_name
            except Exception:
                pass
            raise RuntimeError(f"No permission to open {port}. Add your user to the '{group}' group with: "
                               f"sudo usermod -aG {group} $USER — then log out and back in.")
        raise
    attrs=termios.tcgetattr(fd)
    attrs[0]=0; attrs[1]=0; attrs[2]=termios.CS8|termios.CLOCAL|termios.CREAD; attrs[3]=0
    speed=getattr(termios,"B460800",termios.B115200)
    attrs[4]=speed; attrs[5]=speed
    attrs[6][termios.VMIN]=0; attrs[6][termios.VTIME]=0
    termios.tcsetattr(fd,termios.TCSANOW,attrs)
    # macOS needs IOSSIOSPEED to reach non-standard rates; Linux has a real
    # B460800 constant (set via termios above), so no ioctl is needed here.
    if IS_MAC:
        fcntl.ioctl(fd,0x80045402,struct.pack("I",SERIAL_BAUD))
    try:
        termios.tcflush(fd,termios.TCIFLUSH)
    except Exception:
        pass
    return os.fdopen(fd,"r+b",buffering=0), fd
def serial_write(port_obj,data):
    view=memoryview(data); sent=0
    deadline=time.time()+max(8,len(data)/4000)
    while sent<len(view):
        try:
            written=port_obj.write(view[sent:])
        except (BlockingIOError,InterruptedError):
            written=0
        if written:
            sent+=written
            continue
        if time.time()>=deadline:
            raise TimeoutError(f"Only sent {sent} of {len(view)} byte(s) to the remote.")
        if os.name!="nt":
            select.select([],[port_obj.fileno()],[],0.05)
        else:
            time.sleep(0.01)
    port_obj.flush()
def serial_readline(port_obj,fd,timeout=8):
    deadline=time.time()+timeout; buf=b""
    while time.time()<deadline:
        if fd is not None:
            r,_,_=select.select([fd],[],[],0.05)
            chunk=os.read(fd,1) if r else b""
        else:
            chunk=port_obj.read(1)
            if not chunk: time.sleep(0.05)
        if chunk:
            buf+=chunk
            while b"\n" in buf:
                line,buf=buf.split(b"\n",1)
                text=line.decode("utf-8","ignore").strip()
                if text.startswith("ORUSB "): return text[6:]
    raise TimeoutError("No OpenRemote USB response. Check the port and that the remote is awake.")
def usb_handshake(port_obj,fd,timeout=30):
    deadline=time.time()+timeout
    last_error=None
    while time.time()<deadline:
        try:
            serial_write(port_obj,b"\nORUSB PING\n")
            response=json.loads(serial_readline(port_obj,fd,timeout=1.2))
            if response.get("ok"):
                return response
            last_error=response.get("error","Remote rejected ping.")
        except Exception as e:
            last_error=str(e)
        time.sleep(0.35)
    raise TimeoutError((last_error or "No OpenRemote USB response")+". Make sure firmware 1.24 or newer is installed, the remote is awake, and Arduino Serial Monitor is closed.")
def firmware_at_least(response,minimum):
    try:
        return float(str(response.get("firmwareVersion","0")))>=float(minimum)
    except Exception:
        return False
def require_usb_firmware(response,minimum="1.24"):
    if not firmware_at_least(response,minimum):
        raise RuntimeError("Remote USB answered, but firmware "+str(response.get("firmwareVersion","unknown"))+" does not support this USB feature. Flash OpenRemote firmware "+minimum+" or newer.")
def serial_read_exact(port_obj,fd,length,timeout=90):
    deadline=time.time()+timeout; data=bytearray()
    while len(data)<length and time.time()<deadline:
        if fd is not None:
            r,_,_=select.select([fd],[],[],0.1)
            chunk=os.read(fd,min(4096,length-len(data))) if r else b""
        else:
            chunk=port_obj.read(min(4096,length-len(data)))
            if not chunk: time.sleep(0.05)
        if chunk: data.extend(chunk)
    if len(data)!=length:
        raise TimeoutError(f"Only read {len(data)} of {length} byte(s) from the remote.")
    return bytes(data)

def serial_upload_payload(port_obj,fd,header,payload,progress=None):
    serial_write(port_obj,header)
    ready=serial_readline(port_obj,fd,timeout=12)
    if ready.startswith("{"):
        response=json.loads(ready)
        raise RuntimeError(response.get("error","Remote rejected the upload."))
    match=re.fullmatch(r"READY (\d+)",ready)
    if not match: raise RuntimeError("Unexpected remote upload response: "+ready[:80])
    window=int(match.group(1))
    if window<32 or window>4096: raise RuntimeError("Remote supplied an invalid upload window.")
    sent=0
    while sent<len(payload):
        chunk=payload[sent:sent+window]
        serial_write(port_obj,chunk)
        sent+=len(chunk)
        response=serial_readline(port_obj,fd,timeout=20)
        if progress: progress(sent,len(payload))
        if sent==len(payload):
            result=json.loads(response)
            if not result.get("ok"): raise RuntimeError(result.get("error","Remote rejected the upload."))
            return result
        match=re.fullmatch(r"ACK (\d+)",response)
        if not match or int(match.group(1))!=sent:
            raise RuntimeError("Remote upload acknowledgement was missing or out of sequence.")
    raise RuntimeError("USB upload contained no data.")

def _usb_import_device(port,id):
    ir_file=ir_file_from_db(id)
    if ir_file.get("error"): return ir_file
    payload=ir_file["text"].encode("utf-8")
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        header=("ORUSB IRFILE %s %d\n"%(ir_file["filename"],len(payload))).encode("ascii")
        response=serial_upload_payload(port_obj,fd,header,payload)
        response["device"]=ir_file["device"]; response["commands"]=ir_file["commands"]; response["filename"]=ir_file["filename"]
        return response
    except Exception as e:
        return {"error":str(e)}
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass
def serial_read_chunked_file(port_obj,fd,size):
    data=bytearray()
    USB_TRANSFER_STATE.update({"active":True,"received":0,"total":size,"error":""})
    while True:
        if len(data)<size: serial_write(port_obj,b"ORUSB NEXT\n")
        frame=serial_readline(port_obj,fd,timeout=15)
        if frame=="DONE": break
        if frame.startswith("{"):
            response=json.loads(frame)
            raise RuntimeError(response.get("error","Remote stopped the file transfer."))
        match=re.fullmatch(r"DATA (\d+) (\d+)",frame)
        if not match:
            raise RuntimeError("Unexpected remote USB transfer frame: "+frame[:80])
        offset=int(match.group(1)); length=int(match.group(2))
        if offset!=len(data) or length<1 or length>4096 or len(data)+length>size:
            raise RuntimeError("Remote USB transfer frame was out of sequence.")
        data.extend(serial_read_exact(port_obj,fd,length,timeout=15))
        separator=serial_read_exact(port_obj,fd,1,timeout=3)
        if separator==b"\r": separator+=serial_read_exact(port_obj,fd,1,timeout=3)
        if not separator.endswith(b"\n"):
            raise RuntimeError("Remote USB transfer frame was not terminated correctly.")
        USB_TRANSFER_STATE["received"]=len(data)
    if len(data)!=size:
        raise RuntimeError(f"Only read {len(data)} of {size} byte(s) from the remote.")
    return bytes(data)

def _usb_read_file(port,path):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        serial_write(port_obj,("ORUSB READ %s\n"%path).encode("ascii"))
        header=serial_readline(port_obj,fd,timeout=12)
        if header.startswith("{"):
            response=json.loads(header)
            raise RuntimeError(response.get("error","Remote rejected the file read."))
        match=re.fullmatch(r"FILE (\d+) CHUNKED",header)
        if not match:
            raise RuntimeError("Unexpected remote USB response: "+header[:80])
        return serial_read_chunked_file(port_obj,fd,int(match.group(1)))
    except Exception:
        try:
            if port_obj: serial_write(port_obj,b"ORUSB CANCEL\n")
        except Exception:
            pass
        raise
    finally:
        USB_TRANSFER_STATE["active"]=False
        try:
            if port_obj: port_obj.close()
        except Exception: pass
def _usb_ping_remote(port):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        response=usb_handshake(port_obj,fd,timeout=30)
        response["port"]=port
        response["supportsUsbSdDevices"]=firmware_at_least(response,"1.24")
        return response
    except Exception as e:
        return {"error":str(e)}
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_import_device(port,id):
    with USB_LOCK: return _usb_import_device(port,id)

def usb_read_file(port,path):
    with USB_LOCK:
        try:
            return _usb_read_file(port,path)
        except Exception as e:
            USB_TRANSFER_STATE["error"]=str(e)
            raise

def usb_ping_remote(port):
    with USB_LOCK: return _usb_ping_remote(port)

def _usb_json_command(port,command):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        serial_write(port_obj,("ORUSB "+command+"\n").encode("ascii"))
        response=json.loads(serial_readline(port_obj,fd,timeout=12))
        if not response.get("ok"): raise RuntimeError(response.get("error","Remote rejected the USB command."))
        return response
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_json_command(port,command):
    with USB_LOCK: return _usb_json_command(port,command)

def _usb_write_file(port,path,payload):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        header=("ORUSB WRITE %s %d\n"%(path,len(payload))).encode("ascii")
        return serial_upload_payload(port_obj,fd,header,payload)
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_write_file(port,path,payload):
    with USB_LOCK: return _usb_write_file(port,path,payload)

def _usb_write_file_progress(port,path,payload,progress):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        header=("ORUSB WRITE %s %d\n"%(path,len(payload))).encode("ascii")
        return serial_upload_payload(port_obj,fd,header,payload,progress=progress)
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def _usb_factory_reset(port):
    port_obj=None
    try:
        port=normalize_usb_port(port)
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=30)
        require_usb_firmware(hello,"1.24")
        serial_write(port_obj,b"ORUSB FACTORYRESET\n")
        reply=serial_readline(port_obj,fd,timeout=60)
        if not reply.startswith("{"):
            raise RuntimeError("Unexpected remote response: "+reply[:80])
        result=json.loads(reply)
        if not result.get("ok"):
            raise RuntimeError(result.get("error","The remote rejected the factory reset."))
        return result
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_factory_reset(port):
    try:
        with USB_LOCK: return _usb_factory_reset(port)
    except Exception as e:
        return {"error":str(e)}

BACKUP_CATEGORIES={"full-backup","devices","learned","activities","macros",
                   "icons","themes","learned-device"}

def describe_backup_payload(payload):
    """Validates a backup file and summarises it, or raises.

    Accepts a full backup and any single-category export (themes, icons,
    devices, ...), plus a single learned-device .ir JSON. Anything else is
    refused before a byte is sent, so a stray file cannot land in /backups.
    """
    try:
        doc=json.loads(payload.decode("utf-8"))
    except Exception:
        raise RuntimeError("That file is not valid JSON, so it is not an OpenRemote backup.")
    if not isinstance(doc,dict):
        raise RuntimeError("That file is not an OpenRemote backup.")
    category=str(doc.get("category") or "")
    fmt=str(doc.get("format") or "")
    if category not in BACKUP_CATEGORIES and not fmt.startswith("OpenRemote"):
        raise RuntimeError("That JSON is not an OpenRemote backup (no recognised category).")
    if category=="full-backup":
        counts=doc.get("counts") or {}
        bits=[f"{counts.get(k,0)} {k}" for k in ("devices","activities","macros","icons","themes")]
        return "Full backup - "+", ".join(bits)
    if category=="learned-device":
        device=doc.get("device") or {}
        commands=device.get("commands")
        return "learned IR device - %s, %d command(s)"%(
            device.get("name") or "unnamed",
            len(commands) if isinstance(commands,list) else 0)
    if category:
        items=doc.get("items")
        n=len(items) if isinstance(items,list) else (1 if doc.get("device") else 0)
        summary=f"{category} backup - {n} item(s)"
        # A devices export records file-backed Studio/IRDB devices by path
        # only - unlike a full backup it carries no copy of the .ir bytes. The
        # device record restores either way, but it will have nothing to send
        # unless that file is still in /devices, so say so before it looks
        # like a silent failure.
        if isinstance(items,list):
            external=[i.get("name") or "unnamed" for i in items
                      if isinstance(i,dict) and i.get("fileBacked") and i.get("filePath")]
            if external:
                summary+=" (%d need their .ir file already in /devices: %s)"%(
                    len(external),", ".join(external[:4])+(", ..." if len(external)>4 else ""))
        return summary
    return fmt or "OpenRemote backup"

def send_recovery_file(port,remote_path,payload,label,ir_filename=None):
    """One transfer worker for every single-file Recovery action.

    Reuses WEBCONFIG_STATE so all of them report through the Recovery tab's
    existing progress bar and KB/MB counter rather than each inventing their
    own.
    """
    WEBCONFIG_STATE.update({"running":True,"done":False,"error":"","sent":0,
                            "total":len(payload),"status":"Connecting to the remote..."})
    def report(sent,total):
        WEBCONFIG_STATE["sent"]=sent
        WEBCONFIG_STATE["total"]=total
        WEBCONFIG_STATE["status"]=label
    port_obj=None
    uptime_before=None
    try:
        with USB_LOCK:
            port_n=normalize_usb_port(port)
            port_obj,fd=open_serial_port(port_n)
            hello=usb_handshake(port_obj,fd,timeout=30)
            uptime_before=hello.get("uptimeSec")
            require_usb_firmware(hello,"1.24")
            if ir_filename:
                header=("ORUSB IRFILE %s %d\n"%(ir_filename,len(payload))).encode("ascii")
            else:
                header=("ORUSB WRITE %s %d\n"%(remote_path,len(payload))).encode("ascii")
            result=serial_upload_payload(port_obj,fd,header,payload,progress=report)
        WEBCONFIG_STATE.update({"running":False,"done":True,"sent":len(payload),
                                "total":len(payload),
                                "status":"Installed "+(ir_filename or remote_path)+"."})
        return result
    except Exception as e:
        detail=usb_probe_after_failure(port,uptime_before)
        message=str(e)+"  [diagnostic: "+detail+"]"
        WEBCONFIG_STATE.update({"running":False,"done":True,"error":message,
                                "status":"Failed: "+message})
        return {"error":message}
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_restore_backup(port,name):
    """Asks the remote to apply a backup already sitting in /backups."""
    port_obj=None
    try:
        with USB_LOCK:
            port_obj,fd=open_serial_port(normalize_usb_port(port))
            usb_handshake(port_obj,fd,timeout=30)
            serial_write(port_obj,("ORUSB RESTORE %s\n"%name).encode("ascii"))
            reply=serial_readline(port_obj,fd,timeout=90)
            if not reply.startswith("{"):
                raise RuntimeError("Unexpected remote response: "+reply[:80])
            result=json.loads(reply)
            if not result.get("ok"):
                raise RuntimeError(result.get("error","The remote rejected the restore."))
            return result
    except Exception as e:
        return {"error":str(e)}
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_probe_after_failure(port,uptime_before=None):
    """Re-pings the remote after a failed transfer and summarises its state.

    Studio holds the only serial port during a transfer, so a crash on the
    remote is otherwise invisible. Reading resetReason/uptime immediately
    afterwards distinguishes "the remote rebooted mid-copy" from "the remote
    stayed up and the link or SD write failed".

    Pass uptime_before (the uptimeSec from the handshake that opened the
    transfer) whenever it is known. Guessing from uptime alone declared a
    reboot for any remote that had been running under a minute, so a transfer
    refused instantly on a freshly booted remote reported "remote REBOOTED
    mid-transfer" - the exact false lead this diagnostic exists to prevent.
    An uptime that only went up is proof it never restarted.
    """
    port_obj=None
    try:
        time.sleep(1.5)
        with USB_LOCK:
            port_obj,fd=open_serial_port(normalize_usb_port(port))
            hello=usb_handshake(port_obj,fd,timeout=20)
        reason=str(hello.get("resetReason","?"))
        uptime=int(hello.get("uptimeSec",0) or 0)
        if uptime_before is not None:
            rebooted=uptime<int(uptime_before)
        else:
            rebooted=uptime<60 and reason not in ("software (esp_restart)","external reset pin")
        overflows=hello.get("uartRxOverflows")
        detail=("remote REBOOTED mid-transfer (reset: %s, up %ds)"%(reason,uptime) if rebooted
                else "remote stayed up (reset: %s, up %ds, heap %s, psram %s)"%(
                    reason,uptime,hello.get("heapFree","?"),hello.get("psramFree","?")))
        # Firmware 2.85+ counts bytes the UART driver dropped. A non-zero
        # count means data was lost on the wire, not a crash or an SD fault.
        if overflows is not None:
            detail+=", %s UART receive overflow(s)"%overflows
        return detail
    except Exception as e:
        return "remote did not answer a follow-up ping (%s)"%str(e)[:60]
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def usb_file_exists(port,remote_path):
    """STAT probe used for the .ir duplicate warning (firmware 2.71+)."""
    port_obj=None
    try:
        with USB_LOCK:
            port_obj,fd=open_serial_port(normalize_usb_port(port))
            usb_handshake(port_obj,fd,timeout=20)
            serial_write(port_obj,("ORUSB STAT %s\n"%remote_path).encode("ascii"))
            info=json.loads(serial_readline(port_obj,fd,timeout=15))
            return bool(info.get("ok") and info.get("exists"))
    except Exception:
        return False
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def install_webconfig_over_usb(port,payload):
    """Writes an HTML file to the remote's SD card as /www/index.html.

    This is the recovery path when a WebConfig install has left the web UI
    unreachable - the remote's own HTTP server cannot be used to replace a
    broken WebConfig, but the USB serial link is always available. The
    firmware already accepts this exact path (usbWritableSdPath allows
    WEB_CONFIG_PATH), so no firmware change is needed.
    """
    WEBCONFIG_STATE.update({"running":True,"done":False,"error":"","sent":0,
                            "total":len(payload),"status":"Connecting to the remote..."})
    def report(sent,total):
        WEBCONFIG_STATE["sent"]=sent
        WEBCONFIG_STATE["total"]=total
        WEBCONFIG_STATE["status"]="Sending WebConfig to the SD card..."
    try:
        with USB_LOCK:
            result=_usb_write_file_progress(port,"/www/index.html",payload,report)
        WEBCONFIG_STATE.update({"running":False,"done":True,"sent":len(payload),
                                "total":len(payload),
                                "status":"Installed as /www/index.html. Reload WebConfig in your browser."})
        return result
    except Exception as e:
        WEBCONFIG_STATE.update({"running":False,"done":True,"error":str(e),
                                "status":"Install failed: "+str(e)})
        return {"error":str(e)}

def install_firmware_and_sd(port):
    """Step 2 of Setup New Remote: firmware, then the SD card, as one action.

    The SD card cannot be prepared first - preparing it talks to the running
    OpenRemote firmware over USB, which does not exist yet on a blank board.
    So the firmware is installed, given time to boot, and the card is then
    prepared through it.
    """
    try:
        flash_factory_board(port)
    except Exception as error:
        FACTORY_STATE.update({"running":False,"done":False,"error":str(error),
                              "status":"Firmware installation failed."})
        return
    if FACTORY_STATE.get("error") or not SETUP_SELECTION.get("firmwareInstalled"):
        return
    FACTORY_STATE.update({"running":True,"operation":"remote-sd","progress":2,
                          "done":False,"error":"","view":"setup",
                          "status":"Firmware installed. Waiting for the remote to restart..."})
    factory_log("Firmware installed. Waiting for the remote to restart...")
    time.sleep(9)
    prepare_remote_sd(port,recovery=False)

def prepare_remote_sd(port,recovery=False):
    """Prepares the SD card inside the remote over USB.

    recovery=True is the Recovery tab's path: it needs no firmware or
    WebConfig selection at all, because the remote is already flashed and the
    user is there precisely because setup could not be completed normally. It
    falls back to the WebConfig bundled with Studio.

    Firmware .bin files are never copied to the card - the remote runs from
    internal flash, and a spare copy on the card only wasted minutes of a
    460800 baud transfer.
    """
    port=normalize_usb_port((port or "").strip())
    port_obj=None
    uptime_before=None
    try:
        firmware=None; webconfig=None
        files=[]
        if recovery:
            bundled=FACTORY_SD_TEMPLATE/"www"/"index.html"
            if SETUP_SELECTION.get("webConfig"):
                webconfig=inspect_webconfig_file(SETUP_SELECTION["webConfig"]["path"])
                files.append(("/www/index.html",Path(webconfig["path"]).read_bytes()))
            elif bundled.is_file():
                files.append(("/www/index.html",bundled.read_bytes()))
        else:
            require_setup_files(include_webconfig=True)
            firmware=inspect_firmware_file(SETUP_SELECTION["firmware"]["path"])
            if firmware["sha256"]!=SETUP_SELECTION["firmware"].get("sha256"):
                raise RuntimeError("The selected firmware changed after it was chosen. Choose it again.")
            webconfig=inspect_webconfig_file(SETUP_SELECTION["webConfig"]["path"])
            if webconfig["sha256"]!=SETUP_SELECTION["webConfig"].get("sha256"):
                raise RuntimeError("The selected WebConfig changed after it was chosen. Choose it again.")
            files.append(("/www/index.html",Path(webconfig["path"]).read_bytes()))
        version_payload=json.dumps({
            "firmwareVersion":(firmware or {}).get("version","unknown"),
            "webConfigVersion":(webconfig or {}).get("version","bundled"),
            "preparedBy":"OpenRemote Studio "+APP_VERSION
        },indent=2).encode("utf-8")+b"\n"
        files.append(("/config/version.json",version_payload))
        icon_root=FACTORY_SD_TEMPLATE/"icons"/"Default"
        for source in sorted(icon_root.rglob("*")):
            if source.is_file() and not source.name.startswith(".") and source.name!=".keep.txt":
                files.append(("/icons/Default/"+source.relative_to(icon_root).as_posix(),source.read_bytes()))
        # Default themes: the .rgb565 wallpapers the LCD renders, their PNG
        # previews and themes.json. The matching _source.png originals are
        # deliberately not bundled - they are ~5.6 MB that only WebConfig's
        # re-crop editor ever reads, and this link is 460800 baud.
        theme_root=FACTORY_SD_TEMPLATE/"themes"/"Default"
        for source in sorted(theme_root.rglob("*")):
            if source.is_file() and not source.name.startswith(".") and source.name!=".keep.txt":
                files.append(("/themes/Default/"+source.relative_to(theme_root).as_posix(),source.read_bytes()))

        FACTORY_STATE.update({"progress":2,"status":"Waiting for the installed OpenRemote firmware..."})
        port_obj,fd=open_serial_port(port)
        hello=usb_handshake(port_obj,fd,timeout=45)
        uptime_before=hello.get("uptimeSec")
        require_usb_firmware(hello,"2.03")
        serial_write(port_obj,b"ORUSB PREPARESD\n")
        prepared=json.loads(serial_readline(port_obj,fd,timeout=30))
        if not prepared.get("ok"):
            raise RuntimeError(prepared.get("error","The remote could not prepare its SD card."))
        if not prepared.get("runtimeExists"):
            runtime_path=FACTORY_SD_TEMPLATE/"config"/"runtime.json"
            files.insert(2,("/config/runtime.json",runtime_path.read_bytes()))

        # Skip anything the card already holds at the same size. Without this
        # every setup re-sends several megabytes over USB even when the card
        # is already populated. Firmware older than 2.71 has no STAT command,
        # in which case everything is sent as before.
        def remote_file_size(remote_path):
            try:
                serial_write(port_obj,("ORUSB STAT %s\n"%remote_path).encode("ascii"))
                reply=serial_readline(port_obj,fd,timeout=15)
                if not reply.startswith("{"): return None
                info=json.loads(reply)
                if not info.get("ok"): return None
                return int(info.get("size",0)) if info.get("exists") else None
            except Exception:
                return None

        # WebConfig is always rewritten, never size-skipped. It is the file
        # most likely to be the reason someone is running setup or recovery in
        # the first place, and a same-size-but-wrong copy (a truncated or
        # older build) would otherwise be silently kept.
        ALWAYS_OVERWRITE={"/www/index.html"}
        pending=[]
        skipped=0
        for remote_path,payload in files:
            existing=None if remote_path in ALWAYS_OVERWRITE else remote_file_size(remote_path)
            if existing is not None and existing==len(payload):
                skipped+=1
                factory_log("Already present, skipped "+remote_path)
                continue
            pending.append((remote_path,payload))
        files=pending
        if skipped:
            factory_log("Skipped %d file(s) already on the card."%skipped)
        if not files:
            FACTORY_STATE.update({"running":False,"done":True,"progress":100,
                                  "status":"SD card already has every required file. Nothing to install.",
                                  "error":"","filesInstalled":0,"bytesInstalled":0,
                                  "filesSkipped":skipped,
                                  "foldersCreated":prepared.get("foldersCreated",0),
                                  "foldersTotal":prepared.get("foldersTotal",0),
                                  "cardMb":prepared.get("cardMb",0)})
            return

        total=sum(len(payload) for _,payload in files) or 1
        completed=0
        for index,(remote_path,payload) in enumerate(files,1):
            FACTORY_STATE["status"]="Installing "+remote_path
            header=("ORUSB WRITE %s %d\n"%(remote_path,len(payload))).encode("ascii")
            base=completed
            def update_progress(sent,_length,base=base):
                FACTORY_STATE["progress"]=4+int(((base+sent)/total)*95)
                FACTORY_STATE["bytesSent"]=base+sent
                FACTORY_STATE["bytesTotal"]=total
            result=serial_upload_payload(port_obj,fd,header,payload,update_progress)
            if not result.get("ok"):
                raise RuntimeError(result.get("error","The remote rejected "+remote_path))
            completed+=len(payload)
            factory_log("Installed "+remote_path+" ("+str(len(payload))+" bytes)")
        message=("SD card ready"+
                 (" with WebConfig "+webconfig["version"] if webconfig else " using the bundled WebConfig")+
                 (" (%d file(s) already present were skipped)."%skipped if skipped else "."))
        FACTORY_STATE.update({"running":False,"done":True,"progress":100,
                              "status":message,"error":"","filesInstalled":len(files),
                              "bytesInstalled":completed,"filesSkipped":skipped,
                              "foldersCreated":prepared.get("foldersCreated",0),
                              "foldersTotal":prepared.get("foldersTotal",0),
                              "cardMb":prepared.get("cardMb",0)})
    except Exception as error:
        message=str(error)+"  [diagnostic: "+usb_probe_after_failure(port,uptime_before)+"]"
        FACTORY_STATE.update({"running":False,"done":False,"error":message,
                              "status":"Remote SD card setup failed."})
    finally:
        try:
            if port_obj: port_obj.close()
        except Exception: pass

def active_remote_usb_port():
    port=(USB_WEBCONFIG_CACHE.get("port") or "").strip()
    if not port: raise RuntimeError("Connect to the OpenRemote over USB first.")
    return port

def acquire_instance_lock():
    global INSTANCE_LOCK
    handle=open(INSTANCE_LOCK_PATH,"a+b")
    try:
        if os.name=="nt":
            handle.seek(0,os.SEEK_END)
            if handle.tell()==0:
                handle.write(b"0"); handle.flush()
            handle.seek(0)
            msvcrt.locking(handle.fileno(),msvcrt.LK_NBLCK,1)
        else:
            fcntl.flock(handle.fileno(),fcntl.LOCK_EX|fcntl.LOCK_NB)
        INSTANCE_LOCK=handle
        return True
    except (OSError,IOError):
        handle.close()
        return False

def release_instance_lock():
    global INSTANCE_LOCK
    if INSTANCE_LOCK is None: return
    try:
        if os.name=="nt":
            INSTANCE_LOCK.seek(0)
            msvcrt.locking(INSTANCE_LOCK.fileno(),msvcrt.LK_UNLCK,1)
        else:
            fcntl.flock(INSTANCE_LOCK.fileno(),fcntl.LOCK_UN)
    except Exception:
        pass
    try: INSTANCE_LOCK.close()
    except Exception: pass
    INSTANCE_LOCK=None

def existing_instance_url(timeout=5.0):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        try:
            state=json.loads(INSTANCE_STATE.read_text(encoding="utf-8"))
            port=int(state.get("port",0))
            if port:
                url=f"http://127.0.0.1:{port}/"
                with urllib.request.urlopen(url+"app/ping",timeout=0.6) as response:
                    status=json.loads(response.read().decode("utf-8"))
                if status.get("ok"): return url
        except Exception:
            pass
        time.sleep(0.15)
    return None

def mark_client_seen():
    global CLIENT_LAST_SEEN,CLIENT_SEEN
    CLIENT_LAST_SEEN=time.monotonic()
    CLIENT_SEEN=True

def server_is_busy():
    # Deliberately does NOT test USB_LOCK.locked(): a worker that died while
    # holding it would pin this True forever and stop the app ever exiting.
    # Liveness is judged from the jobs themselves instead.
    return bool(STATE.get("running") or factory_job_running()
                or WEBCONFIG_STATE.get("running") or USB_TRANSFER_STATE.get("active"))

def idle_server_watchdog(server):
    while APP_SERVER is server:
        time.sleep(1.0)
        if CLIENT_SEEN and time.monotonic()-CLIENT_LAST_SEEN>15.0 and not server_is_busy():
            server.shutdown()
            return

HTML=(APP_DIR/"studio.html").read_text(encoding="utf-8")
class Handler(BaseHTTPRequestHandler):
    def send_data(self,data,ctype="text/html",code=200):
        if isinstance(data,str): data=data.encode("utf-8")
        self.send_response(code); self.send_header("Content-Type",ctype); self.send_header("Content-Length",str(len(data))); self.end_headers(); self.wfile.write(data)
    def log_message(self,*args): pass
    def read_json(self):
        length=int(self.headers.get("Content-Length","0") or 0)
        if not length: return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))
    def do_GET(self):
        mark_client_seen()
        # Serving the app shell is a fresh session: drop any firmware/WebConfig
        # picked in a previous one so they are not silently reused. Skipped
        # while a job is running so a mid-run refresh cannot clear them.
        if self.path in ("/","/index.html") and not server_is_busy():
            SETUP_SELECTION.update({"firmware":None,"webConfig":None,
                                    "boardDetected":False,"detectedPort":"",
                                    "firmwareInstalled":False})
        if self.path.startswith("/app/ping"): self.send_data(json.dumps({"ok":True,"version":APP_VERSION}),"application/json")
        elif self.path.startswith("/usb/webconfig-state"): self.send_data(json.dumps(WEBCONFIG_STATE),"application/json")
        elif self.path.startswith("/status"): self.send_data(json.dumps(STATE),"application/json")
        elif self.path.startswith("/setup/status"): self.send_data(json.dumps(FACTORY_STATE),"application/json")
        elif self.path.startswith("/setup/info"):
            try: self.send_data(json.dumps(setup_info()),"application/json")
            except Exception as e: self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",500)
        elif self.path.startswith("/api/status"):
            try: self.send_data(json.dumps(usb_json_command(active_remote_usb_port(),"STATUS"),ensure_ascii=False),"application/json")
            except Exception as e: self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",503)
        elif self.path.startswith("/api/config"):
            try: self.send_data(usb_read_file(active_remote_usb_port(),"/config/runtime.json"),"application/json")
            except Exception as e: self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",503)
        elif self.path.startswith("/db-info"): self.send_data(json.dumps(db_info()),"application/json")
        elif self.path.startswith("/OpenRemote.png"):
            self.send_data((APP_DIR/"OpenRemote.png").read_bytes(),"image/png")
        elif self.path.startswith("/product.png"):
            p=APP_DIR/"openremote_product.png"; self.send_data(p.read_bytes(),"image/png") if p.exists() else self.send_data("Missing image",code=404)
        elif self.path.startswith("/search"):
            q=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get("q",[""])[0]; self.send_data(json.dumps(search_db(q),ensure_ascii=False),"application/json")
        elif self.path.startswith("/device"):
            id=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get("id",[""])[0]; self.send_data(json.dumps(device_detail(id),ensure_ascii=False),"application/json")
        elif self.path.startswith("/usb-ports"):
            self.send_data(json.dumps({"ports":usb_ports()}),"application/json")
        elif self.path.startswith("/usb/transfer-status"):
            self.send_data(json.dumps(USB_TRANSFER_STATE),"application/json")
        elif self.path.startswith("/usb/ping"):
            port=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get("port",[""])[0].strip()
            if not port:
                self.send_data(json.dumps({"error":"Choose a USB serial port first."}),"application/json")
                return
            self.send_data(json.dumps(usb_ping_remote(port),ensure_ascii=False),"application/json")
        elif self.path.startswith("/usb/webconfig-load"):
            port=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get("port",[""])[0].strip()
            if not port:
                self.send_data(json.dumps({"error":"Choose a USB serial port first."}),"application/json")
                return
            try:
                html=usb_read_file(port,"/www/index.html")
                USB_WEBCONFIG_CACHE.update({"html":html,"loaded_at":time.time(),"port":normalize_usb_port(port),"size":len(html)})
                self.send_data(json.dumps({"ok":True,"url":"/usb/webconfig-view","size":len(html),"port":normalize_usb_port(port)}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"error":str(e)}),"application/json")
        elif self.path.startswith("/usb/webconfig-view"):
            if not USB_WEBCONFIG_CACHE["html"]:
                self.send_data("<h1>No WebConfig loaded</h1><p>Use Remote Config &gt; Connect first.</p>",code=404)
                return
            self.send_data(USB_WEBCONFIG_CACHE["html"],"text/html; charset=utf-8")
        elif self.path.startswith("/usb/webconfig"):
            port=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get("port",[""])[0].strip()
            if not port:
                self.send_data("<h1>Choose a USB serial port first.</h1>",code=400)
                return
            try:
                html=usb_read_file(port,"/www/index.html")
                self.send_data(html,"text/html; charset=utf-8")
            except Exception as e:
                self.send_data("<h1>Could not load WebConfig over USB</h1><p>"+str(e)+"</p>",code=500)
        elif self.path.startswith("/api/"):
            self.send_data(json.dumps({"ok":False,"error":"This WebConfig action requires the remote Wi-Fi connection."}),"application/json",501)
        else: self.send_data(HTML)
    def do_POST(self):
        mark_client_seen()
        if self.path.startswith("/setup/detect"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            try:
                request=self.read_json(); result=detect_factory_board(request.get("port",""))
                self.send_data(json.dumps(result),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/setup/choose-firmware"):
            try:
                path=choose_setup_file("firmware")
                if not path:
                    self.send_data(json.dumps({"ok":False,"cancelled":True}),"application/json"); return
                info=inspect_firmware_file(path); SETUP_SELECTION["firmware"]=info
                SETUP_SELECTION["firmwareInstalled"]=False
                self.send_data(json.dumps({"ok":True,"selection":SETUP_SELECTION}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/setup/choose-webconfig"):
            try:
                path=choose_setup_file("webconfig")
                if not path:
                    self.send_data(json.dumps({"ok":False,"cancelled":True}),"application/json"); return
                info=inspect_webconfig_file(path); SETUP_SELECTION["webConfig"]=info
                self.send_data(json.dumps({"ok":True,"selection":SETUP_SELECTION}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/setup/flash-sensor"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            try:
                request=self.read_json(); port=(request.get("port") or "").strip()
                if not port: raise RuntimeError("Choose the ESP32 USB serial port first.")
                verify_sensor_test_image()
                start_factory_job(flash_sensor_test_board,(port,))
                self.send_data(json.dumps({"ok":True}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/setup/flash"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            try:
                request=self.read_json(); port=(request.get("port") or "").strip()
                if not port: raise RuntimeError("Choose the ESP32 USB serial port first.")
                selected_factory_image()
                start_factory_job(flash_factory_board,(port,))
                self.send_data(json.dumps({"ok":True}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/setup/install-all"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            length=int(self.headers.get("Content-Length","0") or 0)
            body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: request=json.loads(body)
            except Exception: request={}
            port=(request.get("port") or "").strip()
            if not port:
                self.send_data(json.dumps({"ok":False,"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            # A dock is firmware and nothing else: no SD card, no WebConfig, no
            # icon or theme libraries. Requiring a WebConfig .html here - and
            # then running the SD preparation - would be asking for files that
            # have nowhere to go on an ESP32-C3.
            is_dock=(SETUP_SELECTION.get("detectedDevice")=="dock")
            try:
                require_setup_files(include_webconfig=not is_dock)
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400); return
            if is_dock:
                reset_factory_state("install-all","Installing dock firmware...","dock")
                start_factory_job(flash_factory_board,(port,))
            else:
                reset_factory_state("install-all","Installing firmware...","setup")
                start_factory_job(install_firmware_and_sd,(port,))
            self.send_data(json.dumps({"ok":True}),"application/json")
        elif self.path.startswith("/setup/prepare-remote-sd"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            length=int(self.headers.get("Content-Length","0") or 0)
            body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: request=json.loads(body)
            except Exception: request={}
            port=(request.get("port") or "").strip()
            if not port:
                self.send_data(json.dumps({"ok":False,"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            recovery=bool(request.get("recovery"))
            if not recovery:
                try:
                    require_setup_files(include_webconfig=True)
                except Exception as e:
                    self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400); return
            reset_factory_state("remote-sd","Preparing the SD card inside the remote...",
                                "recovery" if recovery else "setup")
            start_factory_job(prepare_remote_sd,(port,recovery))
            self.send_data(json.dumps({"ok":True}),"application/json")
        elif self.path.startswith("/setup/prepare-sd"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another setup operation is already running."}),"application/json",409); return
            try:
                require_setup_files(include_webconfig=True)
                target=choose_sd_folder()
                if not target:
                    self.send_data(json.dumps({"ok":False,"cancelled":True}),"application/json"); return
                reset_factory_state("sd","Checking the SD card in this computer...","setup")
                start_factory_job(prepare_sd_card,(target,))
                self.send_data(json.dumps({"ok":True,"path":target}),"application/json")
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/api/config"):
            length=int(self.headers.get("Content-Length","0") or 0); payload=self.rfile.read(length) if length else b""
            try:
                json.loads(payload.decode("utf-8"))
                response=usb_write_file(active_remote_usb_port(),"/config/runtime.json",payload)
                self.send_data(json.dumps(response,ensure_ascii=False),"application/json")
            except Exception as e: self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400)
        elif self.path.startswith("/start-build"):
            if STATE["running"]: self.send_data(json.dumps({"ok":True}),"application/json"); return
            length=int(self.headers.get("Content-Length","0") or 0); body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: selected=json.loads(body).get("sources",[])
            except Exception: selected=[]
            folder=choose_folder()
            if not folder: self.send_data(json.dumps({"error":"Save cancelled."}),"application/json"); return
            def worker():
                try: build_database(folder,selected)
                except Exception as e:
                    STATE["error"]=str(e); STATE["running"]=False; log("ERROR: "+str(e))
            threading.Thread(target=worker,daemon=True).start(); self.send_data(json.dumps({"ok":True}),"application/json")
        elif self.path.startswith("/upload-db"):
            length=int(self.headers.get("Content-Length","0") or 0)
            if length<=0: self.send_data(json.dumps({"error":"No file uploaded."}),"application/json"); return
            remaining=length
            with open(ACTIVE_DB,"wb") as f:
                while remaining>0:
                    chunk=self.rfile.read(min(1024*1024,remaining))
                    if not chunk: break
                    f.write(chunk); remaining-=len(chunk)
            if remaining:
                ACTIVE_DB.unlink(missing_ok=True)
                self.send_data(json.dumps({"error":"The database upload ended early."}),"application/json",400); return
            STATE["db_path"]=str(ACTIVE_DB); STATE["bin_mb"]=ACTIVE_DB.stat().st_size/1024/1024
            self.send_data(json.dumps({"ok":True,"path":str(ACTIVE_DB)}),"application/json")
        elif self.path.startswith("/usb/factory-reset"):
            length=int(self.headers.get("Content-Length","0") or 0)
            body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: request=json.loads(body)
            except Exception: request={}
            port=(request.get("port") or "").strip()
            if not port:
                self.send_data(json.dumps({"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            self.send_data(json.dumps(usb_factory_reset(port),ensure_ascii=False),"application/json")
        elif self.path.startswith("/usb/ir-exists"):
            query=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            port=(query.get("port",[""])[0] or "").strip()
            name=(query.get("name",[""])[0] or "").strip()
            if not port or not name:
                self.send_data(json.dumps({"ok":False,"error":"Missing port or filename."}),"application/json",400); return
            exists=usb_file_exists(port,"/devices/"+name)
            self.send_data(json.dumps({"ok":True,"exists":exists}),"application/json")
        elif self.path.startswith("/usb/load-backup") or self.path.startswith("/usb/load-ir"):
            # One handler for both Recovery slots. Which button was clicked no
            # longer decides how the file is treated - the contents do. A
            # WebConfig learned-device export is named ".ir" but holds
            # OpenRemote JSON, and a category export is a ".json" a user will
            # reasonably drop into either slot; routing on the file extension
            # or the button rejected both of those outright.
            if WEBCONFIG_STATE.get("running"):
                self.send_data(json.dumps({"ok":False,"error":"A USB transfer is already running."}),"application/json",409); return
            query=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            port=(query.get("port",[""])[0] or "").strip()
            name=(query.get("name",[""])[0] or "backup.json").strip()
            length=int(self.headers.get("Content-Length","0") or 0)
            payload=self.rfile.read(length) if length else b""
            if not port:
                self.send_data(json.dumps({"ok":False,"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            safe=re.sub(r"[^0-9A-Za-z._-]","_",name) or "backup.json"
            if payload[:64].decode("utf-8","ignore").startswith("Filetype: IR signals file"):
                # A real Flipper signals file: the remote plays these straight
                # from /devices, so it goes there and needs no restore step.
                if not safe.lower().endswith(".ir"): safe+=".ir"
                start_factory_job(send_recovery_file,(port,"/devices/"+safe,payload,"Sending IR device to the SD card...",safe))
                self.send_data(json.dumps({"ok":True,"bytes":len(payload),"name":safe,"backup":False}),"application/json")
                return
            try:
                summary=describe_backup_payload(payload)
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)+" Studio accepts Flipper-format .ir signals files, and OpenRemote full backups, category exports and learned-device .ir exports."}),"application/json",400); return
            # OpenRemote JSON of any category - including a learned-device .ir.
            # It describes configuration, so it belongs in /backups where the
            # remote's restore can apply it, never in /devices.
            if not safe.lower().endswith((".json",".ir")): safe+=".json"
            start_factory_job(send_recovery_file,(port,"/backups/"+safe,payload,"Sending backup to the SD card...",None))
            self.send_data(json.dumps({"ok":True,"bytes":len(payload),"summary":summary,"name":safe,"backup":True}),"application/json")
        elif self.path.startswith("/usb/restore-backup"):
            length=int(self.headers.get("Content-Length","0") or 0)
            body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: request=json.loads(body)
            except Exception: request={}
            port=(request.get("port") or "").strip()
            name=(request.get("name") or "").strip()
            if not port or not name:
                self.send_data(json.dumps({"error":"Missing port or backup name."}),"application/json",400); return
            self.send_data(json.dumps(usb_restore_backup(port,name),ensure_ascii=False),"application/json")
        elif self.path.startswith("/usb/flash-firmware"):
            if factory_job_running():
                self.send_data(json.dumps({"ok":False,"error":"Another operation is already running."}),"application/json",409); return
            query=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            port=(query.get("port",[""])[0] or "").strip()
            length=int(self.headers.get("Content-Length","0") or 0)
            payload=self.rfile.read(length) if length else b""
            if not port:
                self.send_data(json.dumps({"ok":False,"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            if not payload:
                self.send_data(json.dumps({"ok":False,"error":"Choose a firmware .bin file first."}),"application/json",400); return
            try:
                start_factory_job(flash_remote_application,(port,payload))
            except Exception as e:
                self.send_data(json.dumps({"ok":False,"error":str(e)}),"application/json",400); return
            self.send_data(json.dumps({"ok":True,"bytes":len(payload)}),"application/json")
        elif self.path.startswith("/usb/install-webconfig"):
            if WEBCONFIG_STATE.get("running"):
                self.send_data(json.dumps({"ok":False,"error":"A WebConfig install is already running."}),"application/json",409); return
            query=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            port=(query.get("port",[""])[0] or "").strip()
            length=int(self.headers.get("Content-Length","0") or 0)
            payload=self.rfile.read(length) if length else b""
            if not port:
                self.send_data(json.dumps({"ok":False,"error":"Choose the remote's USB serial port first."}),"application/json",400); return
            if not payload:
                self.send_data(json.dumps({"ok":False,"error":"Choose a WebConfig .html file first."}),"application/json",400); return
            head=payload[:768].lower()
            if b"<html" not in head or b"openremote-webconfig-version" not in head:
                self.send_data(json.dumps({"ok":False,"error":"That file is not an OpenRemote WebConfig .html."}),"application/json",400); return
            if b"</html>" not in payload[-512:].lower():
                self.send_data(json.dumps({"ok":False,"error":"That WebConfig file looks truncated (no closing </html>)."}),"application/json",400); return
            threading.Thread(target=install_webconfig_over_usb,args=(port,payload),daemon=True).start()
            self.send_data(json.dumps({"ok":True,"bytes":len(payload)}),"application/json")
        elif self.path.startswith("/usb/import-device"):
            length=int(self.headers.get("Content-Length","0") or 0); body=self.rfile.read(length).decode("utf-8") if length else "{}"
            try: request=json.loads(body)
            except Exception: request={}
            port=(request.get("port") or "").strip(); id=(request.get("id") or "").strip()
            if not port or not id:
                self.send_data(json.dumps({"error":"Choose a USB serial port and IRDB device first."}),"application/json"); return
            self.send_data(json.dumps(usb_import_device(port,id),ensure_ascii=False),"application/json")
        elif self.path.startswith("/api/"):
            self.send_data(json.dumps({"ok":False,"error":"This WebConfig action requires the remote Wi-Fi connection."}),"application/json",501)
        else: self.send_data("Not found",code=404)
def open_app_window(url,title="OpenRemote Studio"):
    # A real app window instead of webbrowser.open()'s plain browser tab: no
    # URL bar, no tabs, and it uses the OS's own built-in web engine
    # (WKWebView on Mac, WebView2 on Windows) rather than depending on some
    # particular browser being installed - the engine is part of the OS
    # itself and gets updated by the OS, not by this app.
    import webview
    return webview.create_window(title,url,width=1483,height=860,min_size=(900,600))

def show_app_window_or_fallback(url,on_close=None):
    # Best-effort native app window, not an assumed one. pywebview's Windows
    # backend needs the WebView2 runtime (and .NET, via pythonnet) actually
    # present on that machine - Mac's WKWebView backend is verified working
    # here, Windows could not be tested at all (no Windows machine available
    # to this build), so this has to degrade rather than fail outright on a
    # machine where the native backend does not come up for any reason.
    # Falling back to the plain browser tab this app always used before means
    # a native-window failure is never worse than "works like it always did"
    # - which matters more than always looking the same everywhere.
    try:
        import webview
        window=open_app_window(url)
        if on_close: window.events.closed+=on_close
        webview.start()
        return True
    except Exception as error:
        log(f"Native app window unavailable ({error}); opening in the default browser instead.")
        webbrowser.open(url)
        return False

def main():
    global APP_SERVER
    if not acquire_instance_lock():
        # Another instance already owns the server - open a window onto it
        # rather than starting a second server. The window closing here does
        # not touch the other process; that one keeps running exactly as it
        # was.
        url=existing_instance_url()
        if url: show_app_window_or_fallback(url)
        return
    server=None
    serverThread=None
    try:
        s=socket.socket(); s.bind(("127.0.0.1",0)); port=s.getsockname()[1]; s.close()
        server=ThreadingHTTPServer(("127.0.0.1",port),Handler)
        APP_SERVER=server
        INSTANCE_STATE.write_text(json.dumps({"pid":os.getpid(),"port":port,"version":APP_VERSION}),encoding="utf-8")
        threading.Thread(target=idle_server_watchdog,args=(server,),daemon=True).start()
        # serve_forever() now runs on its own thread rather than blocking
        # main(): webview.start() must own the main thread itself (required
        # by Cocoa's run loop on Mac; kept the same on Windows for one
        # consistent code path rather than two).
        serverThread=threading.Thread(target=server.serve_forever,daemon=True)
        serverThread.start()
        url=f"http://127.0.0.1:{port}/"
        # idle_server_watchdog is still a real safety net (e.g. the window
        # object failing to report its own close event), but a native window
        # gives an actual "closed" signal a browser tab never could - passed
        # as on_close so the app quits the moment its window does, like a
        # normal desktop app, instead of waiting out the watchdog's 15s idle
        # timer.
        nativeWindowShown=show_app_window_or_fallback(url,on_close=server.shutdown)
        if nativeWindowShown:
            # webview.start() already blocked until the window closed and
            # fired on_close; make sure serve_forever() has actually
            # returned before falling through to cleanup.
            server.shutdown()
            serverThread.join(timeout=5)
        else:
            # No native window ever opened - behave exactly as this app
            # always has: the browser tab is the app, and
            # idle_server_watchdog alone decides when to shut down once the
            # tab stops making requests.
            serverThread.join()
    finally:
        APP_SERVER=None
        if server:
            try: server.server_close()
            except Exception: pass
        try:
            state=json.loads(INSTANCE_STATE.read_text(encoding="utf-8"))
            if int(state.get("pid",0))==os.getpid(): INSTANCE_STATE.unlink(missing_ok=True)
        except Exception:
            pass
        release_instance_lock()
if __name__=="__main__": main()
