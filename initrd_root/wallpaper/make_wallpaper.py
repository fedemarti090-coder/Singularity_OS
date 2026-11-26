import argparse
import struct
from pathlib import Path
from PIL import Image


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
    parser = argparse.ArgumentParser(description="Generate uncompressed 24-bit BMP wallpaper")
    parser.add_argument("-i", "--input", type=str, default=None, help="Source image (optional)")
    parser.add_argument("-o", "--output", type=str, default="wallpaper.bmp", help="Output BMP path")
    parser.add_argument("-W", "--width", type=int, default=1024, help="Target width")
    parser.add_argument("-H", "--height", type=int, default=768, help="Target height")
    parser.add_argument("--top-down", action="store_true", help="Write negative biHeight top-down BMP")
    args = parser.parse_args()

    if args.input:
        img = Image.open(args.input).convert("RGB").resize((args.width, args.height), Image.Resampling.LANCZOS)
        rgb_bytes = img.tobytes()
    else:
        rgb_bytes = generate_gradient(args.width, args.height)

    bmp_bytes = build_bmp_24bit(args.width, args.height, rgb_bytes, top_down=args.top_down)
    out_path = Path(args.output)
    out_path.write_bytes(bmp_bytes)
    print(f"Wrote {out_path} ({len(bmp_bytes)} bytes, {args.width}x{args.height}x24)")


if __name__ == "__main__":
    main()