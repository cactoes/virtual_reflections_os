import argparse
import re
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(
        description="Convert a PNG image into a C RGB24 array."
    )
    parser.add_argument("input", type=Path, help="Input PNG file")
    parser.add_argument("output", type=Path, help="Output C file")
    parser.add_argument("--name", default="splash", help="C symbol prefix")
    args = parser.parse_args()

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.name):
        parser.error("--name must be a valid C identifier")

    with Image.open(args.input) as image:
        rgba = image.convert("RGBA")
        black = Image.new("RGBA", rgba.size, (0, 0, 0, 255))
        rgb = Image.alpha_composite(black, rgba).convert("RGB")

    width, height = rgb.size
    pixels = rgb.tobytes()

    rows = []
    for offset in range(0, len(pixels), 12):
        values = ", ".join(f"0x{byte:02X}" for byte in pixels[offset:offset + 12])
        rows.append(f"    {values},")

    c_source = (
        '#include "libtype.h"\n\n'
        f"const u64 {args.name}_width = {width};\n"
        f"const u64 {args.name}_height = {height};\n\n"
        f"const u8 {args.name}_rgb[] = {{\n"
        + "\n".join(rows)
        + "\n};\n"
    )

    args.output.write_text(c_source, encoding="utf-8")
    print(f"Wrote {args.output}: {width}x{height}, RGB24")


if __name__ == "__main__":
    main()