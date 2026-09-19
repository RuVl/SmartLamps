#!/usr/bin/env python3
"""Read-only serial capture for the d1_mini lamp.

    python3 serial_capture.py /dev/ttyUSB0 out/run1      # system python: uv's build of pyserial
                                                        # rejects the 74880 baud (termios EINVAL)

Writes out/run1.log (timestamped raw lines) and out/run1.csv (parsed `mem:` telemetry).
Flags exceptions, resets, OOM and WS-queue overflows to stderr and to out/run1.events.
Never writes to the port; DTR/RTS are held low before open so the board is not reset.
"""
import csv, os, re, sys, time
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
OUT = sys.argv[2] if len(sys.argv) > 2 else "out/run"
os.makedirs(os.path.dirname(OUT) or ".", exist_ok=True)

# Telemetry line printed by the LAMP_MEMLOG build once a second:
# mem: free=23456 blk=17800 frag=24 cont=2900 minfree=19000 minblk=12000 mincont=2800 sys=1900 fx=Огонь foc=1 mqtt=1 up=123
MEM = re.compile(r"^mem: (.*)$")
EVENT = re.compile(r"Exception \(|Fatal exception|Soft WDT|wdt reset|rst cause|:oom\(|"
                   r"Too many messages queued|Panic|Stack overflow|ets Jan|boot mode|"
                   r"WiFi: отключено|MQTT: отключено|reset: ", re.I)

s = serial.Serial()
s.port, s.baudrate, s.timeout = PORT, 74880, 1
s.dtr = False
s.rts = False
s.open()

log = open(OUT + ".log", "a", buffering=1)
ev = open(OUT + ".events", "a", buffering=1)
csvf = open(OUT + ".csv", "a", newline="")
w = csv.writer(csvf)
if csvf.tell() == 0:
    w.writerow(["t", "free", "blk", "frag", "cont", "minfree", "minblk", "mincont", "sys", "fx", "foc", "mqtt", "up"])

print(f"reading {PORT} @74880 -> {OUT}.*", file=sys.stderr)
buf = b""
while True:
    chunk = s.read(256)
    if not chunk:
        continue
    buf += chunk
    while b"\n" in buf:
        raw, buf = buf.split(b"\n", 1)
        line = raw.decode("utf-8", "replace").rstrip("\r")
        t = time.strftime("%H:%M:%S") + f".{int(time.time() * 1000) % 1000:03d}"
        log.write(f"{t} {line}\n")
        m = MEM.match(line)
        if m:
            kv = dict(p.split("=", 1) for p in m.group(1).split() if "=" in p)
            w.writerow([t] + [kv.get(k, "") for k in
                              ("free", "blk", "frag", "cont", "minfree", "minblk", "mincont", "sys", "fx", "foc", "mqtt", "up")])
            csvf.flush()
        elif EVENT.search(line):
            ev.write(f"{t} {line}\n")
            print(f"!! {t} {line}", file=sys.stderr)
