#!/usr/bin/env python3
"""Drive the GyverLibs Settings panel over its WebSocket protocol, no browser.

    uv run --with websockets sets_ws.py <lamp-ip> [--pass smartlamp] CMD ...

Commands:
    load                       request the full page build (what a browser does on open)
    ping N                     keep the panel "focused" for N seconds (ping every 2 s)
    set KEY VALUE              KEY: app key (lamp|brgt|efcx), db key (wifi_ssid, mqtt_host, ...),
                               or effect param as "Effect.param" (paramKey FNV-1a)
    click LABEL                press a button ("Переподключить" etc. - hash of label text)
    spam Effect.param MIN MAX N DELAY_MS    slider spam
    effects N DELAY_MS         cycle efcx 0..11 N times
    twin                       open a second client that only pings (run in a second process)

Frame = <H pid><I auth><I action><I id> + payload text. Hashes = su::hash (h = h*33 + c, 32-bit),
effect params = FNV-1a("Effect.param") as in src/hal/Storage.h.
"""
import asyncio, struct, sys, time
import websockets

def su(s: str) -> int:
    h = 0
    for c in s.encode("utf-8"):
        h = (h + (h << 5) + c) & 0xFFFFFFFF
    return h

def fnv(effect: str, key: str) -> int:
    h = 2166136261
    for c in (effect + "." + key).encode("utf-8"):
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h

APP = {"lamp": 0x6C616D70, "brgt": 0x62726774, "efcx": 0x65666378}

def key_id(k: str) -> int:
    if k in APP: return APP[k]
    if "." in k:
        e, p = k.split(".", 1)
        return fnv(e, p)
    return su(k)

def frame(action: str, ident: int, value: str, auth: int, pid: int = 1) -> bytes:
    return struct.pack("<HIII", pid, auth, su(action), ident) + value.encode("utf-8")

async def main():
    args = sys.argv[1:]
    ip = args.pop(0)
    auth = 0
    if args and args[0] == "--pass":
        args.pop(0); auth = su(args.pop(0))
    cmd = args.pop(0)
    rx = 0
    async with websockets.connect(f"ws://{ip}/", max_size=None, ping_interval=None) as ws:
        async def drain(timeout=0.3):
            nonlocal rx
            try:
                while True:
                    m = await asyncio.wait_for(ws.recv(), timeout)
                    rx += len(m)
            except asyncio.TimeoutError:
                pass
        t0 = time.time()
        if cmd == "load":
            await ws.send(frame("load", 0, f"{int(time.time()):x}", auth)); await drain(1.5)
        elif cmd == "ping":
            end = time.time() + float(args[0])
            while time.time() < end:
                await ws.send(frame("ping", 0, "", auth)); await drain(2.0)
        elif cmd == "set":
            await ws.send(frame("set", key_id(args[0]), args[1], auth)); await drain()
        elif cmd == "click":
            a = args[0]
            ident = int(a, 16) if a.startswith("0x") else su(a)
            await ws.send(frame("click", ident, "", auth)); await drain()
        elif cmd == "mem":
            # Full page load; the "Память" label carries free/min/block/stack as text.
            import re
            got = b""
            async def drain_mem(timeout=1.5):
                nonlocal got, rx
                try:
                    while True:
                        m = await asyncio.wait_for(ws.recv(), timeout)
                        rx += len(m); got += m if isinstance(m, bytes) else m.encode()
                except asyncio.TimeoutError:
                    pass
            await ws.send(frame("load", 0, f"{int(time.time()):x}", auth)); await drain_mem()
            txt = got.decode("utf-8", "replace")
            m = re.search(r"свободно (\d+) · минимум (\d+) · блок (\d+)(?: · стек loop (\d+))?", txt)
            fx = re.search(r"IP [\d.]+ · RSSI (-?\d+) · (\d+) к/с", txt)
            print("mem:", m.groups() if m else None, "rssi/fps:", fx.groups() if fx else None, "page", len(got))
        elif cmd == "spam":
            k, lo, hi, n, d = args[0], int(args[1]), int(args[2]), int(args[3]), int(args[4]) / 1000
            v = lo
            for i in range(n):
                v = lo + (i * 7) % (hi - lo + 1)
                await ws.send(frame("set", key_id(k), str(v), auth)); await drain(d)
        elif cmd == "effects":
            n, d = int(args[0]), int(args[1]) / 1000
            for i in range(n):
                await ws.send(frame("set", APP["efcx"], str(i % 12), auth)); await drain(d)
        elif cmd == "twin":
            while True:
                await ws.send(frame("ping", 0, "", auth)); await drain(2.0)
        print(f"{cmd}: rx {rx} bytes in {time.time() - t0:.1f}s")

asyncio.run(main())
