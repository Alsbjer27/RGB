"""Generate the RGB main-menu Settings background (1920x1080 transparent PNG).

Reuses the frame from generate_credits_image.py (keep both scripts in the same folder).
The setting rows themselves are drawn live by the C++ RGB Settings Page widget.
Usage:  python generate_settings_image.py [output_folder]
"""
import os
import sys
from PIL import ImageDraw

import generate_credits_image as frame_style

frame_style.ACCENT = (70, 235, 95)          # Settings = green, like the menu button
frame_style.PANEL = (360, 190, 1560, 890)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    img = frame_style.frame()
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = frame_style.PANEL
    cx = (x0 + x1) / 2
    title = frame_style.font("black", 84)
    tw = d.textlength("SETTINGS", font=title)
    d.text((cx - tw / 2, y0 + 34), "SETTINGS", font=title, fill=(255, 255, 255))
    d.rectangle([cx - 100, y0 + 132, cx + 100, y0 + 137], fill=frame_style.ACCENT)
    hint = frame_style.font("bold", 26)
    for text, dy in (("UP / DOWN  SELECT     LEFT / RIGHT  CHANGE", 112), ("PRESS  B / ESC  TO RETURN", 66)):
        hw = d.textlength(text, font=hint)
        d.text((cx - hw / 2, y1 - dy), text, font=hint, fill=frame_style.YELLOW if "RETURN" in text else (170, 175, 190))
    path = os.path.join(out, "T_MenuSettings.png")
    img.save(path)
    print(path)


if __name__ == "__main__":
    main()
