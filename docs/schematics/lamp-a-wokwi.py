# Lamp A (ESP32-S3-DevKitC-1 N16R8) as a picture of the parts on the desk. This lamp is
# not assembled yet: the picture is the plan from docs/esp32s3.md, not a photo. Render:
#   uv run --with cairosvg python docs/schematics/lamp-a-wokwi.py

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from wokwi_style import BLK, BLU, GRN, RED, Scene  # noqa: E402

VIO = '#7048e8'  # I2S microphone lines
OUT = Path(__file__).with_suffix('.svg')
s = Scene(1700, 1060)
s.text(40, 46, 'Лампа A (ESP32-S3): подключение — план', size=24, weight='bold')
s.text(40, 72, 'Красный +5 В / 3V3 · чёрный GND · зелёный данные · синий кнопка · фиолетовый I2S микрофона',
       size=13, color='#495057')

# --- ESP32-S3-DevKitC-1, USB down ----------------------------------------------
BX, BY, BW, BH = 340, 200, 205, 520
s.pcb(BX, BY, BW, BH, '#1f2933', holes=[(BX + 14, BY + BH - 14), (BX + BW - 14, BY + BH - 14)])
# WROOM-1 module with the antenna at the top edge
s.add(f"<rect x='{BX + 40}' y='{BY + 10}' width='125' height='150' rx='4' fill='#c7ccd4' stroke='#8d949e'/>"
      f"<rect x='{BX + 40}' y='{BY + 10}' width='125' height='34' rx='4' fill='#e9ecef' stroke='#8d949e'/>"
      f"<path d='M{BX + 60} {BY + 18} h16 v8 h-8 v8 h8 v8 h-16' fill='none' stroke='#8d949e' stroke-width='2'/>"
      f"<text x='{BX + 102}' y='{BY + 104}' font-size='10' fill='#555' text-anchor='middle' font-family='sans-serif'>ESP32-S3-WROOM-1</text>"
      f"<text x='{BX + 102}' y='{BY + 118}' font-size='9' fill='#777' text-anchor='middle' font-family='sans-serif'>N16R8</text>")
s.chip(BX + 60, BY + 380, 40, 40, 'CP2102')
s.chip(BX + 115, BY + 385, 30, 30, '')
s.add(f"<rect x='{BX + 70}' y='{BY + 330}' width='14' height='14' rx='3' fill='#ffd43b'/>"
      f"<text x='{BX + 92}' y='{BY + 341}' font-size='8' fill='#ccc' font-family='sans-serif'>RGB</text>")
for ux in (BX + 48, BX + 118):
    s.add(f"<rect x='{ux}' y='{BY + BH - 10}' width='40' height='18' rx='5' fill='#b8bec7' stroke='#7d8590'/>")
s.text(BX + 68, BY + BH + 24, 'UART', size=8, color='#495057', anchor='middle')
s.text(BX + 138, BY + BH + 24, 'USB', size=8, color='#495057', anchor='middle')
s.text(BX + BW / 2, BY + 300, 'ESP32-S3-DevKitC-1', size=11, weight='bold', color='white', anchor='middle', layer='parts')
PITCH = 20.5
left = ['3V3', '3V3', 'RST', '4', '5', '6', '7', '15', '16', '17', '18', '8', '3', '46', '9', '10', '11', '12', '13', '14', '5V', 'GND']
right = ['GND', 'TX', 'RX', '1', '2', '42', '41', '40', '39', '38', '37', '36', '35', '0', '45', '48', '47', '21', '20', '19', 'GND', 'GND']
s.header(BX + 14, BY + 40, left, PITCH, True, 'right', bold={'3V3', '4', '5', '6', '7', '15', '5V', 'GND'}, prefix='l.')
s.header(BX + BW - 14, BY + 40, right, PITCH, True, 'left', bold={'GND'}, prefix='r.')
P = s.pins
# duplicate names: header() keeps the last one, so recover the others by position
y = lambda i: BY + 40 + i * PITCH  # noqa: E731
L = BX + 14
p3v3a, p3v3b = (L, y(0)), (L, y(1))
p4, p5, p6, p7, p15 = (L, y(3)), (L, y(4)), (L, y(5)), (L, y(6)), (L, y(7))
p5v, pgnd_l = (L, y(20)), (L, y(21))
R = BX + BW - 14
pgnd_rt, pgnd_rb = (R, y(0)), (R, y(21))

# --- touch pad, top left ---------------------------------------------------------
TX, TY = 60, 214
s.pcb(TX, TY, 110, 84, '#c92a2a', rx=6)
s.add(f"<circle cx='{TX + 40}' cy='{TY + 42}' r='22' fill='#e03131' stroke='#a61e1e' stroke-width='3'/>"
      f"<rect x='{TX + 74}' y='{TY + 30}' width='20' height='16' rx='2' fill='#2b2b2b'/>")
