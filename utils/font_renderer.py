from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
]

W, H = 8, 16
NUM_GLYPHS = 128
THRESHOLD = 128


def load_font() -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for path in FONT_CANDIDATES:
        if Path(path).exists():
            return ImageFont.truetype(path, 13)
    return ImageFont.load_default()


def render_glyph(font, code: int) -> list[int]:
    if code < 32 or code == 127:
        return [0] * H

    ch = chr(code)
    img = Image.new("L", (W, H), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, fill=255, font=font)

    rows: list[int] = []
    for y in range(H):
        mask = 0
        for x in range(W):
            if img.getpixel((x, y)) >= THRESHOLD:
                mask |= (1 << (7 - x))
        rows.append(mask)
    return rows


def label_for(code: int) -> str:
    if code < 32:
        return f"CTRL_{code}"
    if code == 127:
        return "DEL"
    return repr(chr(code))


def main() -> None:
    font = load_font()
    print(f"inline constexpr uint8_t kGlyphs[NUM_GLYPHS][GLYPH_H] = {{")
    for code in range(NUM_GLYPHS):
        rows = render_glyph(font, code)
        hex_list = ", ".join(f"0x{b:02X}" for b in rows)
        print(f"    // [{code}] {label_for(code)}")
        print(f"    {{{hex_list}}},")
    print("};")


if __name__ == "__main__":
    main()