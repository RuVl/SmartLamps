# Lamp B (WeMos D1 mini) as a picture of the parts on the desk. Render:
#   uv run --with cairosvg python docs/schematics/lamp-b-wokwi.py
# Writes lamp-b-wokwi.svg (+ .png review copy) next to this file.

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from wokwi_style import BLK, BLU, GRN, RED, Scene  # noqa: E402

OUT = Path(__file__).with_suffix('.svg')
s = Scene(1700, 1060)
s.text(40, 46, 'Лампа B (ESP8266): подключение', size=24, weight='bold')
s.text(40, 72, 'Красный +5 В · чёрный GND · зелёный данные · синий кнопка', size=13, color='#495057')

# --- WeMos D1 mini, USB down ------------------------------------------------
BX, BY, BW, BH = 300, 380, 205, 274
s.pcb(BX, BY, BW, BH, '#1b4f9c', holes=[(BX + 14, BY + 14), (BX + BW - 14, BY + 14)])
s.add(f"<rect x='{BX + 48}' y='{BY + 18}' width='108' height='96' rx='4' fill='#c7ccd4' stroke='#8d949e'/>"
      f"<path d='M{BX + 56} {BY + 26} h18 v10 h-10 v10 h10 v10 h-10 v10 h10 v10 h-18' fill='none' stroke='#8d949e' stroke-width='2'/>"
      f"<text x='{BX + 104}' y='{BY + 70}' font-size='10' fill='#555' text-anchor='middle' font-family='sans-serif'>ESP-12F</text>")
s.chip(BX + 70, BY + 150, 44, 30, 'CH340')
s.chip(BX + 130, BY + 150, 28, 30, '')
s.add(f"<rect x='{BX + BW / 2 - 22}' y='{BY + BH - 14}' width='44' height='22' rx='4' fill='#b8bec7' stroke='#7d8590'/>")
s.text(BX + BW / 2, BY + 230, 'WeMos D1 mini', size=12, weight='bold', color='white', anchor='middle', layer='parts')
PITCH = 22
left = ['RST', 'A0', 'D0', 'D5', 'D6', 'D7', 'D8', '3V3']
right = ['TX', 'RX', 'D1', 'D2', 'D3', 'D4', 'GND', '5V']
s.header(BX + 14, BY + 26, left, PITCH, True, 'right', bold={'3V3'}, prefix='b.')
s.header(BX + BW - 14, BY + 26, right, PITCH, True, 'left', bold={'RX', 'D2', 'GND', '5V'}, prefix='b.')
P = s.pins

# --- touch pad, left of the board -------------------------------------------
TX, TY = 60, 420
s.pcb(TX, TY, 110, 84, '#c92a2a', rx=6)
s.add(f"<circle cx='{TX + 40}' cy='{TY + 42}' r='22' fill='#e03131' stroke='#a61e1e' stroke-width='3'/>"
      f"<rect x='{TX + 74}' y='{TY + 30}' width='20' height='16' rx='2' fill='#2b2b2b'/>")
s.text(TX + 55, TY + 76, 'ttp223', size=11, weight='bold', color='white', anchor='middle', layer='parts')
s.header(TX + 110, TY + 20, ['VCC', 'I/O', 'GND'], 25, True, 'left', bold={'VCC', 'I/O', 'GND'}, prefix='t.')

# --- level-shifting pixel ------------------------------------------------------
PX, PY = 700, 372   # breakout board 100x100, LED in the middle
s.pcb(PX, PY, 100, 100, '#f8f9fa', rx=6)
s.add(f"<rect x='{PX + 25}' y='{PY + 25}' width='50' height='50' rx='4' fill='#e9ecef' stroke='#adb5bd'/>"
      f"<circle cx='{PX + 50}' cy='{PY + 50}' r='16' fill='#fff3bf' stroke='#f59f00'/>"
      f"<circle cx='{PX + 50}' cy='{PY + 50}' r='4' fill='#495057'/>")