s.text(TX + 55, TY + 76, 'ttp223', size=11, weight='bold', color='white', anchor='middle', layer='parts')
s.header(TX + 110, y(0), ['GND', 'VCC', 'I/O'], PITCH, True, 'left', bold={'GND', 'VCC', 'I/O'}, prefix='t.')
tg, tv, ti = P['t.GND'], P['t.VCC'], P['t.I/O']
s.wire([tv, p3v3b], RED)                                        # straight into the second 3V3
s.wire([ti, (290, ti[1]), (290, p5[1]), p5], BLU)               # GPIO5
s.wire([tg, (230, tg[1]), (230, 110), (600, 110), (600, pgnd_rt[1]), pgnd_rt], BLK)  # over the top to GND

# --- I2S microphone, below the pad ---------------------------------------------
MX0, MY0 = 60, 400
s.pcb(MX0, MY0, 110, 130, '#1864ab', rx=6)
s.add(f"<circle cx='{MX0 + 40}' cy='{MY0 + 50}' r='14' fill='#343a40' stroke='#868e96' stroke-width='2'/>"
      f"<circle cx='{MX0 + 40}' cy='{MY0 + 50}' r='4' fill='#111'/>")
s.text(MX0 + 8, MY0 + 16, 'ICS-43434', size=11, weight='bold', color='white', layer='parts')
s.text(MX0 + 8, MY0 + 122, 'I2S · 65 dBA', size=9, color='#dde3ea', layer='parts')
s.header(MX0 + 110, MY0 + 20, ['VDD', 'SCK', 'WS', 'SD', 'GND'], 20, True, 'left',
         bold={'VDD', 'SCK', 'WS', 'SD', 'GND'}, prefix='m.')
mv, msck, mws, msd, mg = P['m.VDD'], P['m.SCK'], P['m.WS'], P['m.SD'], P['m.GND']
s.wire([mv, (270, mv[1]), (270, tv[1])], RED); s.dot(270, tv[1], RED)   # 3V3 tapped from the pad's wire
s.wire([msck, (302, msck[1]), (302, p6[1]), p6], VIO)
s.wire([mws, (316, mws[1]), (316, p7[1]), p7], VIO)
s.wire([msd, (330, msd[1]), (330, p15[1]), p15], VIO)
s.text(240, p6[1] - 6, 'I2S0: SCK 6 · WS 7 · SD 15', size=9, color='#495057', anchor='end')

# --- data: GPIO4 over the top, R1 in the drop, into the pixel ------------------------
DXW = 700
s.wire([p4, (336, p4[1]), (336, 150), (DXW, 150), (DXW, 200)], GRN)
s.resistor((DXW, 200), (DXW, 300), 'R1 100 Ω')
PX, PY = 760, 320
s.ws2812b(PX, PY)
s.text(PX + 32, PY - 22, 'жертвенный WS2812B', size=12, weight='bold', anchor='middle')
din, gnd, vdd, dout = P['p.DIN'], P['p.GND'], P['p.VDD'], P['p.DOUT']
s.wire([(DXW, 300), (DXW, din[1]), din], GRN)
DX, GX = vdd[0] + 60, gnd[0] - 40
s.wire([vdd, (DX, vdd[1]), (DX, vdd[1] + 80)], RED)
s.diode((DX, vdd[1] + 170), (DX, vdd[1] + 80), 'D1 1N4007')
s.wire([gnd, (GX, gnd[1])], BLK)
CY = vdd[1] + 62
s.cap_ceramic((GX + DX) / 2, CY, 'C4 100 nF', lead_to=(GX, DX), label_at=((GX + DX) / 2 - 30, CY + 28))
s.dot(GX, CY, BLK); s.dot(DX, CY, RED)

# --- matrix -------------------------------------------------------------------
MX, MY, CELL = 1060, 120, 30
s.pcb(MX, MY, 16 * CELL, 16 * CELL, '#111111', rx=4)
for r in range(16):
    for c in range(16):
        x, yy = MX + c * CELL + 6, MY + r * CELL + 6
        s.add(f"<rect x='{x}' y='{yy}' width='18' height='18' rx='2' fill='#f1f3f5'/>"
              f"<circle cx='{x + 9}' cy='{yy + 9}' r='4' fill='#fff3bf'/>")
