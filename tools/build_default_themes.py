#!/usr/bin/env python3
"""Build the LCD-ready OpenRemote default theme collection."""

from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter


SOFTWARE_ROOT = next(
    parent for parent in Path(__file__).resolve().parents if parent.name == "SOFTWARE"
)
ROOT = SOFTWARE_ROOT / "SD Card Structure" / "themes" / "Default"
WIDTH = 240
HEIGHT = 320
RGB565_REVISION = 3


THEMES = [
    {
        "id": "smooth_blue",
        "name": "Smooth Blue",
        "mode": "gradient",
        "split": 112,
        "colour1": "#0b80db",
        "colour2": "#12386c",
        "colour3": "#03060d",
        "gradientStyle": "glass-glow",
        "glassColour": "#07172a",
        "glassTransparency": 26,
    },
    {
        "id": "obsidian_silk",
        "name": "Obsidian Silk",
        "mode": "gradient",
        "split": 116,
        "colour1": "#77818f",
        "colour2": "#1e252e",
        "colour3": "#040608",
        "gradientStyle": "metallic",
        "glassColour": "#10151c",
        "glassTransparency": 24,
    },
    {
        "id": "aurora_glass",
        "name": "Aurora Glass",
        "mode": "gradient",
        "split": 122,
        "colour1": "#35bea9",
        "colour2": "#155e85",
        "colour3": "#030a12",
        "gradientStyle": "aurora",
        "glassColour": "#092633",
        "glassTransparency": 25,
    },
    {
        "id": "champagne_noir",
        "name": "Champagne Noir",
        "mode": "gradient",
        "split": 118,
        "colour1": "#d8bd7d",
        "colour2": "#695431",
        "colour3": "#070605",
        "gradientStyle": "dual-spotlight",
        "glassColour": "#211b12",
        "glassTransparency": 25,
    },
    {
        "id": "grand_cinema",
        "name": "Grand Cinema",
        "mode": "combined",
        "split": 168,
        "colour1": "#9a673e",
        "colour2": "#2a1c18",
        "colour3": "#050506",
        "gradientStyle": "cinematic",
        "glassColour": "#1b1414",
        "glassTransparency": 27,
    },
    {
        "id": "alpine_ember",
        "name": "Alpine Ember",
        "mode": "combined",
        "split": 174,
        "colour1": "#b56d35",
        "colour2": "#3c281e",
        "colour3": "#070606",
        "gradientStyle": "ambient",
        "glassColour": "#251b16",
        "glassTransparency": 26,
    },
    {
        "id": "midnight_penthouse",
        "name": "Midnight Penthouse",
        "mode": "image",
        "split": 152,
        "colour1": "#57809f",
        "colour2": "#1d2b38",
        "colour3": "#04070a",
        "gradientStyle": "vignette",
        "glassColour": "#111b24",
        "glassTransparency": 25,
    },
]


def hex_rgb(value: str) -> tuple[int, int, int]:
    value = value.lstrip("#")
    return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))


def mix(a: tuple[int, int, int], b: tuple[int, int, int], amount: float):
    amount = max(0.0, min(1.0, amount))
    return tuple(round(a[i] * (1.0 - amount) + b[i] * amount) for i in range(3))


def soft_spot(x: float, y: float, cx: float, cy: float, radius: float) -> float:
    distance = math.hypot(x - cx, y - cy) / radius
    return max(0.0, 1.0 - distance) ** 2


def gradient_theme(theme: dict) -> Image.Image:
    accent = hex_rgb(theme["colour1"])
    secondary = hex_rgb(theme["colour2"])
    base = hex_rgb(theme["colour3"])
    style = theme["gradientStyle"]
    image = Image.new("RGB", (WIDTH, HEIGHT))
    pixels = image.load()

    for y in range(HEIGHT):
        ny = y / (HEIGHT - 1)
        for x in range(WIDTH):
            nx = x / (WIDTH - 1)
            colour = mix(secondary, base, min(1.0, ny * 0.94 + 0.06))
            if style == "glass-glow":
                # Smooth Blue deliberately avoids radial spotlights. The old
                # concentric falloff looked like RGB565 banding even after
                # dithering; a continuous vertical/diagonal blend matches the
                # clean gradient produced by WebConfig's theme editor.
                upper = max(0.0, 1.0 - ny * 1.24)
                diagonal = (1.0 - nx) * max(0.0, 0.58 - ny) * 0.12
                colour = mix(colour, accent, upper * 0.54 + diagonal)
            elif style == "metallic":
                sheen = math.exp(-((nx + ny * 0.44 - 0.56) / 0.17) ** 2)
                edge = soft_spot(nx, ny, 0.82, 0.12, 0.62)
                colour = mix(colour, accent, sheen * 0.25 + edge * 0.10)
            elif style == "aurora":
                wave = 0.50 + 0.50 * math.sin(nx * 5.4 + ny * 3.1)
                ribbon = math.exp(-((ny - (0.26 + 0.13 * wave)) / 0.21) ** 2)
                glow = soft_spot(nx, ny, 0.17, 0.55, 0.68)
                colour = mix(colour, accent, ribbon * 0.43 + glow * 0.22)
            else:
                left = soft_spot(nx, ny, 0.18, 0.12, 0.63)
                right = soft_spot(nx, ny, 0.88, 0.37, 0.78)
                colour = mix(colour, accent, left * 0.29 + right * 0.16)

            # Fine deterministic grain keeps low-frequency gradients from
            # collapsing into visible 16-bit bands before RGB565 dithering.
            grain = (((x * 17 + y * 31) % 13) - 6) * 0.38
            pixels[x, y] = tuple(max(0, min(255, round(c + grain))) for c in colour)

    return image if style == "glass-glow" else image.filter(ImageFilter.GaussianBlur(0.32))


