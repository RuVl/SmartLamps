import sys
from pathlib import Path

try:
    Import("env")  # type: ignore (PlatformIO SConscript)
except Exception:
    print("Not running under PlatformIO")
    sys.exit(1)

html = Path("src/sim/index.html").read_text("utf-8")

# Into the source tree (gitignored), not the build dir, so the IDE always
# indexes it and `pio run -t clean` cannot take it away. Quoted #include
# from src/sim/ finds it via the including file's own directory, no -I needed.
out = Path("src/sim/page.h")
out.write_text(
    '// Generated from src/sim/index.html by src/sim/page.py — do not edit.\n'
    '#pragma once\n'
    'const char kPage[] = R"HTML(\n' + html + '\n)HTML";\n',
    "utf-8"
)
print(f"sim page: {out} ({len(html)} chars)")
