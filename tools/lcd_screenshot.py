"""Captures the remote's LCD over USB and saves it as PNG files.

Usage:
  lcd_screenshot.py <out-dir> [setup-command] [name]

  setup-command  sent before the capture, e.g. "ORUSB BRIGHTNESSPANEL" or
                 "ORUSB OVERLAYDEMO restore 62" (default: ORUSB BRIGHTNESSPANEL;
                 pass "" for none)
  name           file name without extension (default: screenshot)

Writes <name>.png at 240x320 and <name>-3x.png, scaled up for viewing.
Needs firmware 5.52+ (ORUSB SCREENSHOT, saved from the LCD frame mirror).

One process holds the port throughout, because opening it resets the remote;
the script waits for it to boot before sending anything.
"""
import json, re, struct, sys, time, zlib
import serial

PORT = "/dev/cu.usbserial-1330"
BAUD = 460800
OUT = sys.argv[1]
SETUP = sys.argv[2] if len(sys.argv) > 2 else "ORUSB BRIGHTNESSPANEL"
NAME = sys.argv[3] if len(sys.argv) > 3 else "screenshot"


def readline(p, timeout):
    deadline = time.time() + timeout
    buf = b""
    while time.time() < deadline:
        c = p.read(1)
        if not c:
            continue
        buf += c
        if c == b"\n":
            text = buf.decode("utf-8", "ignore").strip()
            buf = b""
            if text.startswith("ORUSB "):
                return text[6:]
    raise TimeoutError("no ORUSB line")


def read_exact(p, n, timeout=15):
    deadline = time.time() + timeout
    data = bytearray()
    while len(data) < n and time.time() < deadline:
        data.extend(p.read(n - len(data)))
    if len(data) < n:
        raise TimeoutError("short read")
    return bytes(data)


def json_reply(p, command, timeout=6, tries=8):
    for _ in range(tries):
        p.write(b"\n" + command.encode() + b"\n")
        p.flush()
        end = time.time() + timeout
        try:
            while time.time() < end:
                line = readline(p, end - time.time())
                if line.startswith("{"):
                    return json.loads(line)
        except TimeoutError:
            pass
    raise RuntimeError("no reply to " + command)


def png(width, height, rgb):
    def chunk(kind, body):
        return (struct.pack(">I", len(body)) + kind + body +
                struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        rows += rgb[y * width * 3:(y + 1) * width * 3]
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


p = serial.Serial(PORT, BAUD, timeout=0.2)
time.sleep(6)                      # opening the port reset the remote
p.reset_input_buffer()
hello = json_reply(p, "ORUSB PING")
print("remote firmware:", hello.get("firmwareVersion"))
if SETUP:
    print(json_reply(p, SETUP))
    time.sleep(1.5)                # let it draw
shot = json_reply(p, "ORUSB SCREENSHOT", timeout=15, tries=1)
print(shot)
if not shot.get("ok"):
    sys.exit(1)
width, height = shot["width"], shot["height"]

p.write(("ORUSB READ %s\n" % shot["path"]).encode())
header = readline(p, 10)
while not header.startswith("FILE"):
    if header.startswith("{"):
        raise RuntimeError(header)
    header = readline(p, 10)
size = int(re.fullmatch(r"FILE (\d+) CHUNKED", header).group(1))
data = bytearray()
while True:
    # Always ask: the firmware answers DONE to a NEXT once everything is sent.
    p.write(b"ORUSB NEXT\n")
    frame = readline(p, 15)
    if frame == "DONE":
        break
    if frame.startswith("{"):
        raise RuntimeError(frame)
    m = re.fullmatch(r"DATA (\d+) (\d+)", frame)
    if not m:
        continue
    offset, length = int(m.group(1)), int(m.group(2))
    if offset != len(data):
        raise RuntimeError("out of sequence at %d" % offset)
    data.extend(read_exact(p, length))
    if read_exact(p, 1) == b"\r":
        read_exact(p, 1)
p.close()
if len(data) != width * height * 2:
    raise RuntimeError("got %d bytes, expected %d" % (len(data), width * height * 2))

rgb = bytearray(width * height * 3)
for i in range(width * height):
    v = data[i * 2] | (data[i * 2 + 1] << 8)
    rgb[i * 3] = ((v >> 11) & 31) * 255 // 31
    rgb[i * 3 + 1] = ((v >> 5) & 63) * 255 // 63
    rgb[i * 3 + 2] = (v & 31) * 255 // 31
open("%s/%s.png" % (OUT, NAME), "wb").write(png(width, height, rgb))

scale = 3
big = bytearray(width * scale * height * scale * 3)
for y in range(height * scale):
    for x in range(width * scale):
        s = ((y // scale) * width + (x // scale)) * 3
        d = (y * width * scale + x) * 3
        big[d:d + 3] = rgb[s:s + 3]
open("%s/%s-3x.png" % (OUT, NAME), "wb").write(png(width * scale, height * scale, big))
print("saved", len(data), "bytes")
