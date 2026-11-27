import struct
from pathlib import Path
from PIL import Image

# --- CONFIGURAZIONE DA VARIABILI ---
INPUT_PATH: str | None = "image-2.png"  # Percorso immagine sorgente (oppure None per gradiente)
OUTPUT_PATH: str = "wallpaper.bmp"    # Percorso file BMP di output
TARGET_WIDTH: int = 1600              # Larghezza desiderata in output (px)
TARGET_HEIGHT: int = 900              # Altezza in output (None = proporzionale all'originale)
TOP_DOWN: bool = False                # True per BMP top-down (biHeight negativo)
# -----------------------------------


def build_bmp_24bit(width: int, height: int, rgb_bytes: bytes, top_down: bool = False) -> bytes:
    row_raw_size = width * 3
    row_stride = ((row_raw_size + 3) // 4) * 4
    pad_len = row_stride - row_raw_size
    padding = b"\x00" * pad_len

    pixel_data = bytearray(row_stride * height)
    rows = range(height) if top_down else range(height - 1, -1, -1)

    out_offset = 0
    for y in rows:
        src_row_start = y * row_raw_size
        for x in range(width):
            p = src_row_start + x * 3
            r = rgb_bytes[p]
            g = rgb_bytes[p + 1]
            b = rgb_bytes[p + 2]
            pixel_data[out_offset] = b
            pixel_data[out_offset + 1] = g
            pixel_data[out_offset + 2] = r
            out_offset += 3
        if pad_len:
            pixel_data[out_offset:out_offset + pad_len] = padding
            out_offset += pad_len

    header_size = 14 + 40
    file_size = header_size + len(pixel_data)
    signed_height = -height if top_down else height

    bmp_file_header = struct.pack(
        "<2sIHHI",
        b"BM",
        file_size,
        0,
        0,
        header_size,
    )
    bmp_info_header = struct.pack(
        "<IiiHHIIiiII",
        40,
        width,
        signed_height,
        1,
        24,
        0,
        len(pixel_data),
        2835,
        2835,
        0,
        0,
    )
    return bmp_file_header + bmp_info_header + bytes(pixel_data)


def generate_gradient(width: int, height: int) -> bytes:
    buf = bytearray(width * height * 3)
    idx = 0
    for y in range(height):
        g = int((y / max(1, height - 1)) * 255)
        for x in range(width):
            r = int((x / max(1, width - 1)) * 255)
            b = 140
            buf[idx] = r
            buf[idx + 1] = g
            buf[idx + 2] = b
            idx += 3
    return bytes(buf)


def main() -> None:
    width = TARGET_WIDTH

    if INPUT_PATH:
        img = Image.open(INPUT_PATH).convert("RGB")
        if TARGET_HEIGHT is not None:
            height = TARGET_HEIGHT
        else:
            height = max(1, round(img.height * (width / img.width)))

        img = img.resize((width, height), Image.Resampling.LANCZOS)
        rgb_bytes = img.tobytes()
    else:
        height = TARGET_HEIGHT if TARGET_HEIGHT is not None else 768
        rgb_bytes = generate_gradient(width, height)

    bmp_bytes = build_bmp_24bit(width, height, rgb_bytes, top_down=TOP_DOWN)
    out_path = Path(OUTPUT_PATH)
    out_path.write_bytes(bmp_bytes)
    print(f"Wrote {out_path} ({len(bmp_bytes)} bytes, {width}x{height}x24)")


if __name__ == "__main__":
    main()