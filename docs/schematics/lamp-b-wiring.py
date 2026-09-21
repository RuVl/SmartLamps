# Lamp B (WeMos D1 mini) wiring. Render:
#   uv run --with schemdraw --with cairosvg python docs/schematics/lamp-b-wiring.py
# Writes lamp-b-wiring.svg and .png next to this file.

from pathlib import Path

import schemdraw
import schemdraw.elements as elm

RED, BLK, GRN, BLU = '#c92a2a', '#212529', '#2b8a3e', '#1971c2'
OUT = Path(__file__).with_suffix('')

# schemdraw's own SVG backend, not matplotlib's: smaller file, real text, and an
# explicit white background - GitHub's dark theme otherwise shows black on black.
schemdraw.use('svg')
schemdraw.config(bgcolor='white')

with schemdraw.Drawing(show=False) as d:
    d.config(unit=2, fontsize=11, font='sans-serif', lw=1.6)

    # --- controller -------------------------------------------------------
    mcu = d.add(elm.Ic(
        pins=[
            elm.IcPin(name='3V3', side='top', slot='1/3'),
            elm.IcPin(name='D2', pin='GPIO4', side='top', slot='2/3'),
            elm.IcPin(name='GND', side='top', slot='3/3', anchorname='GNDt'),
            elm.IcPin(name='RX', pin='GPIO3', side='right'),
            elm.IcPin(name='5V', side='bottom', slot='1/2'),
            elm.IcPin(name='GND', side='bottom', slot='2/2', anchorname='GNDb'),
        ],
        size=(4.5, 3.5), pinspacing=1.5, leadlen=0.6,
        label='WeMos D1 mini\n(ESP8266)'))

    # --- touch pad above the controller ------------------------------------
    btn = d.add(elm.Ic(
        pins=[
            elm.IcPin(name='VCC', side='bottom', slot='1/3'),
            elm.IcPin(name='I/O', side='bottom', slot='2/3', anchorname='IO'),
            elm.IcPin(name='GND', side='bottom', slot='3/3'),
        ],
        size=(4.5, 1.6), pinspacing=1.5, leadlen=0.6,
        label='ttp223 (сенсорная кнопка)')
        .anchor('VCC').at((mcu['3V3'].x, mcu['3V3'].y + 2.6)))
    d.add(elm.Line().at(btn.VCC).to(mcu['3V3']).color(RED))
    d.add(elm.Line().at(btn.IO).to(mcu.D2).color(BLU))
    d.add(elm.Line().at(btn.GND).to(mcu.GNDt).color(BLK))

    # --- sacrificial pixel ------------------------------------------------
    d.add(elm.Line().at(mcu.RX).right().length(1.2).color(GRN))
    r1 = d.add(elm.Resistor().right().length(2.5).label('R1\n100 Ω').color(GRN))
    pix = d.add(elm.Ic(
        pins=[
            elm.IcPin(name='DI', side='left'),
            elm.IcPin(name='DO', side='right'),
            elm.IcPin(name='VDD', side='bottom', slot='1/2'),
            elm.IcPin(name='GND', side='bottom', slot='2/2'),
        ],
        size=(4.6, 2.4), pinspacing=1.6, leadlen=0.6,
        label='жертвенный\nWS2812B')
        .anchor('DI').at(r1.end))
    d.add(elm.Line().at(r1.end).right().length(0.01).color(GRN))

    # VDD through one 1N4007: anode at +5 V (bottom), cathode at the pixel.
    d1 = d.add(elm.Diode().at(pix.VDD).down().reverse().length(2.2).label('D1\n1N4007', loc='left').color(RED))
    d.add(elm.Dot().at(pix.VDD).color(RED))
    c4 = d.add(elm.Capacitor().at(pix.VDD).right().tox(pix.GND).label('C4\n100 nF', loc='bottom'))
    d.add(elm.Dot().at(pix.GND).color(BLK))

    # --- matrix ------------------------------------------------------------
    mtx = d.add(elm.Ic(
        pins=[
            elm.IcPin(name='DIN', side='left'),
            elm.IcPin(name='+5V', side='bottom', slot='1/2', anchorname='V1'),
            elm.IcPin(name='GND', side='bottom', slot='2/2', anchorname='G1'),
        ],
        size=(8, 5), pinspacing=1.9, leadlen=0.6,
        label='Матрица WS2812B 16×16\n256 LED')
        .anchor('DIN').at((pix.DO.x + 2.5, pix.DO.y)))
    d.add(elm.Line().at(pix.DO).to(mtx.DIN).color(GRN))
    c1 = d.add(elm.Capacitor2().at((mtx.V1.x, mtx.V1.y - 1.1)).right().tox(mtx.G1)
               .label('C1\n1000 µF', loc='bottom'))
    d.add(elm.Dot().at(c1.start).color(RED))
    d.add(elm.Dot().at(c1.end).color(BLK))

    # --- power rails --------------------------------------------------------
    rail_v = d1.end.y - 1.4
    rail_g = rail_v - 1.2
    psu = d.add(elm.Ic(
        pins=[elm.IcPin(name='+', side='top', slot='1/2', anchorname='P'),
              elm.IcPin(name='−', side='top', slot='2/2', anchorname='M')],
        size=(4.5, 2.6), pinspacing=1.5, leadlen=0.6,
        label='Блок питания\n5 В / 4 А')
        .right().anchor('P').at((mcu['5V'].x, rail_g - 1.4)))

    # +5 V: PSU + → rail → D1 5V, diode, matrix
    d.add(elm.Line().at(psu.P).up().toy(rail_v).color(RED))
    d.add(elm.Dot().at((psu.P.x, rail_v)).color(RED))
    d.add(elm.Line().at((psu.P.x, rail_v)).right().tox(mtx.V1).color(RED))
    for x in (mcu['5V'].x, d1.end.x, mtx.V1.x):
        d.add(elm.Dot().at((x, rail_v)).color(RED))
    d.add(elm.Line().at(mcu['5V']).down().toy(rail_v).color(RED))
    d.add(elm.Line().at(d1.end).down().toy(rail_v).color(RED))
    d.add(elm.Line().at(mtx.V1).down().toy(rail_v).color(RED))

    # GND: PSU − → rail → all grounds
    d.add(elm.Line().at(psu.M).up().toy(rail_g).color(BLK))
    d.add(elm.Dot().at((psu.M.x, rail_g)).color(BLK))
    d.add(elm.Line().at((psu.M.x, rail_g)).right().tox(mtx.G1).color(BLK))
    for x in (mcu.GNDb.x, pix.GND.x, mtx.G1.x):
        d.add(elm.Dot().at((x, rail_g)).color(BLK))
    d.add(elm.Line().at(mcu.GNDb).down().toy(rail_g).color(BLK))
    d.add(elm.Line().at(pix.GND).down().toy(rail_g).color(BLK))
    d.add(elm.Line().at(mtx.G1).down().toy(rail_g).color(BLK))

    # C2 at the board's 5 V, C3 at 3V3 (shown as a note, sits on the module)
    c2 = d.add(elm.Capacitor2().at((mcu['5V'].x, mcu['5V'].y - 0.9)).right().length(1.5)
               .label('C2\n470 µF', loc='bottom'))
    d.add(elm.Line().at(c2.end).down().toy(rail_g).color(BLK))
    d.add(elm.Dot().at((c2.end.x, rail_g)).color(BLK))
    d.add(elm.Dot().at(c2.start).color(RED))

    d.add(elm.Label().at((psu.M.x + 1.0, rail_g - 1.9)).label(
        'Три отдельные пары проводов от клемм БП: к матрице, к плате, к пикселю.\n'
        'Земли сходятся только на клеммах. Провод не тоньше 0,5 мм², до метра.\n'
        'Питание матрицы — на её центральные площадки, не в угол.',
        fontsize=9, halign='left'))
    d.add(elm.Label().at((mcu['3V3'].x - 0.5, btn.VCC.y + 3.0)).label(
        'Лампа B (ESP8266): схема подключения', fontsize=14, halign='left'))

    d.save(str(OUT.with_suffix('.svg')))

# A real white <rect> behind the drawing: viewers that drop the root style attribute
# (GitHub's image proxy among them) would otherwise show the dark theme through.
import re  # noqa: E402

svg_path = OUT.with_suffix('.svg')
svg = svg_path.read_text()
vb = re.search(r'viewBox="([^"]+)"', svg).group(1).split()
rect = f'<rect x="{vb[0]}" y="{vb[1]}" width="{vb[2]}" height="{vb[3]}" fill="white"/>'
svg = re.sub(r'(<svg[^>]*>)', lambda m: m.group(1) + rect, svg, count=1)
svg_path.write_text(svg)

import cairosvg  # noqa: E402  - PNG is a review copy, rendered from the SVG
cairosvg.svg2png(url=str(OUT.with_suffix('.svg')), write_to=str(OUT.with_suffix('.png')), scale=2)