s.text(MX + 8 * CELL, MY - 12, 'Матрица WS2812B 16×16, 256 LED', size=12, weight='bold', anchor='middle')
s.add(f"<rect x='{MX - 8}' y='{dout[1] - 7}' width='16' height='14' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
s.pin('mx.DIN', MX - 8, dout[1])
s.text(MX - 18, dout[1] - 7, 'DIN', size=10, weight='bold', color='#343a40', anchor='end')
MB = MY + 16 * CELL
for name, x in [('5V', MX + 8 * CELL - 35), ('GND', MX + 8 * CELL + 35)]:
    s.add(f"<rect x='{x - 7}' y='{MB - 8}' width='14' height='16' rx='2' fill='#e6c157' stroke='#8a6d1f'/>")
    s.pin('mx.' + name, x, MB + 8)
    s.text(x + (-12 if name == '5V' else 12), MB + 22, name, size=10, weight='bold', color='#343a40',
           anchor='end' if name == '5V' else 'start')
s.text(MX + 8 * CELL, MB + 42, 'питание в центр матрицы', size=10, color='#495057', anchor='middle')
s.wire([dout, P['mx.DIN']], GRN)

# --- power supply, terminal block with a pair per load -----------------------------
SX, SY = 40, 800
s.add(f"<rect x='{SX}' y='{SY}' width='170' height='110' rx='10' fill='#222'/>"
      f"<rect x='{SX + 40}' y='{SY - 26}' width='14' height='26' fill='#777'/><rect x='{SX + 96}' y='{SY - 26}' width='14' height='26' fill='#777'/>"
      f"<text x='{SX + 85}' y='{SY + 50}' font-size='13' fill='#ddd' text-anchor='middle' font-family='sans-serif' font-weight='bold'>5 В / 4 А</text>"
      f"<text x='{SX + 85}' y='{SY + 70}' font-size='10' fill='#aaa' text-anchor='middle' font-family='sans-serif'>на холостом ходу 5,43 В</text>")
s.wire([(SX + 170, SY + 55), (SX + 210, SY + 55)], '#222', width=5)
TBX, TBY = SX + 210, SY - 10
s.add(f"<rect x='{TBX}' y='{TBY}' width='40' height='140' rx='4' fill='#2b8a3e' stroke='#1e6b30'/>")
rows = {}
for i, name in enumerate(['+', '−', '−', '+', '+', '−']):
    yy = TBY + 14 + i * 22
    s.add(f"<circle cx='{TBX + 20}' cy='{yy}' r='7' fill='#c9ced6' stroke='#6c757d'/>"
          f"<line x1='{TBX + 15}' y1='{yy}' x2='{TBX + 25}' y2='{yy}' stroke='#495057' stroke-width='2'/>")
    s.text(TBX - 6, yy + 4, name, size=12, weight='bold', anchor='end', color=RED if name == '+' else BLK)
    rows[i] = (TBX + 40, yy)
s.text(TBX + 20, TBY + 156, 'клеммы БП', size=10, color='#495057', anchor='middle')

# pair 1 → board: + into 5V on the left header, − into GND on the right header
CH_R, CH_B = 312, 580
s.wire([rows[0], (CH_R, rows[0][1]), (CH_R, p5v[1]), p5v], RED)
s.wire([rows[1], (CH_B, rows[1][1]), (CH_B, pgnd_rb[1]), pgnd_rb], BLK)
s.wire([mg, (220, mg[1]), (220, 750), (CH_B, 750)], BLK); s.dot(CH_B, 750, BLK)   # microphone GND
s.cap_electrolytic_h(CH_R, CH_B, 780, 'C2 470 µF', label_dy=30)
s.dot(CH_R, 780, RED); s.dot(CH_B, 780, BLK)
# pair 2 → pixel (− first: the GND wire is the nearer one)
s.wire([rows[2], (GX, rows[2][1]), (GX, gnd[1])], BLK)
s.wire([rows[3], (DX, rows[3][1]), (DX, vdd[1] + 170)], RED)
# pair 3 → matrix centre; C1 between the two wires
m5v, mgnd = P['mx.5V'], P['mx.GND']
s.wire([rows[4], (m5v[0], rows[4][1]), m5v], RED)
s.wire([rows[5], (mgnd[0], rows[5][1]), mgnd], BLK)
s.cap_electrolytic_h(m5v[0], mgnd[0], 700, 'C1 1000 µF', label_dy=30)
s.dot(m5v[0], 700, RED); s.dot(mgnd[0], 700, BLK)

# --- notes ------------------------------------------------------------------------
s.note(620, 960, [
    'Три отдельные пары проводов от клемм БП: к плате, к пикселю, к матрице. Земли сходятся только на клеммах.',
    'Провод не тоньше 0,5 мм², до метра. Питание матрицы — на её центральные площадки, не в угол.'])
s.note(1370, 800, [
    'Лампа A ещё не собрана: это план из docs/esp32s3.md.',
    'Жертвенный пиксель — тот же узел, что на лампе B;',
    'драйвер RMT пока не читает LED_LEAD_PIXELS.',
    'Микрофон только здесь: ESP8266 не может — его',
    'единственный I2S занят лентой.'])
s.note(60, 580, ['GPIO33–37 заняты octal PSRAM,', 'GPIO19/20 — USB, GPIO0/3/45/46 — strapping:', 'на гребёнку не выводить.'], size=11)

s.save(OUT, 'Лампа A: подключение')
