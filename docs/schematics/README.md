# Схемы

Каждая картинка — скрипт на Python; SVG рядом с ним закоммичен, PNG — копия для
просмотра, в git не попадает. Править скрипт, не SVG.

| Файл | Что | Перерисовать |
|---|---|---|
| `lamp-b-wokwi.py` | лампа B как детали на столе, провода цветом по назначению | `uv run --with cairosvg python docs/schematics/lamp-b-wokwi.py` |
| `lamp-b-wiring.py` | лампа B принципиальной схемой (schemdraw) | `uv run --with schemdraw --with cairosvg python docs/schematics/lamp-b-wiring.py` |
| `wokwi_style.py` | примитивы для картинок первого типа: платы с гребёнками, матрица, детали, провода со скруглениями | — |

Цвета проводов: красный `+5 В`, чёрный `GND`, зелёный данные, синий кнопка.
