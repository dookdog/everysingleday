#!/usr/bin/env python3
"""Draw Lily Leap's home-screen icon (a frog on a lily pad at dawn) and save the sizes iPhone and Android use.

    python3 make_icons.py      # needs Pillow (pip install pillow)
"""
import pathlib
from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
OUT = HERE / "icons"
N = 1024  # master size; drawn at 2x then reduced for smooth edges
S = N * 2


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def hexc(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


img = Image.new("RGB", (S, S))
d = ImageDraw.Draw(img)

# dawn sky into warm horizon
top, mid, low = hexc("6fbfe3"), hexc("bde5ef"), hexc("fde3c8")
horizon = int(S * 0.62)
for y in range(horizon):
    t = y / horizon
    d.line([(0, y), (S, y)], fill=lerp(top, mid, t / 0.6) if t < 0.6 else lerp(mid, low, (t - 0.6) / 0.4))
# sun glow
glow = Image.new("L", (S, S), 0)
ImageDraw.Draw(glow).ellipse([S * 0.62, S * 0.10, S * 0.98, S * 0.46], fill=170)
glow = glow.filter(ImageFilter.GaussianBlur(S * 0.05))
img.paste(Image.new("RGB", (S, S), hexc("ffeaa3")), (0, 0), glow)
d.ellipse([S * 0.73, S * 0.21, S * 0.87, S * 0.35], fill=hexc("ffeaa3"))
# hills
d.ellipse([-S * 0.3, S * 0.48, S * 0.7, S * 0.9], fill=hexc("a7d6b0"))
d.ellipse([S * 0.35, S * 0.52, S * 1.35, S * 0.95], fill=hexc("82c389"))
# water
water_top, water_bot = hexc("4aa6c6"), hexc("1f5d73")
for y in range(horizon, S):
    d.line([(0, y), (S, y)], fill=lerp(water_top, water_bot, (y - horizon) / (S - horizon)))
# lily pad with its notch
d.ellipse([S * 0.10, S * 0.70, S * 0.90, S * 0.92], fill=hexc("4f9d47"))
d.ellipse([S * 0.13, S * 0.705, S * 0.87, S * 0.895], fill=hexc("6dbb5a"))
d.polygon([(S * 0.5, S * 0.81), (S * 0.90, S * 0.79), (S * 0.88, S * 0.86)], fill=hexc("3f93b0"))

# frog
body, dark, belly = hexc("5dbb4c"), hexc("2f6e2c"), hexc("d6ef9a")
d.ellipse([S * 0.18, S * 0.42, S * 0.82, S * 0.84], fill=dark)                    # outline
d.ellipse([S * 0.195, S * 0.435, S * 0.805, S * 0.825], fill=body)
d.ellipse([S * 0.30, S * 0.60, S * 0.70, S * 0.82], fill=belly)
for cx in (0.33, 0.67):
    d.ellipse([S * (cx - 0.135), S * 0.25, S * (cx + 0.135), S * 0.52], fill=dark)
    d.ellipse([S * (cx - 0.12), S * 0.265, S * (cx + 0.12), S * 0.505], fill=body)
    d.ellipse([S * (cx - 0.085), S * 0.30, S * (cx + 0.085), S * 0.47], fill=hexc("fffdf5"))
    d.ellipse([S * (cx - 0.035), S * 0.34, S * (cx + 0.045), S * 0.43], fill=hexc("1b1d1a"))
    d.ellipse([S * (cx - 0.005), S * 0.355, S * (cx + 0.025), S * 0.385], fill=(255, 255, 255))
    d.ellipse([S * (cx - 0.07), S * 0.55, S * (cx + 0.01), S * 0.59], fill=hexc("f28db2"))
d.arc([S * 0.33, S * 0.44, S * 0.67, S * 0.62], start=20, end=160, fill=dark, width=int(S * 0.018))

# a fly buzzing at top left
fx, fy = S * 0.2, S * 0.17
d.ellipse([fx - S * 0.03, fy - S * 0.07, fx + S * 0.02, fy - S * 0.005], fill=(235, 248, 255))
d.ellipse([fx + S * 0.0, fy - S * 0.07, fx + S * 0.05, fy - S * 0.005], fill=(235, 248, 255))
d.ellipse([fx - S * 0.035, fy - S * 0.02, fx + S * 0.045, fy + S * 0.03], fill=hexc("2b2a33"))
d.ellipse([fx + S * 0.03, fy - S * 0.02, fx + S * 0.07, fy + S * 0.02], fill=hexc("1c1b22"))

master = img.resize((N, N), Image.LANCZOS)
OUT.mkdir(exist_ok=True)
for size in (1024, 512, 192, 180):
    master.resize((size, size), Image.LANCZOS).save(OUT / f"icon-{size}.png", optimize=True)
print("icons written to", OUT)
