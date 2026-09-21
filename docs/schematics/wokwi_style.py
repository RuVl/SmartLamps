"""Wokwi-like wiring pictures in plain SVG: boards with pin headers, parts that look
like the parts on the desk, straight coloured wires with rounded bends, white
background. No dependencies for the SVG; PNG review copy through cairosvg.

The drawing scripts (lamp-*.py) build the scene from these primitives. Coordinates
are pixels, y grows downwards.
"""

from __future__ import annotations

import html
import re
from pathlib import Path

RED, BLK, GRN, BLU, WHT, YEL = '#d9342b', '#222222', '#2f9e44', '#1c7ed6', '#f1f3f5', '#f5b301'
FONT = "font-family='Inter, Helvetica, Arial, sans-serif'"


class Scene:
    def __init__(self, width: int, height: int):
        self.w, self.h = width, height
        self.layers: dict[str, list[str]] = {'parts': [], 'wires': [], 'over': [], 'text': []}
        self.pins: dict[str, tuple[float, float]] = {}

    def add(self, svg: str, layer: str = 'parts') -> None:
        self.layers[layer].append(svg)

    def pin(self, name: str, x: float, y: float) -> tuple[float, float]:
        self.pins[name] = (x, y)
        return x, y

    # -- text -------------------------------------------------------------
    def text(self, x, y, s, size=13, weight='normal', color='#212529', anchor='start', layer='text'):
        self.add(f"<text x='{x:.1f}' y='{y:.1f}' font-size='{size}' font-weight='{weight}' "
                 f"fill='{color}' text-anchor='{anchor}' {FONT}>{html.escape(s)}</text>", layer)

    def note(self, x, y, lines, size=12, color='#495057'):
        for i, line in enumerate(lines):
            self.text(x, y + i * (size + 4), line, size=size, color=color)

    # -- wires ------------------------------------------------------------
    def wire(self, points, color, width=4, radius=12):
        """Polyline with rounded bends, drawn over a thin white halo so crossings read."""
        d = _rounded_path(points, radius)
        self.add(f"<path d='{d}' fill='none' stroke='white' stroke-width='{width + 3}' "
                 f"stroke-linecap='round' stroke-linejoin='round'/>", 'wires')
        self.add(f"<path d='{d}' fill='none' stroke='{color}' stroke-width='{width}' "
                 f"stroke-linecap='round' stroke-linejoin='round'/>", 'wires')

    def dot(self, x, y, color, r=4.5):
        self.add(f"<circle cx='{x:.1f}' cy='{y:.1f}' r='{r}' fill='{color}'/>", 'wires')

    # -- parts ------------------------------------------------------------
    def header(self, x, y, names, pitch, vertical, label_side, bold=(), prefix=''):
        """Row of square header pins with labels; registers pins as prefix+name."""
        for i, name in enumerate(names):
            px, py = (x, y + i * pitch) if vertical else (x + i * pitch, y)
            self.add(f"<rect x='{px - 6:.1f}' y='{py - 6:.1f}' width='12' height='12' rx='1.5' "
                     f"fill='#1e1e1e'/><rect x='{px - 3:.1f}' y='{py - 3:.1f}' width='6' height='6' "
                     f"fill='#e6c157'/>")
            self.pin(prefix + name, px, py)
            if not name:
                continue
            w = 'bold' if name in bold else 'normal'
            c = '#ffffff' if name in bold else '#dde3ea'
            if vertical:
                ax = px + 12 if label_side == 'right' else px - 12
                anchor = 'start' if label_side == 'right' else 'end'
                self.text(ax, py + 4, name, size=10, weight=w, color=c, anchor=anchor, layer='parts')
            else:
                ay = py + 20 if label_side == 'below' else py - 12
                self.text(px, ay, name, size=10, weight=w, color=c, anchor='middle', layer='parts')

    def pcb(self, x, y, w, h, color, rx=8, holes=()):
        self.add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='{rx}' fill='{color}' "
                 f"stroke='#0b1f33' stroke-opacity='.35'/>")
        for hx, hy in holes:
            self.add(f"<circle cx='{hx}' cy='{hy}' r='5' fill='white' stroke='#c9a227' stroke-width='3'/>")

    def chip(self, x, y, w, h, label='', color='#2b2b2b'):
        self.add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='2' fill='{color}'/>")
        if label:
            self.text(x + w / 2, y + h / 2 + 4, label, size=9, color='#bbb', anchor='middle', layer='parts')

    def resistor(self, p1, p2, label='R', bands=('#8d5524', '#111', '#8d5524', '#d4af37')):
        """Axial resistor with leads from p1 to p2 (any orientation)."""
        self._axial(p1, p2, 56, 18, '#e8d3a8', '#b9a27a', label,
                    [(9 + i * 11 + (6 if i == 3 else 0), c) for i, c in enumerate(bands)])

    def diode(self, anode, cathode, label='1N4007', label_side='right'):
        """Axial diode; the cathode band sits at the `cathode` end."""
        self._axial(anode, cathode, 46, 18, '#1e1e1e', '#1e1e1e', label, [(46 - 12, '#d0d0d0')], label_side)

    def _axial(self, p1, p2, bw, bh, fill, stroke, label, bands, label_side='right'):
        import math
        (x1, y1), (x2, y2) = p1, p2
        cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
        ang = math.degrees(math.atan2(y2 - y1, x2 - x1))
        self.wire([p1, p2], '#8a8f98', width=2.5)
        g = [f"<g transform='translate({cx:.1f},{cy:.1f}) rotate({ang:.1f})'>",
             f"<rect x='{-bw / 2}' y='{-bh / 2}' width='{bw}' height='{bh}' rx='{bh / 2 if fill != '#1e1e1e' else 4}' fill='{fill}' stroke='{stroke}'/>"]
        for off, c in bands:
            g.append(f"<rect x='{-bw / 2 + off}' y='{-bh / 2}' width='5' height='{bh}' fill='{c}'/>")
        g.append('</g>')
        self.add(''.join(g))
        # Label beside the body, on the side away from the wire direction.
        vertical = abs(x2 - x1) < abs(y2 - y1)
        if vertical and label_side == 'left':
            self.text(cx - bh / 2 - 6, cy + 4, label, size=11, anchor='end')
        elif vertical:
            self.text(cx + bh / 2 + 6, cy + 4, label, size=11)
        else:
            self.text(cx, cy - bh / 2 - 6, label, size=11, anchor='middle')

    def cap_electrolytic(self, x, y, h, label, label_side='right', label_at=None):
        """Vertical can drawn over the wires; leads out of the bottom: + at x-8, − at x+8."""
        w = 28
        self.add(f"<rect x='{x - w / 2}' y='{y}' width='{w}' height='{h}' rx='5' fill='#1f2d5c'/>", 'over')
        self.add(f"<rect x='{x + w / 2 - 7}' y='{y}' width='7' height='{h}' rx='3' fill='#cfd5e3'/>", 'over')
        self.add(f"<rect x='{x - w / 2}' y='{y}' width='{w}' height='6' rx='3' fill='#9aa5bd'/>", 'over')
        self.add(f"<text x='{x + w / 2 - 3.5}' y='{y + h / 2 + 4}' font-size='11' fill='#1f2d5c' text-anchor='middle' {FONT}>−</text>", 'over')
        if label_at:
            self.text(label_at[0], label_at[1], label, size=11)
        elif label_side == 'right':
            self.text(x + w / 2 + 6, y + h / 2 + 4, label, size=11)
        elif label_side == 'left':
            self.text(x - w / 2 - 6, y + h / 2 + 4, label, size=11, anchor='end')
        else:  # 'top': for a cap squeezed between the two wires of a pair
            self.text(x, y - 8, label, size=11, anchor='middle')
        return (x - 8, y + h), (x + 8, y + h)

    def cap_ceramic(self, x, y, label, lead_to=None, label_at=None):
        """Disc capacitor. lead_to=(xa, xb) draws horizontal leads to those x's."""
        if lead_to:
            self.wire([(lead_to[0], y), (lead_to[1], y)], '#8a8f98', width=2)
        self.add(f"<ellipse cx='{x}' cy='{y}' rx='13' ry='11' fill='#e8a33d' stroke='#b8752a'/>", 'over')
        if label_at:
            self.text(label_at[0], label_at[1], label, size=11)

    def label_box(self, x, y, w, h, title, sub='', fill='#fff3bf', stroke='#e0a800'):
        self.add(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='6' fill='{fill}' stroke='{stroke}'/>")
        self.text(x + 12, y + 22, title, size=13, weight='bold')
        if sub:
            self.text(x + 12, y + 40, sub, size=11, color='#495057')

    # -- output -----------------------------------------------------------
    def save(self, path: Path, title: str):
        body = ''.join(self.layers['parts'] + self.layers['wires'] + self.layers['over'] + self.layers['text'])
        svg = (f"<svg xmlns='http://www.w3.org/2000/svg' width='{self.w}' height='{self.h}' "
               f"viewBox='0 0 {self.w} {self.h}'><title>{html.escape(title)}</title>"
               f"<rect width='{self.w}' height='{self.h}' fill='white'/>{body}</svg>")
        path.write_text(svg)
        try:
            import cairosvg
            cairosvg.svg2png(bytestring=svg.encode(), write_to=str(path.with_suffix('.png')), scale=1.5)
        except ImportError:
            pass


def _rounded_path(points, r):
    pts = [(float(x), float(y)) for x, y in points]
    if len(pts) < 3 or r <= 0:
        return 'M ' + ' L '.join(f'{x:.1f},{y:.1f}' for x, y in pts)
    d = [f'M {pts[0][0]:.1f},{pts[0][1]:.1f}']
    for i in range(1, len(pts) - 1):
        (x0, y0), (x1, y1), (x2, y2) = pts[i - 1], pts[i], pts[i + 1]
        rr = min(r, _dist(x0, y0, x1, y1) / 2, _dist(x1, y1, x2, y2) / 2)
        ax, ay = _towards(x1, y1, x0, y0, rr)
        bx, by = _towards(x1, y1, x2, y2, rr)
        d.append(f'L {ax:.1f},{ay:.1f} Q {x1:.1f},{y1:.1f} {bx:.1f},{by:.1f}')
    d.append(f'L {pts[-1][0]:.1f},{pts[-1][1]:.1f}')
    return ' '.join(d)


def _dist(x0, y0, x1, y1):
    return ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5


def _towards(x, y, tx, ty, dist):
    L = _dist(x, y, tx, ty) or 1.0
    return x + (tx - x) / L * dist, y + (ty - y) / L * dist
