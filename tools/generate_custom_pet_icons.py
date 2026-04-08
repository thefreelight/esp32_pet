from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path("/Users/jordan/Projects/esp32_pet")
DEMO = ROOT / "assets" / "pet_icons" / "demo"
OUT = ROOT / "assets" / "pet_icons" / "custom"
OUT.mkdir(parents=True, exist_ok=True)


def load(name: str) -> Image.Image:
    return Image.open(DEMO / name).convert("RGBA")


def solid(im: Image.Image, color):
    out = Image.new("RGBA", im.size, (0, 0, 0, 0))
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = im.getpixel((x, y))
            if a:
                out.putpixel((x, y), color)
    return out


def tiger_icon() -> Image.Image:
    base = load("B02_calico.png")
    out = Image.new("RGBA", base.size, (0, 0, 0, 0))

    for y in range(base.height):
        for x in range(base.width):
            r, g, b, a = base.getpixel((x, y))
            if not a:
                continue
            if r < 50 and g < 50 and b < 50:
                out.putpixel((x, y), (20, 18, 24, 255))
            elif r > 220 and g > 210 and b > 180:
                out.putpixel((x, y), (250, 244, 230, 255))
            elif r > 170 and g > 120:
                out.putpixel((x, y), (246, 158, 67, 255))
            else:
                out.putpixel((x, y), (230, 126, 48, 255))

    d = ImageDraw.Draw(out)
    # bold tiger stripes
    stripes = [
        [(13, 11), (14, 16)],
        [(17, 10), (18, 17)],
        [(21, 10), (22, 17)],
        [(26, 12), (27, 18)],
        [(10, 14), (11, 24)],
        [(15, 14), (16, 25)],
        [(20, 14), (21, 25)],
        [(25, 14), (26, 24)],
        [(29, 15), (30, 23)],
    ]
    for (x0, y0), (x1, y1) in stripes:
        d.line((x0, y0, x1, y1), fill=(28, 24, 24, 255), width=2)

    # forehead stripe marks
    d.line((20, 8, 18, 12), fill=(28, 24, 24, 255), width=1)
    d.line((23, 8, 25, 12), fill=(28, 24, 24, 255), width=1)
    return out


def lion_icon() -> Image.Image:
    base = load("B11_solidBlack.png")
    out = Image.new("RGBA", base.size, (0, 0, 0, 0))
    mane = Image.new("RGBA", base.size, (0, 0, 0, 0))
    md = ImageDraw.Draw(mane)
    # mane placed behind the head area
    md.ellipse((12, 3, 29, 20), fill=(125, 75, 36, 255))
    md.ellipse((10, 5, 27, 22), fill=(125, 75, 36, 255))
    md.ellipse((14, 5, 31, 22), fill=(125, 75, 36, 255))
    out.alpha_composite(mane)

    for y in range(base.height):
        for x in range(base.width):
            r, g, b, a = base.getpixel((x, y))
            if not a:
                continue
            if r < 30 and g < 30 and b < 30:
                out.putpixel((x, y), (28, 22, 24, 255))
            else:
                out.putpixel((x, y), (238, 188, 102, 255))

    d = ImageDraw.Draw(out)
    # cream muzzle/chest
    d.rectangle((19, 12, 25, 16), fill=(248, 234, 198, 255))
    d.rectangle((16, 19, 24, 23), fill=(248, 234, 198, 255))
    # tail tuft
    d.ellipse((28, 3, 33, 8), fill=(125, 75, 36, 255))
    return out


def robo_cat_icon() -> Image.Image:
    base = load("B16_tuxedo.png")
    out = Image.new("RGBA", base.size, (0, 0, 0, 0))

    for y in range(base.height):
        for x in range(base.width):
            r, g, b, a = base.getpixel((x, y))
            if not a:
                continue
            if r < 50 and g < 50 and b < 60:
                out.putpixel((x, y), (30, 34, 42, 255))
            elif r > 200 and g > 180 and b > 180:
                out.putpixel((x, y), (230, 238, 244, 255))
            else:
                out.putpixel((x, y), (110, 132, 160, 255))

    d = ImageDraw.Draw(out)
    # square head frame and visor
    d.rectangle((15, 6, 29, 19), outline=(28, 32, 40, 255), fill=(120, 146, 176, 255))
    d.rectangle((18, 10, 26, 15), fill=(65, 225, 246, 255))
    d.rectangle((20, 12, 21, 13), fill=(24, 28, 34, 255))
    d.rectangle((23, 12, 24, 13), fill=(24, 28, 34, 255))
    # antenna
    d.rectangle((21, 3, 22, 6), fill=(65, 225, 246, 255))
    d.rectangle((20, 2, 23, 3), fill=(65, 225, 246, 255))
    # battery core
    d.rectangle((15, 20, 25, 24), fill=(84, 120, 208, 255))
    return out


def save_preview():
    files = [
        ("demo calico", load("B02_calico.png")),
        ("custom tiger", tiger_icon()),
        ("demo black", load("B11_solidBlack.png")),
        ("custom lion", lion_icon()),
        ("demo tuxedo", load("B16_tuxedo.png")),
        ("custom robo", robo_cat_icon()),
    ]
    canvas = Image.new("RGBA", (980, 360), (18, 24, 48, 255))
    x = 20
    for label, im in files:
        big = im.resize((140, 140), Image.Resampling.NEAREST)
        canvas.alpha_composite(big, (x, 40))
        d = ImageDraw.Draw(canvas)
        d.text((x, 200), label, fill=(235, 242, 255, 255))
        x += 155
    out = ROOT / "design_candidates" / "custom_icons_preview_v1.png"
    canvas.save(out)
    print(out)


def main():
    tiger = tiger_icon()
    lion = lion_icon()
    robo = robo_cat_icon()

    tiger.save(OUT / "X01_tiger.png")
    lion.save(OUT / "X02_lion.png")
    robo.save(OUT / "X03_roboCat.png")
    save_preview()


if __name__ == "__main__":
    main()
