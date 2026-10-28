"""Generate RGB main-menu option backgrounds (transparent PNG, no text).

Usage:  python generate_menu_buttons.py [output_folder]
Needs Pillow. Writes T_MenuButton_<Option>_Selected.png and _Dimmed.png for
Start (red), Settings (green), Credits (blue) and Exit (white).
"""
import math
import os
import sys
from PIL import Image, ImageChops, ImageDraw, ImageFilter

W, H = 1024, 256          # final canvas (2:... wide plate with room for glow)
SS = 3                    # supersampling factor for smooth edges
PAD_X, PAD_Y = 70, 52     # space around the plate for the outer glow
CHAMFER = 46              # cut-corner size
OPTIONS = {
    "Start": (255, 52, 52),
    "Settings": (70, 235, 95),
    "Credits": (80, 120, 255),
    "Exit": (235, 240, 255),
}
DIM = 0.38                # brightness of an unselected option


def s(v):
    return int(round(v * SS))


def plate_points(inset=0.0):
    x0, y0, x1, y1 = PAD_X + inset, PAD_Y + inset, W - PAD_X - inset, H - PAD_Y - inset
    c = max(CHAMFER - inset * 0.6, 4)
    # Cut top-left and bottom-right corners, small cuts on the others.
    small = c * 0.35
    return [(x0 + c, y0), (x1 - small, y0), (x1, y0 + small), (x1, y1 - c), (x1 - c, y1),
            (x0 + small, y1), (x0, y1 - small), (x0, y0 + c)]


def big(points):
    return [(s(x), s(y)) for x, y in points]


def layer():
    return Image.new("RGBA", (s(W), s(H)), (0, 0, 0, 0))


def scale(color, k):
    return tuple(min(255, int(c * k)) for c in color)


def hex_pattern(mask, color, alpha):
    """Faint hexagon grid inside the plate, echoing the platform hex panels."""
    img = layer()
    d = ImageDraw.Draw(img)
    r = s(16)
    w = math.sqrt(3) * r
    for row in range(-1, int(s(H) / (1.5 * r)) + 2):
        for col in range(-1, int(s(W) / w) + 2):
            cx = col * w + (w / 2 if row % 2 else 0)
            cy = row * 1.5 * r
            pts = [(cx + r * math.cos(math.radians(60 * k + 30)), cy + r * math.sin(math.radians(60 * k + 30))) for k in range(6)]
            d.polygon(pts, outline=color + (alpha,), width=max(1, SS))
    out = layer()
    out.paste(img, (0, 0), mask)
    return out


def make(color, selected):
    k = 1.0 if selected else DIM
    col = scale(color, k)
    glow_strength = 1.0 if selected else 0.35
    final = layer()

    plate = big(plate_points())
    inner = big(plate_points(10))
    mask = Image.new("L", final.size, 0)
    ImageDraw.Draw(mask).polygon(plate, fill=255)

    # 1. Outer neon glow.
    for width, blur, strength in ((14, 22, 1.0), (8, 7, 0.9)):   # wide soft halo + tight bright rim
        glow = layer()
        ImageDraw.Draw(glow).line(plate + [plate[0]], fill=col + (255,), width=s(width), joint="curve")
        glow = glow.filter(ImageFilter.GaussianBlur(s(blur)))
        glow.putalpha(glow.getchannel("A").point(lambda v, f=strength: min(255, int(v * 1.6 * f * glow_strength))))
        final = Image.alpha_composite(final, glow)

    # 2. Dark glass body with a vertical tint gradient.
    body = layer()
    bd = ImageDraw.Draw(body)
    top, bottom = PAD_Y, H - PAD_Y
    for y in range(s(top), s(bottom)):
        t = (y - s(top)) / max(1, s(bottom) - s(top))
        tint = 0.30 - 0.18 * t
        base = (8, 10, 16)
        c = tuple(int(b * (1 - tint) + cc * tint * (0.9 if selected else 0.55)) for b, cc in zip(base, color))
        bd.line([(0, y), (s(W), y)], fill=c + (int(225 if selected else 205),))
    clipped = layer()
    clipped.paste(body, (0, 0), mask)
    final = Image.alpha_composite(final, clipped)

    # 3. Hex texture and a soft top sheen.
    final = Image.alpha_composite(final, hex_pattern(mask, scale(color, 0.9 * k), 26 if selected else 14))
    sheen = layer()
    ImageDraw.Draw(sheen).polygon(big([(PAD_X + 40, PAD_Y + 6), (W - PAD_X - 20, PAD_Y + 6), (W - PAD_X - 60, PAD_Y + 46), (PAD_X + 10, PAD_Y + 46)]),
                                  fill=(255, 255, 255, 22 if selected else 8))
    sheen_clip = layer()
    sheen_clip.paste(sheen, (0, 0), mask)
    final = Image.alpha_composite(final, sheen_clip)

    # 4. Bright border, a thin inner line and a white-hot core on the main edge.
    d = ImageDraw.Draw(final)
    d.line(plate + [plate[0]], fill=col + (255,), width=s(5), joint="curve")
    d.line(inner + [inner[0]], fill=col + (110 if selected else 70,), width=s(1.6), joint="curve")
    if selected:
        hot = scale((255, 255, 255), 1.0)
        d.line(plate + [plate[0]], fill=hot + (150,), width=s(1.6), joint="curve")

    # 5. Corner brackets (white) on the cut corners.
    white = scale((240, 245, 255), k)
    x0, y0, x1, y1 = PAD_X - 14, PAD_Y - 14, W - PAD_X + 14, H - PAD_Y + 14
    c = CHAMFER + 18
    br = s(4)
    d.line(big([(x0, y0 + c + 40), (x0, y0 + c), (x0 + c, y0), (x0 + c + 70, y0)]), fill=white + (255,), width=br, joint="curve")
    d.line(big([(x1, y1 - c - 40), (x1, y1 - c), (x1 - c, y1), (x1 - c - 70, y1)]), fill=white + (255,), width=br, joint="curve")

    # 6. Yellow/black accents like the menu colour bar, plus side ticks.
    yellow = scale((255, 205, 40), k)
    for side in (0, 1):
        cx = PAD_X + 26 if side == 0 else W - PAD_X - 26
        cy = H / 2
        d.rectangle(big([(cx - 7, cy - 24), (cx + 7, cy + 24)]), fill=(10, 10, 12, 255))
        d.rectangle(big([(cx - 7, cy - 24), (cx + 7, cy - 17)]), fill=yellow + (255,))
        d.rectangle(big([(cx - 7, cy + 17), (cx + 7, cy + 24)]), fill=yellow + (255,))
    for i in range(6):
        tx = W - PAD_X - CHAMFER - 40 - i * 22
        d.line(big([(tx, PAD_Y + 12), (tx - 10, PAD_Y + 26)]), fill=col + (200 if selected else 120,), width=s(3))
    d.rectangle(big([(PAD_X + CHAMFER + 30, H - PAD_Y - 16), (PAD_X + CHAMFER + 150, H - PAD_Y - 11)]), fill=col + (230,))
    d.rectangle(big([(PAD_X + CHAMFER + 160, H - PAD_Y - 16), (PAD_X + CHAMFER + 180, H - PAD_Y - 11)]), fill=yellow + (230,))

    return final.resize((W, H), Image.LANCZOS)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    for name, color in OPTIONS.items():
        for selected in (True, False):
            img = make(color, selected)
            path = os.path.join(out, "T_MenuButton_%s_%s.png" % (name, "Selected" if selected else "Dimmed"))
            img.save(path)
            print(path)


if __name__ == "__main__":
    main()
