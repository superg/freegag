from io import BytesIO
from pathlib import Path
import struct

from PIL import Image
import resvg_py


ROOT = Path(__file__).resolve().parents[2]
ASSET_DIRECTORY = ROOT / "assets" / "tabler"
OUTPUT_DIRECTORY = ROOT / "src" / "extra"

WIDTH = 48
HEIGHT = 48
PALETTE_ENTRIES = 256
PALETTE_OFFSET = 14 + 40
PIXEL_OFFSET = PALETTE_OFFSET + PALETTE_ENTRIES * 4
PIXEL_SIZE = WIDTH * HEIGHT
BITMAP_SIZE = PIXEL_OFFSET + PIXEL_SIZE

ICONS = (
    ("square-arrow-left.svg", "sl_left_bmp", "synthesize_sl_left_bmp", (0xD0, 0xD0, 0xD0)),
    ("square-arrow-right.svg", "sl_right_bmp", "synthesize_sl_right_bmp", (0xD0, 0xD0, 0xD0)),
    ("square-check.svg", "sl_load_bmp", "synthesize_sl_load_bmp", (0x43, 0xA0, 0x47)),
    ("square-check.svg", "sl_save_bmp", "synthesize_sl_save_bmp", (0xE5, 0x39, 0x35)),
)


def render_alpha(source_name):
    source = (ASSET_DIRECTORY / source_name).read_text(encoding="utf-8")
    source = source.replace('width="24"', f'width="{WIDTH}"', 1)
    source = source.replace('height="24"', f'height="{HEIGHT}"', 1)
    source = source.replace('stroke="currentColor"', 'stroke="#ffffff"', 1)
    png = resvg_py.svg_to_bytes(svg_string=source)
    image = Image.open(BytesIO(png)).convert("RGBA")
    if image.size != (WIDTH, HEIGHT):
        raise RuntimeError(f"unexpected rendered size for {source_name}: {image.size}")
    return image.getchannel("A")


def create_bitmap(source_name, color):
    alpha = render_alpha(source_name)
    palette = bytearray()
    for index in range(PALETTE_ENTRIES):
        blue = color[2] * index // 255
        green = color[1] * index // 255
        red = color[0] * index // 255
        palette.extend((blue, green, red, 0))

    pixels = bytearray()
    for y in range(HEIGHT - 1, -1, -1):
        for x in range(WIDTH):
            pixels.append(alpha.getpixel((x, y)))

    file_header = struct.pack("<HIHHI", 0x4D42, BITMAP_SIZE, 0, 0, PIXEL_OFFSET)
    info_header = struct.pack("<IiiHHIIiiII", 40, WIDTH, HEIGHT, 1, 8, 0, PIXEL_SIZE, 2835, 2835, PALETTE_ENTRIES, PALETTE_ENTRIES)
    bitmap = file_header + info_header + palette + pixels
    if len(bitmap) != BITMAP_SIZE:
        raise RuntimeError(f"unexpected bitmap size: {len(bitmap)}")
    return bitmap


def format_bytes(data):
    lines = []
    for offset in range(0, len(data), 16):
        values = ", ".join(f"0x{value:02x}" for value in data[offset : offset + 16])
        lines.append(f"    {values},")
    return "\n".join(lines)


def create_source(symbol, function, bitmap):
    return f'''#include "{symbol}.h"
#include <cstring>
#include "runtime_services.h"



namespace freegag
{{

constexpr uint8_t {symbol}_data[]{{
    // clang-format off
{format_bytes(bitmap)}
    // clang-format on
}};

std::pair<void *, uint32_t> {function}(RuntimeHeap *heap)
{{
    constexpr uint32_t size = sizeof({symbol}_data);
    void *data = allocate_runtime_heap(heap, 0, size);
    if(data == nullptr)
        return {{}};
    std::memcpy(data, {symbol}_data, size);
    return {{ data, size }};
}}

}}
'''


def main():
    for source_name, symbol, function, color in ICONS:
        bitmap = create_bitmap(source_name, color)
        source = create_source(symbol, function, bitmap)
        (OUTPUT_DIRECTORY / f"{symbol}.cpp").write_text(source, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
