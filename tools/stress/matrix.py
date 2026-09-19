#!/usr/bin/env python3
"""Stress matrix against the lamp's Settings panel (WS) and MQTT.
uv run --quiet --with websockets --with paho-mqtt matrix.py <case> [<case>...]
Env: LAMP, NAME, BROKER, PASS, OUT (csv prefix)."""
import asyncio, os, re, struct, sys, time, threading
import websockets
import paho.mqtt.client as mqtt

LAMP = os.environ.get("LAMP", "192.168.31.220")
NAME = os.environ.get("NAME", "Lamp8266")
BROKER = os.environ.get("BROKER", "192.168.31.21")
PASS = os.environ.get("PASS", "")
OUT = os.environ.get("OUT", "out/matrix")

def su(s):
    h = 0
    for c in s.encode(): h = (h + (h << 5) + c) & 0xFFFFFFFF
    return h
def fnv(e, k):
    h = 2166136261
    for c in (e + "." + k).encode(): h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h
AUTH = su(PASS) if PASS else 0
APP = {"lamp": 0x6C616D70, "brgt": 0x62726774, "efcx": 0x65666378}
EFF = ["Тест", "Конфетти", "Снегопад", "Радуга", "Пейнтбол", "Шум 3D", "Матрица", "Светлячки",
       "Огонь", "Блуждающий кубик", "Смена цвета", "Метель"]
PARAMS = {  # key -> (min, max); Switch/Select spammed as 0..n
 "Смена цвета": {"speed": (1,100), "saturation": (0,100)},
 "Огонь": {"speed": (1,100), "cooling": (10,100), "spark": (10,200), "hue": (0,60)},
 "Светлячки": {"speed": (1,100), "count": (1,12), "trace": (0,1)},
 "Матрица": {"speed": (1,100), "density": (1,100)},
 "Тест": {"pattern": (0,3)},
 "Пейнтбол": {"speed": (1,100), "blur": (1,100)},
 "Метель": {"speed": (1,100), "density": (1,100), "tail": (0,100)},
 "Шум 3D": {"speed": (1,100), "scale": (1,100), "palette": (0,3)},
 "Снегопад": {"speed": (1,100), "density": (1,100)},
 "Конфетти": {"speed": (1,100), "density": (1,30), "fade": (5,100)},
 "Радуга": {"speed": (1,100), "scale": (1,100), "dir": (0,1)},
 "Блуждающий кубик": {"speed": (1,100), "size": (1,5)},
}
MEM_RE = re.compile(r"свободно (\d+) · минимум (\d+) · блок (\d+)(?: · стек loop (\d+))?")

csvf = open(OUT + ".csv", "a", buffering=1)
if csvf.tell() == 0: csvf.write("t,case,step,free,min,blk,cont,page_bytes,complete\n")
def log(*a): print(time.strftime("%H:%M:%S"), *a, flush=True)

# ---- MQTT observer ---------------------------------------------------------
state = {"last": None, "count": 0, "avail": None}
def on_msg(c, u, m):
    if m.topic.endswith("/state"): state["last"] = m.payload.decode("utf-8", "replace"); state["count"] += 1
    elif m.topic.endswith("/avail"): state["avail"] = m.payload.decode()
mq = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mq.on_message = on_msg
if not os.environ.get("NOMQTT"):
    mq.connect(BROKER, 1883, 30); mq.subscribe(f"lamp/{NAME}/#"); mq.loop_start()
def pub(cmd, val): mq.publish(f"lamp/{NAME}/cmd/{cmd}", str(val), qos=0)

# ---- WS driver -------------------------------------------------------------
class Panel:
    def __init__(self): self.ws = None; self.rx = 0
    async def __aenter__(self):
        self.ws = await websockets.connect(f"ws://{LAMP}/", max_size=None, ping_interval=None, open_timeout=10); return self
    async def __aexit__(self, *a): await self.ws.close()
    def frame(self, action, ident, value=""):
        return struct.pack("<HIII", 1, AUTH, su(action), ident) + value.encode()
    async def drain(self, timeout=0.3):
        got = b""
        try:
            while True:
                m = await asyncio.wait_for(self.ws.recv(), timeout)
                got += m if isinstance(m, bytes) else m.encode()
        except asyncio.TimeoutError: pass
        self.rx += len(got); return got
    async def send(self, action, ident=0, value="", timeout=0.3):
        await self.ws.send(self.frame(action, ident, value)); return await self.drain(timeout)
    async def set(self, key, value, timeout=0.3):
        ident = APP[key] if key in APP else (fnv(*key.split(".", 1)) if "." in key else su(key))
        return await self.send("set", ident, str(value), timeout)
    async def load(self, timeout=1.5):
        return await self.send("load", 0, f"{int(time.time()):x}", timeout)
    async def mem(self, case, step):
        page = await self.load()
        txt = page.decode("utf-8", "replace")
        m = MEM_RE.search(txt)
        complete = "Применить имя" in txt  # last widget of the last menu
        row = [time.strftime("%H:%M:%S"), case, step] + (list(m.groups()) if m else ["", "", "", ""]) + [len(page), int(complete)]
        csvf.write(",".join(str(x if x is not None else "") for x in row) + "\n")
        log(f"[{case}/{step}] free={m.group(1) if m else '?'} blk={m.group(3) if m else '?'} cont={m.group(4) if m else '?'} page={len(page)} complete={complete}")
        return m

