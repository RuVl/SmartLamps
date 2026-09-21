# Post-build heap budget for the lamp (env:d1_mini and everything that extends it).
#
# The ESP8266 has 80 KB of DRAM and no MMU. What the linker reports as "RAM" is
# only the static part; the panel, WiFi and MQTT take the rest at run time, and
# a build that links fine can still be one page load away from a crash. This
# script adds the run-time model to the exact static numbers from the ELF and
# refuses the build when the predicted margin is gone. The model and every
# measured constant are explained in docs/memory.md - update the
# constants there and here together when the firmware changes shape.
#
# Three kinds of numbers, marked in the output:
#   exact    - read from the ELF or the build flags
#   source   - derived from library or project source (formula, not a guess)
#   measured - taken on the hardware on the date given; re-measure after any
#              change to the network stack, the panel layout or the log size
Import("env")

import glob
import os
import re
import subprocess

DRAM_END = 0x3FFFC000  # end of the 80 KB user DRAM; the sys stack lives above

# --- measured on the lamp, 2026-09-19, env d1_mini_mem, WS client + MQTT connected ---
# Heap taken at idle by everything that is not static: WiFi + lwIP (AP+STA,
# DHCP server), the open WebSocket, the MQTT session, LittleFS, GyverDB,
# NeoPixelBus buffers and the Logger. Measured as heap0 - ESP.getFreeHeap() at
# idle with the panel open and the broker connected, log full.
IDLE_TAKEN = 19_600
# Largest transient seen on top of idle: a full page build while the previous
# pushLog() packet was still queued (minfree from the mem: telemetry).
TRANSIENT_PEAK_MEASURED = None  # not measured yet on the 1-Log firmware; model below is used

# --- source: sizes fixed by the libraries ---
PACKET_SIZE = 1024       # sets::SettingsBase::_packet_size - page chunk before ws.binary()
WS_MAX_QUEUED = 8        # ESPAsyncWebServer-esphome WS_MAX_QUEUED_MESSAGES on ESP8266
TCP_SND_BUF = 2 * 536    # lwip2-536-feat: TCP_MSS 536, one pbuf copy per send
HTTP_BURST = 6 * 1024    # 3-4 parallel HTTP connections while the browser loads the page
MQTT_JSON = 2_400        # ArduinoJson pool + String for discovery + state, back to back
PAGE_FIXED = 1_470       # header + WiFi/MQTT/System menus without the log (calibrated: page 4790 B
                         # with 3 x 1024 log and the Тест effect on 2026-09-19)

FAIL_FREE = 4 * 1024     # below this lwIP has no pbufs and WiFi falls over
WARN_FREE = 8 * 1024     # margin for fragmentation: pushLog needs one contiguous block


def B(s):
    return len(s.encode("utf-8"))


def defines(env):
    out = {}
    for d in env.get("CPPDEFINES", []):
        if isinstance(d, (tuple, list)):
            out[d[0]] = d[1]
        else:
            out[d] = None
    return out


def symbol(env, elf, name):
    nm = env.subst("$CC").replace("-gcc", "-nm")  # xtensa-lx106-elf-nm next to the compiler
    out = subprocess.check_output([nm, elf]).decode()
    m = re.search(r"^([0-9a-f]+) . %s$" % re.escape(name), out, re.M)
    if not m:
        raise RuntimeError("symbol %s not in %s" % (name, elf))
    return int(m.group(1), 16)


def effects(root):
    """name -> [(label, select options or '')] from src/effects, the same way the panel builds them."""
    found = {}
    for path in sorted(glob.glob(os.path.join(root, "src", "effects", "*.cpp"))):
        src = open(path, encoding="utf-8").read()
        name = re.search(r'REGISTER_EFFECT\(\w+,\s*"([^"]+)"', src)
        if not name:
            continue
        params = []
        for m in re.finditer(r'core::Param\s+\w+\{\*this,\s*"[^"]+",\s*"([^"]+)",(.*?)\};', src, re.S):
            label, rest = m.group(1), m.group(2)
            opts = re.search(r'"([^"]+)"', rest).group(1) if "Select" in rest else ""
            params.append((label, opts))
        found[name.group(1)] = params
    return found


