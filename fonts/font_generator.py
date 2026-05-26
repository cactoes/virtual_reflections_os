import sys
import os
from PIL import Image

COLS = 16
ROWS = 6
CHAR_W = 8
CHAR_H = 16

def import_image(img_path="font_grid_vga.png", out_path="font.c"):
    if not os.path.exists(img_path):
        print(f"error: image not found {img_path}")
        return

    img = Image.open(img_path).convert('RGB')

    c_code = "const u8 font8x16[95][16] = {\n"

    for char_idx in range(95):
        grid_x = char_idx % COLS
        grid_y = char_idx // COLS
    
        hex_vals = []
        for row_idx in range(CHAR_H):
            byte_val = 0
            for col_idx in range(CHAR_W):
                px_x = (grid_x * CHAR_W) + col_idx
                px_y = (grid_y * CHAR_H) + row_idx

                r, g, b = img.getpixel((px_x, px_y))

                if r > 127 or g > 127 or b > 127:
                    byte_val |= (0x80 >> col_idx)

            hex_vals.append(f"0x{byte_val:02X}")

        char_ascii = char_idx + 32
        char_repr = chr(char_ascii)

        if char_repr == '\\':
            comment = "\\\\"
        else:
            comment = char_repr

        c_code += "    { " + ", ".join(hex_vals) + f" }}, // {comment} ({char_ascii})\n"

    c_code += "};\n"

    with open(out_path, 'w') as f:
        f.write(c_code)

    print(f"generated font")

def print_help():
    print("py font_generator.py {filepath}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_help()
    else:
        import_image(sys.argv[1].lower())