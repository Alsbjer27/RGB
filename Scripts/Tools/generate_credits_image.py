"""Generate the RGB main-menu credits panel (1920x1080 transparent PNG, text included).

Usage:  python generate_credits_image.py [output_folder]
Needs Pillow. Edit CREDITS below and run again to update the text.
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 1920, 1080
PANEL = (190, 70, 1730, 1010)        # left, top, right, bottom of the framed panel
ACCENT = (80, 120, 255)               # Credits = blue, like the menu button
YELLOW = (255, 205, 40)
SS = 2                                 # supersampling for smooth frame edges

# ("title", text) big heading, ("head", text) section heading, ("line", text) normal line,
# ("gap", pixels) vertical space, ("hint", text) bottom instruction.
CREDITS = [
    ("title", "CREDITS"),
    ("line", "RGB  -  a game by Emil Alsbjer"),
    ("gap", 26),
    ("head", "DESIGN, PROGRAMMING, LEVELS, ENEMY AI, MODELS, VFX, UI, MUSIC & SOUND"),
    ("line", "Emil Alsbjer"),
    ("gap", 14),
    ("head", "PLAYER ROBOT MODEL & ANIMATIONS"),
    ("line", "\"Stylized / Robot - Low Poly Game-Ready\" by CharlieCatling (Sketchfab), CC BY 4.0"),
    ("gap", 14),
    ("head", "HEAVY ROBOT ANIMATIONS"),
    ("line", "Mixamo, Adobe"),
    ("gap", 14),
    ("head", "ENVIRONMENT"),
    ("line", "\"Stylized Landscape and Grass\" by dm-mayu (Fab), CC BY 4.0"),
    ("line", "\"Grid material\" by Pixel Affect (Fab)"),
    ("gap", 14),
    ("head", "ADDITIONAL SOUND EFFECTS (FREESOUND.ORG)"),
    ("line", "suntemple  \u00b7  kenneth_cooney  \u00b7  heyheytheree  \u00b7  lilmati  \u00b7  djfroyd  \u00b7  neopolitansixth"),
    ("line", "cat-fox_alex  \u00b7  lostphosphene  \u00b7  vurca  \u00b7  khaotix523  \u00b7  valhallaproject"),
    ("gap", 18),
    ("small", "Made with Unreal Engine 5.7  \u00b7  Blender  \u00b7  FL Studio"),
    ("small", "TNM114 / TDDD23  \u00b7  Link\u00f6ping University  \u00b7  2026"),
    ("hint", "PRESS  B / ESC  TO RETURN"),
]

FONT_CANDIDATES = {
    "bold": ["C:/Windows/Fonts/segoeuib.ttf", "/usr/share/fonts/opentype/inter/Inter-Bold.otf", "DejaVuSans-Bold.ttf"],
    "black": ["C:/Windows/Fonts/seguibl.ttf", "/usr/share/fonts/opentype/inter/Inter-Black.otf", "DejaVuSans-Bold.ttf"],
    "regular": ["C:/Windows/Fonts/segoeui.ttf", "/usr/share/fonts/opentype/inter/Inter-Medium.otf", "DejaVuSans.ttf"],
}


def font(kind, size):
    for path in FONT_CANDIDATES[kind]:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    return ImageFont.load_default()


def frame():
    big = Image.new("RGBA", (W * SS, H * SS), (0, 0, 0, 0))
    x0, y0, x1, y1 = [v * SS for v in PANEL]
    c = 70 * SS
    small = 26 * SS
    pts = [(x0 + c, y0), (x1 - small, y0), (x1, y0 + small), (x1, y1 - c), (x1 - c, y1), (x0 + small, y1), (x0, y1 - small), (x0, y0 + c)]
    # Glow.
    glow = Image.new("RGBA", big.size, (0, 0, 0, 0))
    ImageDraw.Draw(glow).line(pts + [pts[0]], fill=ACCENT + (255,), width=16 * SS, joint="curve")
    glow = glow.filter(ImageFilter.GaussianBlur(22 * SS))
    glow.putalpha(glow.getchannel("A").point(lambda v: min(255, int(v * 1.4))))
    big = Image.alpha_composite(big, glow)
    # Dark body.
    d = ImageDraw.Draw(big)
    d.polygon(pts, fill=(8, 10, 18, 238))
    # Header band tint.
    band = Image.new("RGBA", big.size, (0, 0, 0, 0))
    ImageDraw.Draw(band).rectangle([x0, y0, x1, y0 + 215 * SS], fill=ACCENT + (34,))
    mask = Image.new("L", big.size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    clip = Image.new("RGBA", big.size, (0, 0, 0, 0))
    clip.paste(band, (0, 0), mask)
    big = Image.alpha_composite(big, clip)
    d = ImageDraw.Draw(big)
    d.line(pts + [pts[0]], fill=ACCENT + (255,), width=5 * SS, joint="curve")
    d.line(pts + [pts[0]], fill=(255, 255, 255, 140), width=2 * SS, joint="curve")
    # Corner brackets.
    w = 5 * SS
    e = 30 * SS
    d.line([(x0 - e, y0 + c + 120 * SS), (x0 - e, y0 + c), (x0 + c, y0 - e), (x0 + c + 200 * SS, y0 - e)], fill=(240, 245, 255, 255), width=w, joint="curve")
    d.line([(x1 + e, y1 - c - 120 * SS), (x1 + e, y1 - c), (x1 - c, y1 + e), (x1 - c - 200 * SS, y1 + e)], fill=(240, 245, 255, 255), width=w, joint="curve")
    # Yellow/black clips and RGB status lights.
    for cy in (y0 + (y1 - y0) // 2,):
        for cx in (x0 + 30 * SS, x1 - 30 * SS):
            d.rectangle([cx - 9 * SS, cy - 50 * SS, cx + 9 * SS, cy + 50 * SS], fill=(10, 10, 12, 255))
            d.rectangle([cx - 9 * SS, cy - 50 * SS, cx + 9 * SS, cy - 38 * SS], fill=YELLOW + (255,))
            d.rectangle([cx - 9 * SS, cy + 38 * SS, cx + 9 * SS, cy + 50 * SS], fill=YELLOW + (255,))
    for i, col in enumerate(((255, 52, 52), (70, 235, 95), (80, 120, 255))):
        bx = x1 - 150 * SS - i * 46 * SS
        d.rectangle([bx, y0 + 34 * SS, bx + 32 * SS, y0 + 42 * SS], fill=col + (255,))
    return big.resize((W, H), Image.LANCZOS)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    img = frame()
    d = ImageDraw.Draw(img)
    cx = (PANEL[0] + PANEL[2]) / 2
    styles = {
        "title": (font("black", 84), (255, 255, 255), 100),
        "head": (font("bold", 25), (130, 160, 255), 38),
        "line": (font("regular", 31), (235, 238, 245), 42),
        "small": (font("regular", 25), (160, 165, 180), 36),
    }
    y = PANEL[1] + 34
    for kind, value in CREDITS:
        if kind == "gap":
            y += value
            continue
        if kind == "hint":
            f = font("bold", 28)
            tw = d.textlength(value, font=f)
            hy = PANEL[3] - 58
            d.text((cx - tw / 2, hy), value, font=f, fill=YELLOW)
            continue
        f, col, step = styles[kind]
        tw = d.textlength(value, font=f)
        d.text((cx - tw / 2, y), value, font=f, fill=col)
        if kind == "title":
            d.rectangle([cx - 90, y + 98, cx + 90, y + 103], fill=ACCENT)
            step += 18
        y += step
    path = os.path.join(out, "T_MenuCredits.png")
    img.save(path)
    print(path)


if __name__ == "__main__":
    main()
