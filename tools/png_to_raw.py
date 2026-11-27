import argparse
import struct
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(
        description="Convert a PNG into a simple RGB binary format."
    )
    parser.add_argument("input", type=Path, help="Input PNG file")
    parser.add_argument("output", type=Path, help="Output binary file")
    args = parser.parse_args()

    with Image.open(args.input) as image:
        rgba = image.convert("RGBA")
        black = Image.new("RGBA", rgba.size, (0, 0, 0, 255))
        rgb = Image.alpha_composite(black, rgba).convert("RGB")

    width, height = rgb.size
    pixels = rgb.tobytes()

    with open(args.output, "wb") as f:
        # Write width and height as little-endian u64s
        f.write(struct.pack("<QQ", width, height))

        # Write raw RGB24 pixel data
        f.write(pixels)

    print(f"Wrote {args.output}: {width}x{height}, RGB24")


if __name__ == "__main__":
    main()