"""Procedural PBR texture sets for OLE-02 demo (sand + rusty metal)."""
import random
from pathlib import Path

from PIL import Image

OUT = Path(__file__).resolve().parent.parent / "textures" / "pbr"
SIZE = 512


def noise2d(w, h, scale=8, seed=0):
    rng = random.Random(seed)
    gw, gh = w // scale + 2, h // scale + 2
    grid = [[rng.random() for _ in range(gw)] for _ in range(gh)]

    def smooth(t):
        return t * t * (3.0 - 2.0 * t)

    out = [[0.0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            fx, fy = x / scale, y / scale
            ix, iy = int(fx), int(fy)
            tx, ty = smooth(fx - ix), smooth(fy - iy)
            v00, v10 = grid[iy][ix], grid[iy][ix + 1]
            v01, v11 = grid[iy + 1][ix], grid[iy + 1][ix + 1]
            v0 = v00 * (1 - tx) + v10 * tx
            v1 = v01 * (1 - tx) + v11 * tx
            out[y][x] = v0 * (1 - ty) + v1 * ty
    return out


def save_gray(path: Path, values):
    img = Image.new("L", (SIZE, SIZE))
    px = img.load()
    for y in range(SIZE):
        for x in range(SIZE):
            px[x, y] = int(max(0, min(1, values[y][x])) * 255)
    img.save(path, quality=92)


def save_rgb(path: Path, rgb_rows):
    img = Image.new("RGB", (SIZE, SIZE))
    img.putdata(rgb_rows)
    img.save(path, quality=92)


def make_sand():
    d = OUT / "sand"
    d.mkdir(parents=True, exist_ok=True)

    n1 = noise2d(SIZE, SIZE, 12, 101)
    n2 = noise2d(SIZE, SIZE, 28, 202)
    albedo = []
    rough = [[0.0] * SIZE for _ in range(SIZE)]
    ao = [[0.0] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            t = 0.55 * n1[y][x] + 0.45 * n2[y][x]
            albedo.append((int(175 + 45 * t), int(145 + 40 * t), int(90 + 30 * t)))
            rough[y][x] = 0.72 + 0.22 * n2[y][x]
            ao[y][x] = 0.88 + 0.12 * n1[y][x]

    save_rgb(d / "sand_Color.jpg", albedo)
    save_gray(d / "sand_Metalness.jpg", [[0.0] * SIZE for _ in range(SIZE)])
    save_gray(d / "sand_Roughness.jpg", rough)
    save_gray(d / "sand_AmbientOcclusion.jpg", ao)
    print("sand ->", d)


def make_rusty_metal():
    d = OUT / "rusty_metal"
    d.mkdir(parents=True, exist_ok=True)

    n = noise2d(SIZE, SIZE, 16, 303)
    n2 = noise2d(SIZE, SIZE, 6, 404)
    albedo = []
    metal = [[0.0] * SIZE for _ in range(SIZE)]
    rough = [[0.0] * SIZE for _ in range(SIZE)]
    ao = [[0.0] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            rust = max(0.0, min(1.0, 0.35 + 0.65 * (0.6 * n[y][x] + 0.4 * n2[y][x])))
            if rust > 0.55:
                albedo.append((int(120 + 60 * rust), int(55 + 30 * rust), int(25 + 15 * rust)))
                metal[y][x] = 0.05 + 0.15 * (1 - rust)
                rough[y][x] = 0.82 + 0.15 * rust
            else:
                t = 0.5 + 0.5 * n2[y][x]
                albedo.append((int(110 * t + 50), int(115 * t + 55), int(120 * t + 60)))
                metal[y][x] = 0.85 + 0.15 * (1 - rust)
                rough[y][x] = 0.18 + 0.25 * n[y][x]
            ao[y][x] = 0.75 + 0.25 * n2[y][x]

    save_rgb(d / "rusty_metal_Color.jpg", albedo)
    save_gray(d / "rusty_metal_Metalness.jpg", metal)
    save_gray(d / "rusty_metal_Roughness.jpg", rough)
    save_gray(d / "rusty_metal_AmbientOcclusion.jpg", ao)
    print("rusty_metal ->", d)


if __name__ == "__main__":
    make_sand()
    make_rusty_metal()
