"""What every bench script needs to address the lamp: the panel's hashes, the
lamp's own keys and the effect catalogue with parameter ranges.

Hashes mirror the firmware - su() is su::hash from StringUtils (Settings
widget ids, actions), fnv() is hal::paramKey from src/hal/Storage.h - and
the effect list is the registry order the panel's Select uses."""


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


APP = {"lamp": 0x6C616D70, "brgt": 0x62726774, "efcx": 0x65666378}  # src/app/Keys.h


def key_id(k: str) -> int:
    """'lamp' | 'brgt' | 'efcx' -> app key; 'Effect.param' -> paramKey; else a db key by name."""
    if k in APP:
        return APP[k]
    if "." in k:
        return fnv(*k.split(".", 1))
    return su(k)


EFF = ["Тест", "Конфетти", "Снегопад", "Радуга", "Пейнтбол", "Шум 3D", "Матрица", "Светлячки",
       "Огонь", "Блуждающий кубик", "Смена цвета", "Метель"]

PARAMS = {  # key -> (min, max); Switch/Select spammed as 0..n
    "Смена цвета": {"speed": (1, 100), "saturation": (0, 100)},
    "Огонь": {"speed": (1, 100), "cooling": (10, 100), "spark": (10, 200), "hue": (0, 60)},
    "Светлячки": {"speed": (1, 100), "count": (1, 12), "trace": (0, 1)},
    "Матрица": {"speed": (1, 100), "density": (1, 100)},
    "Тест": {"pattern": (0, 3)},
    "Пейнтбол": {"speed": (1, 100), "blur": (1, 100)},
    "Метель": {"speed": (1, 100), "density": (1, 100), "tail": (0, 100)},
    "Шум 3D": {"speed": (1, 100), "scale": (1, 100), "palette": (0, 3)},
    "Снегопад": {"speed": (1, 100), "density": (1, 100)},
    "Конфетти": {"speed": (1, 100), "density": (1, 30), "fade": (5, 100)},
    "Радуга": {"speed": (1, 100), "scale": (1, 100), "dir": (0, 1)},
    "Блуждающий кубик": {"speed": (1, 100), "size": (1, 5)},
}

# What a browser fetches when the panel is opened; all served from PROGMEM.
STATIC = ["/", "/script.js", "/style.css", "/favicon.svg"]