def page_model(root):
    net_cpp = open(os.path.join(root, "src", "net", "Net.cpp"), encoding="utf-8").read()
    log_bytes = int(re.search(r"kLogBytes\s*=\s*(\d+)", net_cpp).group(1))
    topic_bytes = int(re.search(r"kTopicLogBytes\s*=\s*(\d+)", net_cpp).group(1))
    webui = open(os.path.join(root, "src", "net", "WebUi.cpp"), encoding="utf-8").read()
    webui = re.sub(r"//[^\n]*", "", webui)  # comments mention b.Log() too
    # Every Log widget carries its whole buffer, in the page and in each push.
    log_widgets = len(re.findall(r"\bb\.Log\(", webui))
    log_total = (len(re.findall(r"\bb\.Log\(\w+,\s*log\(\)", webui)) * log_bytes
                 + len(re.findall(r"\bb\.Log\(\w+,\s*(?:wifi|mqtt)Log\(\)", webui)) * topic_bytes)
    push_log_total = (len(re.findall(r"\.update\(\w+,\s*log\(\)\)", webui)) * log_bytes
                      + len(re.findall(r"\.update\(\w+,\s*(?:wifi|mqtt)Log\(\)\)", webui)) * topic_bytes)
    fx = effects(root)
    options = B(";".join(fx))  # the effect Select
    per_effect = {}
    for name, params in fx.items():
        widgets = sum(48 + B(label) + B(opts) for label, opts in params)
        per_effect[name] = PAGE_FIXED + options + 120 + widgets + log_total + log_widgets * 16
    worst = max(per_effect.items(), key=lambda kv: kv[1])
    # pushLog: journal + 2 status strings + 2 LEDs, then a copy in the WS queue
    push_packet = 24 + push_log_total + 3 * 16 + 2 * 80 + 2 * 12
    return log_bytes, log_widgets, per_effect, worst, push_packet


def neopixel_bytes(defs):
    w = int(defs.get("MATRIX_WIDTH", 16))
    h = int(defs.get("MATRIX_HEIGHT", 16))
    lead = int(defs.get("LED_LEAD_PIXELS", 0) or 0)
    steps = 4 if "NPB_CONF_4STEP_CADENCE" in defs else 3
    px = w * h + lead
    data = px * 3                       # NeoPixelBus's own copy of the pixels
    i2s = -(-(px * 3 * steps) // 4) * 4  # DMA bit stream, rounded to 4
    idle = 320                          # reset/idle block
    desc = 12 * 32                      # DMA descriptors
    return data + i2s + idle + desc, steps


def check(source, target, env):
    elf = str(target[0])
    root = env.subst("$PROJECT_DIR")
    defs = defines(env)

    heap0 = DRAM_END - symbol(env, elf, "_heap_start")
    neo, steps = neopixel_bytes(defs)
    log_bytes, log_widgets, per_effect, (worst_name, page), push_packet = page_model(root)
    chunks = -(-page // PACKET_SIZE)

    # A page build inside the WebSocket callback: the packet buffer (grows past
    # PACKET_SIZE by up to one Log widget), a malloc'd copy of every chunk in
    # the client's queue and one pbuf copy of the first chunk in flight.
    build_peak = (PACKET_SIZE + 128 + log_bytes) + page + TCP_SND_BUF + 300
    push_peak = push_packet * 2 + TCP_SND_BUF
    # The HTTP burst (browser fetching the page) precedes the WebSocket and does
    # not overlap a build; a build can overlap a queued pushLog and, from loop(),
    # the MQTT state JSON that a slider change triggers.
    transient = TRANSIENT_PEAK_MEASURED or (max(HTTP_BURST, build_peak + push_peak) + MQTT_JSON)
    free_idle = heap0 - IDLE_TAKEN
    free_worst = free_idle - transient

    kind = "measured" if TRANSIENT_PEAK_MEASURED else "model"
    print("")
    print("ESP8266 heap budget (docs/memory.md)")
    print("  heap at boot            %6d  exact   DRAM_END - _heap_start" % heap0)
    print("  idle, everything up    -%6d  measured 2026-09-19 (includes NeoPixelBus %d B, %d-step)" % (IDLE_TAKEN, neo, steps))
    print("  = free at idle          %6d" % free_idle)
    print("  page build peak        -%6d  model   page %d B (%s, %d Log x %d) = %d chunks of %d" % (
        build_peak, page, worst_name, log_widgets, log_bytes, chunks, PACKET_SIZE))
    print("  pushLog peak           -%6d  model   packet %d B + WS copy + pbuf" % (push_peak, push_packet))
    print("  HTTP burst              %6d  model   (does not overlap a build; the larger of the two counts)" % HTTP_BURST)
    print("  MQTT JSON              -%6d  model" % MQTT_JSON)
    print("  = worst-case free       %6d  %s   (fail < %d, warn < %d)" % (free_worst, kind, FAIL_FREE, WARN_FREE))

    failed = False
    if chunks > WS_MAX_QUEUED:
        print("  !! the page needs %d WebSocket chunks, ESPAsyncWebServer queues %d: the rest is dropped and the panel comes up broken" % (chunks, WS_MAX_QUEUED))
        failed = True
    if free_worst < FAIL_FREE:
        print("  !! predicted worst-case free heap %d B < %d B" % (free_worst, FAIL_FREE))
        failed = True
    elif free_worst < WARN_FREE:
        print("  ! predicted worst-case free heap %d B < %d B: little room for fragmentation" % (free_worst, WARN_FREE))
    if failed:
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check)
