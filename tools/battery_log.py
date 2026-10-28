#!/usr/bin/env python3
"""Fetch the remote's battery log (firmware 5.97+) and say where the charge went.

    battery_log.py --host 192.168.1.50 --token <token from the WebConfig link>
    battery_log.py --usb /dev/cu.usbserial-120
    battery_log.py --file battery.csv

Over Wi-Fi the remote's web server only runs while WebConfig is open, and it
wants the token from the WebConfig link. The token is never stored: pass it,
or set OPENREMOTE_TOKEN for the one command. Over USB, opening the port may
restart the remote; the log is on the SD card and survives that.

The log has one line per power event; between two lines the remote was in the
state the first one entered. Percentage drops while not charging are summed
per state, with an estimated average current for the 2000 mAh cell. The
MAX17048 measures no current, so every mA here is derived from the percentage.
"""
import argparse, csv, io, os, sys, urllib.request
from collections import defaultdict

CAPACITY_MAH = 2000

# The state each event leaves the remote in.
STATE_AFTER = {
    "boot": "awake", "screen-on": "awake", "screen-off": "screen off",
    "ble-idle": "BLE idle", "ble-idle-exit": "screen off",
    "light-sleep": "light sleep", "light-wake": "screen off",
    "deep-sleep": "deep sleep",
}


def fetch_wifi(host, token, old=False):
    url = f"http://{host}/api/battery/log?token={token}" + ("&old=1" if old else "")
    with urllib.request.urlopen(url, timeout=20) as r:
        return r.read().decode("utf-8", "replace")


def fetch_usb(port):
    here = os.path.dirname(os.path.abspath(__file__))
    sys.path.insert(0, os.path.join(here, "..", "studio", "Windows", "app"))
    import openremote_studio as studio   # reuses Studio's ORUSB READ client
    return studio.usb_read_file(port, "/logs/battery.csv").decode("utf-8", "replace")


def summarise(text):
    rows = [r for r in csv.DictReader(io.StringIO(text)) if r.get("event")]
    if not rows:
        print("The log is empty.")
        return
    per_state = defaultdict(lambda: [0.0, 0.0])   # hours, percent used
    wakes = defaultdict(int)
    state = "awake"
    for a, b in zip(rows, rows[1:]):
        state = STATE_AFTER.get(a["event"], state)
        if a["event"] == "boot":
            wakes[a["detail"].split("wake=")[-1].split(" ")[0] or "?"] += 1
        if a["event"] == "light-wake":
            wakes["light: " + a["detail"].split(" ")[0]] += 1
        t0, t1 = int(a["epoch"]), int(b["epoch"])
        # Without a real clock on both lines the gap is unknown; so is any
        # interval spent charging.
        if not t0 or not t1 or t1 <= t0 or a["charging"] == "1" or b["charging"] == "1":
            continue
        hours = (t1 - t0) / 3600.0
        used = float(a["percent"]) - float(b["percent"])
        per_state[state][0] += hours
        per_state[state][1] += used

    first, last = rows[0], rows[-1]
    print(f"{len(rows)} lines, {first['percent']}% -> {last['percent']}%")
    print(f"{'state':<13}{'hours':>8}{'% used':>9}{'%/h':>8}{'~mA':>8}")
    total_h = total_p = 0.0
    for name, (hours, used) in sorted(per_state.items(), key=lambda kv: -kv[1][0]):
        rate = used / hours if hours > 0 else 0.0
        print(f"{name:<13}{hours:8.2f}{used:9.2f}{rate:8.2f}{rate / 100 * CAPACITY_MAH:8.1f}")
        total_h += hours
        total_p += used
    if total_h:
        rate = total_p / total_h
        print(f"{'all':<13}{total_h:8.2f}{total_p:9.2f}{rate:8.2f}{rate / 100 * CAPACITY_MAH:8.1f}")
    if wakes:
        print("wakes: " + ", ".join(f"{k} x{v}" for k, v in sorted(wakes.items(), key=lambda kv: -kv[1])))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--host")
    src.add_argument("--usb")
    src.add_argument("--file")
    ap.add_argument("--token", default=os.environ.get("OPENREMOTE_TOKEN", ""))
    ap.add_argument("--old", action="store_true", help="the rotated battery.prev.csv (Wi-Fi only)")
    ap.add_argument("--raw", action="store_true", help="print the CSV instead of the summary")
    args = ap.parse_args()
    if args.host:
        if not args.token:
            sys.exit("--token (or OPENREMOTE_TOKEN) is required over Wi-Fi")
        text = fetch_wifi(args.host, args.token, args.old)
    elif args.usb:
        text = fetch_usb(args.usb)
    else:
        text = open(args.file, encoding="utf-8").read()
    if args.raw:
        print(text, end="")
    else:
        summarise(text)


if __name__ == "__main__":
    main()
