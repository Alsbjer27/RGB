"""Render the ability instruction panels shown by URGBHUDWidget.

Run from the repository root:  python Scripts/Tools/generate_ability_instruction_images.py
Writes PNG source images to Content/RGB/UI/HUD/AbilityInstructions/. Re-import them in the
editor after changing wording or bindings. Fonts come from the Windows font folder.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

OUTPUT_DIR = Path("Content/RGB/UI/HUD/AbilityInstructions")
FONT_DIR = Path("C:/Windows/Fonts")
SUPERSAMPLE = 2
WIDTH, HEIGHT = 960, 420

PANEL_FILL = (12, 14, 20, 230)
TEXT = (240, 242, 246, 255)
MUTED = (165, 170, 182, 255)
KEY_FILL = (52, 56, 68, 255)
KEY_EDGE = (120, 126, 140, 255)
PAD_COLORS = {"A": (96, 186, 70), "B": (214, 62, 52), "X": (52, 122, 220), "Y": (232, 190, 40)}
RGB_STRIPE = [(235, 50, 40), (60, 205, 80), (50, 110, 240)]


def font(name, size):
    return ImageFont.truetype(str(FONT_DIR / name), size * SUPERSAMPLE)


TITLE_FONT = font("segoeuib.ttf", 46)
BODY_FONT = font("segoeui.ttf", 28)
KEY_FONT = font("segoeuib.ttf", 22)
FOOTER_FONT = font("segoeui.ttf", 22)


def s(value):
    return value * SUPERSAMPLE


def segment_width(draw, segment, body_font):
    kind, label = segment[0], segment[1]
    if kind == "text":
        return draw.textlength(label, font=body_font)
    if kind == "pad" and label in PAD_COLORS:
        return s(40)
    return max(draw.textlength(label, font=KEY_FONT) + s(28), s(44))


def draw_line(draw, segments, center_x, top, body_font, color):
    gap = s(10)
    widths = [segment_width(draw, segment, body_font) for segment in segments]
    x = center_x - (sum(widths) + gap * (len(segments) - 1)) / 2
    line_height = s(44)
    middle = top + line_height / 2

    for segment, width in zip(segments, widths):
        kind, label = segment[0], segment[1]
        if kind == "text":
            draw.text((x, middle), label, font=body_font, fill=color, anchor="lm")
        elif kind == "pad" and label in PAD_COLORS:
            # Face buttons use their usual controller colours.
            radius = s(19)
            cx = x + width / 2
            draw.ellipse((cx - radius, middle - radius, cx + radius, middle + radius), fill=PAD_COLORS[label] + (255,))
            draw.text((cx, middle), label, font=KEY_FONT, fill=TEXT, anchor="mm")
        else:
            # Keyboard keys and shoulder buttons are drawn as keycaps.
            box = (x, middle - s(19), x + width, middle + s(19))
            draw.rounded_rectangle(box, radius=s(8), fill=KEY_FILL, outline=KEY_EDGE, width=s(2))
            draw.text((x + width / 2, middle), label, font=KEY_FONT, fill=TEXT, anchor="mm")
        x += width + gap
    return top + line_height


def render(file_name, title, lines, accent):
    image = Image.new("RGBA", (s(WIDTH), s(HEIGHT)), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    margin = s(6)
    panel = (margin, margin, s(WIDTH) - margin, s(HEIGHT) - margin)
    draw.rounded_rectangle(panel, radius=s(28), fill=PANEL_FILL, outline=(255, 255, 255, 40), width=s(2))

    # Accent stripe under the title: one colour for dash, red/green/blue for weapons.
    stripe_width = s(360)
    stripe_left = s(WIDTH) / 2 - stripe_width / 2
    stripe_top = s(104)
    for index, color in enumerate(accent):
        part = stripe_width / len(accent)
        draw.rounded_rectangle((stripe_left + part * index, stripe_top, stripe_left + part * (index + 1), stripe_top + s(5)), radius=s(2), fill=color + (255,))

    center_x = s(WIDTH) / 2
    draw.text((center_x, s(62)), title, font=TITLE_FONT, fill=TEXT, anchor="mm")

    # Centre the body block between the title stripe and the footer.
    block_height = len(lines) * s(44) + (len(lines) - 1) * s(10)
    top = (s(116) + s(HEIGHT - 92)) / 2 - block_height / 2
    for line in lines:
        top = draw_line(draw, line, center_x, top, BODY_FONT, TEXT) + s(10)

    footer = [("text", "Press"), ("pad", "A"), ("text", "or"), ("key", "Space"), ("text", "to continue")]
    draw_line(draw, footer, center_x, s(HEIGHT - 82), FOOTER_FONT, MUTED)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    image.resize((WIDTH, HEIGHT), Image.LANCZOS).save(OUTPUT_DIR / file_name)
    print(f"Wrote {OUTPUT_DIR / file_name}")


def main():
    render(
        "T_AbilityInstruction_Dash.png",
        "DASH UNLOCKED",
        [
            [("text", "Dash while in the air:"), ("key", "Left Shift"), ("text", "or"), ("pad", "B")],
            [("text", "You can dash once per jump.")],
        ],
        [(230, 235, 245)],
    )
    render(
        "T_AbilityInstruction_FirstWeapon.png",
        "WEAPON UNLOCKED",
        [
            [("text", "You can now shoot.")],
            [("text", "Fire:"), ("key", "P"), ("text", "or"), ("pad", "X")],
        ],
        RGB_STRIPE,
    )
    render(
        "T_AbilityInstruction_WeaponSwitching.png",
        "NEW WEAPON COLOUR",
        [
            [("text", "You now have two weapon colours.")],
            [("text", "Previous colour:"), ("key", "O"), ("text", "or"), ("key", "LB")],
            [("text", "Next colour:"), ("key", "\u00C5"), ("text", "or"), ("key", "RB")],
        ],
        RGB_STRIPE,
    )
    render(
        "T_AbilityInstruction_AdditionalWeapon.png",
        "NEW WEAPON COLOUR",
        [
            [("text", "Another colour has been added to your weapon.")],
            [("text", "Switch colours:"), ("key", "O"), ("text", "/"), ("key", "LB"), ("text", "and"), ("key", "\u00C5"), ("text", "/"), ("key", "RB")],
        ],
        RGB_STRIPE,
    )


if __name__ == "__main__":
    main()