def image_theme(theme: dict) -> Image.Image:
    source_path = ROOT / f"{theme['id']}_source.png"
    source = Image.open(source_path).convert("RGB")
    source = source.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    source = ImageEnhance.Contrast(source).enhance(1.06)
    source = ImageEnhance.Color(source).enhance(0.90)
    overlay = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    for y in range(HEIGHT):
        ny = y / (HEIGHT - 1)
        if theme["mode"] == "combined":
            split = theme["split"] / HEIGHT
            alpha = int(18 + 172 * max(0.0, (ny - split) / max(0.01, 1.0 - split)))
        else:
            alpha = int(22 + 58 * ny)
        draw.line((0, y, WIDTH, y), fill=(2, 5, 8, alpha))

    # Vignette protects white labels without flattening the actual photograph.
    vignette = Image.new("L", (WIDTH, HEIGHT), 0)
    vp = vignette.load()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            edge = max(abs(x / (WIDTH - 1) - 0.5) * 1.75,
                       abs(y / (HEIGHT - 1) - 0.5) * 1.30)
            vp[x, y] = int(max(0.0, min(1.0, edge - 0.36)) * 95)
    black = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    black.putalpha(vignette.filter(ImageFilter.GaussianBlur(16)))
    return Image.alpha_composite(Image.alpha_composite(source.convert("RGBA"), overlay), black).convert("RGB")


def rgb565_dither(image: Image.Image, output: Path) -> None:
    rgb_image = image.convert("RGB")
    if hasattr(rgb_image, "get_flattened_data"):
        source = list(rgb_image.get_flattened_data())
    else:
        source = list(rgb_image.getdata())
    current = [[0.0] * (WIDTH + 2) for _ in range(3)]
    following = [[0.0] * (WIDTH + 2) for _ in range(3)]
    encoded = bytearray(WIDTH * HEIGHT * 2)

    for y in range(HEIGHT):
        left_to_right = y % 2 == 0
        xs = range(WIDTH) if left_to_right else range(WIDTH - 1, -1, -1)
        for x in xs:
            src = source[y * WIDTH + x]
            idx = x + 1
            values = [max(0.0, min(255.0, src[c] + current[c][idx])) for c in range(3)]
            quantised = [round(values[0] * 31 / 255), round(values[1] * 63 / 255), round(values[2] * 31 / 255)]
            restored = [quantised[0] * 255 / 31, quantised[1] * 255 / 63, quantised[2] * 255 / 31]
            errors = [values[c] - restored[c] for c in range(3)]
            forward = idx + 1 if left_to_right else idx - 1
            lower_back = idx - 1 if left_to_right else idx + 1
            lower_forward = idx + 1 if left_to_right else idx - 1
            for c in range(3):
                current[c][forward] += errors[c] * 7 / 16
                following[c][lower_back] += errors[c] * 3 / 16
                following[c][idx] += errors[c] * 5 / 16
                following[c][lower_forward] += errors[c] / 16
            value = (quantised[0] << 11) | (quantised[1] << 5) | quantised[2]
            out = (y * WIDTH + x) * 2
            encoded[out] = value & 0xFF
            encoded[out + 1] = (value >> 8) & 0xFF
        current, following = following, [[0.0] * (WIDTH + 2) for _ in range(3)]

    output.write_bytes(encoded)


def build() -> None:
    manifest = []
    for theme in THEMES:
        image = gradient_theme(theme) if theme["mode"] == "gradient" else image_theme(theme)
        preview_path = ROOT / f"{theme['id']}.png"
        runtime_path = ROOT / f"{theme['id']}.rgb565"
        image.save(preview_path, optimize=True)
        if theme["mode"] == "gradient":
            image.save(ROOT / f"{theme['id']}_source.png", optimize=True)
        rgb565_dither(image, runtime_path)
        record = dict(theme)
        record.update(
            {
                "cropSize": "240x320",
                "glassEnabled": True,
                "library": "default",
                "builtIn": True,
                "runtimePath": f"/themes/Default/{theme['id']}.rgb565",
                "previewPath": f"/themes/Default/{theme['id']}.png",
                "sourcePath": f"/themes/Default/{theme['id']}_source.png",
                "rgb565Revision": RGB565_REVISION,
            }
        )
        manifest.append(record)

    (ROOT / "themes.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    build()
