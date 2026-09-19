#!/usr/bin/env python3
"""Same slider load as matrix.py's `sliders`, but through MQTT only - no panel client.
uv run --quiet --with paho-mqtt mqtt_only.py [reps]"""
import os, sys, time
import paho.mqtt.client as mqtt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lampkeys import EFF, PARAMS
BROKER = os.environ.get("BROKER", "192.168.31.21"); NAME = os.environ.get("NAME", "Lamp8266")
n = {"state": 0}
mq = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mq.on_message = lambda c, u, m: n.__setitem__("state", n["state"] + 1)
mq.connect(BROKER, 1883, 30); mq.subscribe(f"lamp/{NAME}/state"); mq.loop_start()
def pub(cmd, v): mq.publish(f"lamp/{NAME}/cmd/{cmd}", str(v))
reps = int(sys.argv[1]) if len(sys.argv) > 1 else 1
sent = 0
for r in range(reps):
    for e in EFF:
        pub("effect", e); time.sleep(0.6); sent += 1
        for k, (lo, hi) in PARAMS[e].items():
            for i in range(40):
                pub(f"param/{k}", lo + (i * 7) % (hi - lo + 1)); sent += 1; time.sleep(0.25)
        print(time.strftime("%H:%M:%S"), f"rep {r} {e}: sent {sent}, state msgs {n['state']}", flush=True)
mq.loop_stop()
