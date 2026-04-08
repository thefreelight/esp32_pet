from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path("/Users/jordan/Projects/esp32_pet")
DEMO = ROOT / "assets" / "pet_icons" / "demo"
CUSTOM = ROOT / "assets" / "pet_icons" / "custom"
OUT = ROOT / "src" / "PetSpriteAssets.h"

SPRITE_SIZE = 24
FRAME_COUNT = 4


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def to_canvas(im: Image.Image, max_w: int = SPRITE_SIZE, max_h: int = SPRITE_SIZE) -> Image.Image:
    im = im.convert("RGBA")
    bbox = im.getchannel("A").getbbox()
    if bbox:
        im = im.crop(bbox)
    im.thumbnail((max_w, max_h), Image.Resampling.BOX)
    canvas = Image.new("RGBA", (SPRITE_SIZE, SPRITE_SIZE), (0, 0, 0, 0))
    canvas.paste(im, ((SPRITE_SIZE - im.width) // 2, (SPRITE_SIZE - im.height) // 2), im)
    return canvas


def load_png(path: Path) -> Image.Image:
    return Image.open(path).convert("RGBA")


def blink_frame(base: Image.Image, left_eye, right_eye, width=3):
    out = base.copy()
    d = ImageDraw.Draw(out)
    d.line((left_eye[0], left_eye[1], left_eye[0] + width, left_eye[1]), fill=(24, 20, 26, 255), width=1)
    d.line((right_eye[0], right_eye[1], right_eye[0] + width, right_eye[1]), fill=(24, 20, 26, 255), width=1)
    return out


def shift_frame(base: Image.Image, dx=0, dy=0):
    out = base.copy()
    canvas = Image.new("RGBA", out.size, (0, 0, 0, 0))
    canvas.paste(out, (dx, dy), out)
    return canvas


def ear_twitch(base: Image.Image, left_up=False, right_up=False):
    out = base.copy()
    d = ImageDraw.Draw(out)
    if left_up:
        d.line((8, 3, 9, 1), fill=(24, 20, 26, 255), width=1)
    if right_up:
        d.line((16, 3, 17, 1), fill=(24, 20, 26, 255), width=1)
    return out


def glow_frame(base: Image.Image, box, color):
    out = base.copy()
    d = ImageDraw.Draw(out)
    d.rectangle(box, outline=color)
    return out


def cub_cat_frames() -> list[Image.Image]:
    base = to_canvas(load_png(DEMO / "B02_calico.png"))
    return [
        base,
        ear_twitch(shift_frame(base, 1, 0), left_up=True),
        blink_frame(base, (8, 7), (13, 6)),
        shift_frame(base, -1, 0),
    ]


def tiger_frames() -> list[Image.Image]:
    base = to_canvas(load_png(CUSTOM / "X01_tiger.png"))
    return [
        base,
        shift_frame(base, 1, 0),
        blink_frame(base, (11, 7), (16, 7)),
        shift_frame(base, -1, 0),
    ]


def lion_frames() -> list[Image.Image]:
    base = to_canvas(load_png(CUSTOM / "X02_lion.png"))
    return [
        base,
        shift_frame(base, 1, 0),
        blink_frame(base, (10, 7), (15, 7)),
        shift_frame(base, -1, 0),
    ]


def robo_frames() -> list[Image.Image]:
    base = to_canvas(load_png(CUSTOM / "X03_roboCat.png"))
    return [
        base,
        glow_frame(base, (11, 5, 17, 9), (67, 233, 250, 255)),
        blink_frame(base, (11, 7), (15, 7), width=2),
        glow_frame(base, (11, 5, 17, 9), (255, 255, 255, 255)),
    ]


def encode_frames(name: str, frames: list[Image.Image]) -> str:
    frame_blocks_u16 = []
    frame_blocks_u8 = []

    for im in frames:
        colors = []
        mask = []
        for y in range(SPRITE_SIZE):
            for x in range(SPRITE_SIZE):
                r, g, b, a = im.getpixel((x, y))
                colors.append(rgb565(r, g, b))
                mask.append(1 if a > 0 else 0)

        def fmt(values, u16: bool) -> str:
            rows = []
            for i in range(0, len(values), SPRITE_SIZE):
                chunk = values[i:i + SPRITE_SIZE]
                if u16:
                    rows.append("      " + ", ".join(f"0x{v:04X}" for v in chunk))
                else:
                    rows.append("      " + ", ".join(str(v) for v in chunk))
            return ",\n".join(rows)

        frame_blocks_u16.append("    {\n" + fmt(colors, True) + "\n    }")
        frame_blocks_u8.append("    {\n" + fmt(mask, False) + "\n    }")

    return f"""inline constexpr uint16_t {name}Colors[{FRAME_COUNT}][{SPRITE_SIZE * SPRITE_SIZE}] = {{
{",\n".join(frame_blocks_u16)}
}};

inline constexpr uint8_t {name}Mask[{FRAME_COUNT}][{SPRITE_SIZE * SPRITE_SIZE}] = {{
{",\n".join(frame_blocks_u8)}
}};"""


def main() -> None:
    assets = {
        "CubCat": cub_cat_frames(),
        "Tiger": tiger_frames(),
        "Lion": lion_frames(),
        "RoboCat": robo_frames(),
    }

    blocks = [encode_frames(name, frames) for name, frames in assets.items()]
    text = """#pragma once

#include <stdint.h>

namespace PetSpriteAssets {

inline constexpr int kSpriteWidth = 24;
inline constexpr int kSpriteHeight = 24;
inline constexpr int kFrameCount = 4;

""" + "\n\n".join(blocks) + "\n\n}  // namespace PetSpriteAssets\n"

    OUT.write_text(text)
    print(OUT)


if __name__ == "__main__":
    main()
