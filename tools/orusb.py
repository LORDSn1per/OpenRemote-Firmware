"""ORUSB client for the remote over USB serial.

Two things have to be right and neither is obvious:
  * replies are prefixed "ORUSB " and share the line with nothing, but the
    firmware's own logging is interleaved on the same port, so the JSON has to
    be found by its prefix rather than by assuming it starts a line;
  * the remote uses automatic light sleep, and the first bytes sent to a
    sleeping remote are consumed waking it up rather than parsed - so the
    handshake has to be retried, which is what Studio does too.
"""
import serial, sys, time, json

PORT="/dev/cu.usbserial-1330"; BAUD=460800

def _find_reply(buf):
    i=buf.find(b"ORUSB {")
    if i<0: return None
    end=buf.find(b"\n", i)
    chunk=buf[i+6:end if end>=0 else len(buf)].strip()
    try: return json.loads(chunk.decode("utf-8","replace"))
    except Exception: return None

def command(cmd, wait=25, port=None):
    own = port is None
    p = port or serial.Serial(port=PORT, baudrate=BAUD, timeout=0.2)
    try:
        deadline=time.time()+wait
        while time.time()<deadline:
            p.write(b"\n"+cmd.encode()+b"\n"); p.flush()
            t=time.time()+2.0; buf=b""
            while time.time()<t:
                buf+=p.read(4096)
                r=_find_reply(buf)
                if r is not None: return r
                time.sleep(0.03)
            time.sleep(0.3)
        return None
    finally:
        if own: p.close()

if __name__=="__main__":
    print(json.dumps(command(sys.argv[1] if len(sys.argv)>1 else "ORUSB PING")))