# ---- cases ----------------------------------------------------------------
async def c_load(p, n=10):
    sizes = []
    for i in range(n):
        page = await p.load(); sizes.append(len(page))
    log(f"load x{n}: sizes {sizes}")
    await p.mem("load", "after")

async def c_sliders(p, delay=0.25, steps=40, effects=EFF):
    for idx, e in enumerate(EFF):
        if e not in effects: continue
        await p.set("efcx", idx); await asyncio.sleep(0.6); await p.load()
        for k, (lo, hi) in PARAMS[e].items():
            for i in range(steps):
                v = lo + (i * 7) % (hi - lo + 1)
                await p.set(f"{e}.{k}", v, timeout=delay)
        await p.mem("sliders", f"{e}@{int(delay*1000)}ms")

async def c_fastfx(p):
    for d in (0.15, 0.06, 0.02):
        for i in range(24): await p.set("efcx", i % 12, timeout=d)
        await asyncio.sleep(1.0)
        await p.mem("fastfx", f"{int(d*1000)}ms")
        log("  state:", state["last"])

async def c_power(p):
    for r in range(10):
        await p.set("lamp", 0, 0.15); await p.set("efcx", r % 12, 0.1); await p.set("lamp", 1, 0.4)
    await asyncio.sleep(1); await p.mem("power", "after"); log("  state:", state["last"])

async def twin_task(stop):
    async with Panel() as q:
        while not stop.is_set(): await q.send("ping", 0, "", 2.0)

async def c_twin(p):
    stop = asyncio.Event(); t = asyncio.create_task(twin_task(stop))
    await asyncio.sleep(1); await c_load(p); await c_sliders(p, effects=["Огонь", "Шум 3D", "Тест"])
    stop.set(); await t; await p.mem("twin", "after")

async def c_mqttburst(p):
    for r in range(5):
        before = state["count"]
        pub("on", 1); pub("brightness", 30 + r); pub("effect", EFF[(r + 3) % 12])
        for i in range(9): pub(f"param/speed", 10 + i)
        await asyncio.sleep(2.5)
        log(f"burst {r}: state msgs +{state['count'] - before}; last {state['last']}")
    await p.mem("mqttburst", "after")

async def mqtt_soak(seconds, stop):
    t0 = time.time(); i = 0
    while time.time() - t0 < seconds and not stop.is_set():
        pub("brightness", 20 + (i * 13) % 80); i += 1
        if i % 5 == 0: pub("effect", EFF[i % 12])
        await asyncio.sleep(1.0)

async def c_both(p):
    stop = asyncio.Event(); t = asyncio.create_task(mqtt_soak(600, stop))
    await c_sliders(p, effects=["Огонь", "Шум 3D", "Конфетти", "Метель"])
    stop.set(); await t; await p.mem("both", "after")

async def c_logspam(p):
    for i in range(60): pub(f"bogus{i}", i); await asyncio.sleep(0.05)
    await asyncio.sleep(2)
    for i in range(5): await p.mem("logspam", f"load{i}")

async def c_pair(p):
    # Тест pattern spam -> switch to Конфетти -> load -> speed spam: the sequence that crashed twice.
    a, b = os.environ.get("PAIR", "0,1").split(",")
    a, b = int(a), int(b)
    for r in range(int(os.environ.get("REPS", "4"))):
        await p.set("efcx", a); await asyncio.sleep(0.6); await p.load()
        ea, eb = EFF[a], EFF[b]
        k, (lo, hi) = next(iter(PARAMS[ea].items()))
        for i in range(12): await p.set(f"{ea}.{k}", lo + (i * 7) % (hi - lo + 1), timeout=0.25)
        await p.set("efcx", b); await asyncio.sleep(0.6); await p.load()
        k, (lo, hi) = next(iter(PARAMS[eb].items()))
        for i in range(12): await p.set(f"{eb}.{k}", lo + (i * 7) % (hi - lo + 1), timeout=0.25)
        await p.mem("pair", f"{a}->{b} r{r}")

async def c_notest(p):
    await c_sliders(p, effects=[e for e in EFF if e != "Тест"])

async def c_testonly(p):
    for r in range(6): await c_sliders(p, effects=["Тест"])

async def c_spamlong(p):
    # One effect, no switches: 4 params x 250 steps = 1000 sets.
    await c_sliders(p, steps=250, effects=["Огонь"])

async def c_konf(p):
    await c_sliders(p, effects=["Конфетти"])

async def c_one(p):
    await c_sliders(p, effects=[os.environ["EFFECT"]])

async def c_fastsliders(p):
    await c_sliders(p, delay=0.05, steps=60, effects=["Огонь", "Шум 3D", "Радуга"])

async def c_soak(p, minutes=30):
    stop = asyncio.Event(); t = asyncio.create_task(mqtt_soak(minutes * 60, stop))
    for i in range(minutes):
        for _ in range(25): await p.send("ping", 0, "", 2.0)
        await p.mem("soak", f"min{i+1}")
    stop.set(); await t

async def c_mem(p): await p.mem("mem", "probe")

async def main():
    cases = sys.argv[1:] or ["load", "sliders", "fastfx", "power", "twin", "mqttburst", "both", "logspam"]
    async with Panel() as p:
        await p.mem("start", "before")
        for c in cases:
            log(f"=== CASE {c}"); t0 = time.time()
            try: await globals()["c_" + c](p)
            except Exception as ex: log(f"!! case {c} failed: {ex!r}"); break
            log(f"=== {c} done in {time.time()-t0:.0f}s, rx {p.rx} bytes")
        await p.mem("end", "after")
    mq.loop_stop()

asyncio.run(main())
