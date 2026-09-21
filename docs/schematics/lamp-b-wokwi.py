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

# --- level-shifting pixel: DIN in line with RX -----------------------------------
rx = P['b.RX']
PX, PY = 760, rx[1] - 18
s.ws2812b(PX, PY)
s.text(PX + 32, PY - 22, 'жертвенный WS2812B', size=12, weight='bold', anchor='middle')
din, gnd, vdd, dout = P['p.DIN'], P['p.GND'], P['p.VDD'], P['p.DOUT']

# R1 between RX and DIN
s.wire([rx, (580, rx[1])], GRN)
s.resistor((580, rx[1]), (680, rx[1]), 'R1 100 Ω')
s.wire([(680, rx[1]), din], GRN)

# VDD through one 1N4007 (anode towards the supply); the red wire goes right,
# crosses DOUT once and drops to the diode
s.wire([vdd, (vdd[0] + 60, vdd[1]), (vdd[0] + 60, vdd[1] + 60)], RED)
s.diode((vdd[0] + 60, vdd[1] + 150), (vdd[0] + 60, vdd[1] + 60), 'D1 1N4007')
GX = gnd[0] - 40          # the pixel's GND wire runs down here
s.wire([gnd, (GX, gnd[1])], BLK)

# --- matrix -------------------------------------------------------------------
MX, MY, CELL = 1060, 120, 30
s.pcb(MX, MY, 16 * CELL, 16 * CELL, '#111111', rx=4)
for r in range(16):
    for c in range(16):
        x, y = MX + c * CELL + 6, MY + r * CELL + 6
        s.add(f"<rect x='{x}' y='{y}' width='18' height='18' rx='2' fill='#f1f3f5'/>"
              f"<circle cx='{x + 9}' cy='{y + 9}' r='4' fill='#fff3bf'/>")
s.text(MX + 8 * CELL, MY - 12, 'Матрица WS2812B 16×16, 256 LED', size=12, weight='bold', anchor='middle')
# DIN pad on the left edge in line with DOUT; power pads in the middle of the bottom edge
s.add(f"<rect x='{MX - 8}' y='{dout[1] - 7}' width='16' height='14' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
s.pin('m.DIN', MX - 8, dout[1])
s.text(MX - 18, dout[1] - 7, 'DIN', size=10, weight='bold', color='#343a40', anchor='end')
MB = MY + 16 * CELL
for name, x in [('5V', MX + 8 * CELL - 35), ('GND', MX + 8 * CELL + 35)]:
    s.add(f"<rect x='{x - 7}' y='{MB - 8}' width='14' height='16' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
    s.pin('m.' + name, x, MB + 8)
    s.text(x + (-12 if name == '5V' else 12), MB + 22, name, size=10, weight='bold', color='#343a40',
           anchor='end' if name == '5V' else 'start')
s.text(MX + 8 * CELL, MB + 42, 'питание в центр матрицы', size=10, color='#495057', anchor='middle')
s.wire([dout, P['m.DIN']], GRN)

# --- power supply -----------------------------------------------------------------
SX, SY = 60, 800
s.add(f"<rect x='{SX}' y='{SY}' width='170' height='110' rx='10' fill='#222'/>"
      f"<rect x='{SX + 40}' y='{SY - 26}' width='14' height='26' fill='#777'/><rect x='{SX + 96}' y='{SY - 26}' width='14' height='26' fill='#777'/>"
      f"<text x='{SX + 85}' y='{SY + 50}' font-size='13' fill='#ddd' text-anchor='middle' font-family='sans-serif' font-weight='bold'>5 В / 4 А</text>"
      f"<text x='{SX + 85}' y='{SY + 70}' font-size='10' fill='#aaa' text-anchor='middle' font-family='sans-serif'>на холостом ходу 5,43 В</text>")
s.wire([(SX + 170, SY + 55), (SX + 210, SY + 55)], '#222', width=5)
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

# pair 1 → board; C2 lies between the two wires
b5v, bgnd = P['b.5V'], P['b.GND']
CH1, CH2 = 560, 630
s.wire([rows[0], (CH1, rows[0][1]), (CH1, b5v[1]), b5v], RED)
s.wire([rows[1], (CH2, rows[1][1]), (CH2, bgnd[1]), bgnd], BLK)
s.cap_electrolytic_h(CH1, CH2, 720, 'C2 470 µF')
s.dot(CH1, 720, RED); s.dot(CH2, 720, BLK)
# pair 2 → pixel; C4 lies between the two wires below the diode
DX = vdd[0] + 60
s.wire([rows[2], (DX, rows[2][1]), (DX, vdd[1] + 150)], RED)
s.wire([rows[3], (GX, rows[3][1]), (GX, gnd[1])], BLK)
CY = vdd[1] + 190
s.cap_ceramic((GX + DX) / 2, CY, 'C4 100 nF', lead_to=(GX, DX), label_at=((GX + DX) / 2 - 30, CY - 18))
s.dot(GX, CY, BLK); s.dot(DX, CY, RED)
# pair 3 → matrix centre; C1 lies between the two wires
m5v, mgnd = P['m.5V'], P['m.GND']
s.wire([rows[4], (m5v[0], rows[4][1]), m5v], RED)
s.wire([rows[5], (mgnd[0], rows[5][1]), mgnd], BLK)
s.cap_electrolytic_h(m5v[0], mgnd[0], 720, 'C1 1000 µF', label_dy=30)
s.dot(m5v[0], 720, RED); s.dot(mgnd[0], 720, BLK)

# --- touch pad wires ----------------------------------------------------------
tv, ti, tg = P['t.VCC'], P['t.I/O'], P['t.GND']
b3 = P['b.3V3']
s.wire([tv, (230, tv[1]), (230, b3[1]), b3], RED)
s.wire([ti, (200, ti[1]), (200, 340), (700, 340), (700, P['b.D2'][1]), P['b.D2']], BLU)
s.wire([tg, (260, tg[1]), (260, rows[1][1]), rows[1]], BLK)

# --- notes ------------------------------------------------------------------------
s.note(430, 960, [
    'Три отдельные пары проводов от клемм БП: к плате, к пикселю, к матрице. Земли сходятся только на клеммах.',
    'Провод не тоньше 0,5 мм², до метра. Питание матрицы заведено на её центральные площадки, не в угол.'])
s.note(1370, 800, [
    'Жертвенный пиксель: один 1N4007 в питании',
    'снижает его VDD примерно на 0,7 В, и 3,2 В',
    'с GPIO3 читаются уверенно. DOUT перевыдаёт',
    'данные с полным размахом. В прошивке под него',
    '-DLED_LEAD_PIXELS=1: пиксель не часть картинки,',
    'всегда чёрный.'])
s.note(60, 560, ['Кнопка на лампе B пока', 'заглушена в прошивке', '(LAMP_BUTTON_DISABLED).'], size=11)

s.save(OUT, 'Лампа B: подключение')