for name, (x, y), (lx, ly, anc) in [
        ('DI', (PX, PY + 50), (PX + 8, PY + 45, 'start')),
        ('DO', (PX + 100, PY + 50), (PX + 92, PY + 45, 'end')),
        ('VDD', (PX + 25, PY + 100), (PX + 25, PY + 92, 'middle')),
        ('GND', (PX + 75, PY + 100), (PX + 75, PY + 92, 'middle'))]:
    s.add(f"<rect x='{x - 6}' y='{y - 6}' width='12' height='12' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
    s.pin('p.' + name, x, y)
    s.text(lx, ly, name, size=9, weight='bold', color='#343a40', anchor=anc, layer='parts')
s.text(PX + 50, PY - 12, 'жертвенный WS2812B', size=12, weight='bold', anchor='middle')

# R1 between RX and DI: RX → right → down to the DI row → resistor → DI
rx = P['b.RX']
s.wire([rx, (rx[0] + 60, rx[1]), (rx[0] + 60, PY + 50), (560, PY + 50)], GRN)
s.resistor((560, PY + 50), (660, PY + 50), 'R1 100 Ω')
s.wire([(660, PY + 50), P['p.DI']], GRN)

# two 1N4007 below VDD, anode towards the supply
vdd, gnd = P['p.VDD'], P['p.GND']
s.diode((vdd[0], vdd[1] + 118), (vdd[0], vdd[1] + 28), 'D2 1N4007', label_side='left')
s.diode((vdd[0], vdd[1] + 218), (vdd[0], vdd[1] + 128), 'D1 1N4007', label_side='left')
s.wire([vdd, (vdd[0], vdd[1] + 28)], RED)
s.wire([(vdd[0], vdd[1] + 118), (vdd[0], vdd[1] + 128)], RED)
s.cap_ceramic((vdd[0] + gnd[0]) / 2, vdd[1] + 14, 'C4 100 nF', lead_to=(vdd[0], gnd[0]), label_at=(gnd[0] + 12, vdd[1] + 18))
s.dot(vdd[0], vdd[1] + 14, RED)
s.dot(gnd[0], vdd[1] + 14, BLK)

# --- matrix -------------------------------------------------------------------
MX, MY, CELL = 1060, 120, 30
s.pcb(MX, MY, 16 * CELL, 16 * CELL, '#111111', rx=4)
for r in range(16):
    for c in range(16):
        x, y = MX + c * CELL + 6, MY + r * CELL + 6
        s.add(f"<rect x='{x}' y='{y}' width='18' height='18' rx='2' fill='#f1f3f5'/>"
              f"<circle cx='{x + 9}' cy='{y + 9}' r='4' fill='#fff3bf'/>")
s.text(MX + 8 * CELL, MY - 12, 'Матрица WS2812B 16×16, 256 LED', size=12, weight='bold', anchor='middle')
# input pads on the left edge (DIN in line with the pixel's DO), power pads below
for name, y in [('DIN', PY + 50), ('GND', 470 + 30), ('5V', 500 + 30)]:
    s.add(f"<rect x='{MX - 8}' y='{y - 7}' width='16' height='14' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
    s.pin('m.' + name, MX - 8, y)
    s.text(MX - 18, y - 7, name, size=10, weight='bold', color='#343a40', anchor='end')
for name, y in [('GND', 500), ('5V', 530)]:
    s.add(f"<rect x='{MX + 16 * CELL - 8}' y='{y - 7}' width='16' height='14' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
    s.pin('m2.' + name, MX + 16 * CELL + 8, y)
    s.text(MX + 16 * CELL + 18, y - 7, name, size=10, weight='bold', color='#343a40')
s.text(MX - 18, 468, 'вход', size=10, color='#495057', anchor='end')
s.text(MX + 16 * CELL + 18, 468, 'дальний край', size=10, color='#495057')
s.wire([P['p.DO'], P['m.DIN']], GRN)

# --- power supply -----------------------------------------------------------------
SX, SY = 60, 800
s.add(f"<rect x='{SX}' y='{SY}' width='170' height='110' rx='10' fill='#222'/>"
      f"<rect x='{SX + 40}' y='{SY - 26}' width='14' height='26' fill='#777'/><rect x='{SX + 96}' y='{SY - 26}' width='14' height='26' fill='#777'/>"
      f"<text x='{SX + 85}' y='{SY + 50}' font-size='13' fill='#ddd' text-anchor='middle' font-family='sans-serif' font-weight='bold'>5 В / 4 А</text>"
      f"<text x='{SX + 85}' y='{SY + 70}' font-size='10' fill='#aaa' text-anchor='middle' font-family='sans-serif'>на холостом ходу 5,43 В</text>")
s.wire([(SX + 170, SY + 55), (SX + 210, SY + 55)], '#222', width=5)
# screw terminal block, 3 pairs: one pair of wires per load
TBX, TBY = SX + 210, SY - 10
s.add(f"<rect x='{TBX}' y='{TBY}' width='40' height='140' rx='4' fill='#2b8a3e' stroke='#1e6b30'/>")
rows = {}
for i, name in enumerate(['+', '−', '+', '−', '+', '−']):
    y = TBY + 14 + i * 22
    s.add(f"<circle cx='{TBX + 20}' cy='{y}' r='7' fill='#c9ced6' stroke='#6c757d'/>"
          f"<line x1='{TBX + 15}' y1='{y}' x2='{TBX + 25}' y2='{y}' stroke='#495057' stroke-width='2'/>")
    s.text(TBX - 6, y + 4, name, size=12, weight='bold', anchor='end', color=RED if name == '+' else BLK)
    rows[i] = (TBX + 40, y)
s.text(TBX + 20, TBY + 156, 'клеммы БП', size=10, color='#495057', anchor='middle')

# pair 1 → board (near channels x=560/590)
b5v, bgnd = P['b.5V'], P['b.GND']
s.wire([rows[0], (560, rows[0][1]), (560, b5v[1]), b5v], RED)
s.wire([rows[1], (600, rows[1][1]), (600, bgnd[1]), bgnd], BLK)
# pair 2 → pixel
s.wire([rows[2], (vdd[0], rows[2][1]), (vdd[0], vdd[1] + 218)], RED)
s.wire([rows[3], (gnd[0], rows[3][1]), gnd], BLK)
# pair 3 → matrix input, then on to the far end
m5v, mgnd, f5v, fgnd = P['m.5V'], P['m.GND'], P['m2.5V'], P['m2.GND']
s.wire([rows[4], (990, rows[4][1]), (990, m5v[1]), m5v], RED)
s.wire([rows[5], (950, rows[5][1]), (950, mgnd[1]), mgnd], BLK)
s.wire([(990, rows[4][1]), (MX + 16 * CELL + 60, rows[4][1]), (MX + 16 * CELL + 60, f5v[1]), f5v], RED)
s.wire([(950, rows[5][1]), (MX + 16 * CELL + 90, rows[5][1]), (MX + 16 * CELL + 90, fgnd[1]), fgnd], BLK)
s.dot(990, rows[4][1], RED)
s.dot(950, rows[5][1], BLK)

# electrolytics: C2 at the board's 5 V, C1 at the matrix input
# Electrolytics sit between the two wires of their pair: + lead left to the red wire,
# − lead right to the black one, so nothing crosses.
c2p, c2m = s.cap_electrolytic(580, 690, 60, 'C2 470 µF', label_at=(612, 724))
s.wire([c2p, (c2p[0], c2p[1] + 16), (560, c2p[1] + 16)], RED); s.dot(560, c2p[1] + 16, RED)
s.wire([c2m, (c2m[0], c2m[1] + 32), (600, c2m[1] + 32)], BLK); s.dot(600, c2m[1] + 32, BLK)
c1p, c1m = s.cap_electrolytic(970, 640, 90, 'C1 1000 µF', label_at=(1002, 689))
s.wire([c1p, (c1p[0], c1p[1] + 16), (990, c1p[1] + 16)], RED); s.dot(990, c1p[1] + 16, RED)
s.wire([c1m, (c1m[0], c1m[1] + 32), (950, c1m[1] + 32)], BLK); s.dot(950, c1m[1] + 32, BLK)

# --- touch pad wires ----------------------------------------------------------
tv, ti, tg = P['t.VCC'], P['t.I/O'], P['t.GND']
b3 = P['b.3V3']
s.wire([tv, (230, tv[1]), (230, b3[1]), b3], RED)
s.wire([ti, (200, ti[1]), (200, 340), (620, 340), (620, P['b.D2'][1]), P['b.D2']], BLU)
s.wire([tg, (260, tg[1]), (260, rows[1][1]), rows[1]], BLK)

# --- notes ------------------------------------------------------------------------
s.note(430, 960, [
    'Три отдельные пары проводов от клемм БП: к плате, к пикселю, к матрице. Земли сходятся только на клеммах.',
    'Провод не тоньше 0,5 мм², до метра. Питание в матрицу заведено с двух концов: с входа и с дальнего края.'])
s.note(1100, 660, [
    'Жертвенный пиксель: два 1N4007 оставляют ему около 4,2 В, порог DI падает до 2,9 В,',
    'и 3,2 В с GPIO3 читаются уверенно. DO перевыдаёт данные с размахом 4,2 В.',
    'В прошивке под него -DLED_LEAD_PIXELS=1: пиксель не часть картинки, всегда чёрный.'])
s.note(60, 560, ['Кнопка на лампе B пока', 'заглушена в прошивке', '(LAMP_BUTTON_DISABLED).'], size=11)

s.save(OUT, 'Лампа B: подключение')
