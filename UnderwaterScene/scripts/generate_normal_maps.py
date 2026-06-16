"""
OLE-03: Generate procedural normal maps for sand and rusty metal PBR sets.
Uses only PIL (Pillow) - no exotic dependencies.
Output: *_NormalGL.jpg  (OpenGL convention: Y-up)
"""
import math, struct, os

# Minimal BMP writer (no PIL dependency fallback) - but let's try PIL first.
try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def noise2d(x, y, seed=0):
    """Simple value noise via integer hash."""
    n = int(x) + int(y) * 57 + seed * 131
    n = (n << 13) ^ n
    return 1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0


def smoothnoise(x, y, seed=0):
    corners = (noise2d(x-1, y-1, seed) + noise2d(x+1, y-1, seed) +
               noise2d(x-1, y+1, seed) + noise2d(x+1, y+1, seed)) / 16.0
    sides   = (noise2d(x-1, y, seed) + noise2d(x+1, y, seed) +
               noise2d(x, y-1, seed) + noise2d(x, y+1, seed)) / 8.0
    center  = noise2d(x, y, seed) / 4.0
    return corners + sides + center


def interpolate(a, b, x):
    f = (1 - math.cos(x * math.pi)) * 0.5
    return a * (1 - f) + b * f


def interpolated_noise(x, y, seed=0):
    ix, iy = int(x), int(y)
    fx, fy = x - ix, y - iy
    v1 = smoothnoise(ix, iy, seed)
    v2 = smoothnoise(ix+1, iy, seed)
    v3 = smoothnoise(ix, iy+1, seed)
    v4 = smoothnoise(ix+1, iy+1, seed)
    i1 = interpolate(v1, v2, fx)
    i2 = interpolate(v3, v4, fx)
    return interpolate(i1, i2, fy)


def perlin2d(x, y, octaves=6, persistence=0.5, seed=0):
    total = 0.0
    freq = 1.0
    amp = 1.0
    max_val = 0.0
    for _ in range(octaves):
        total += interpolated_noise(x * freq, y * freq, seed) * amp
        max_val += amp
        amp *= persistence
        freq *= 2.0
    return total / max_val


def heightmap_to_normalmap(heightmap, width, height, strength=2.0):
    """Convert a 2D heightmap to an OpenGL normal map (Y-up convention)."""
    pixels = []
    for y in range(height):
        for x in range(width):
            # Sobel-like sampling
            tl = heightmap[((y - 1) % height) * width + ((x - 1) % width)]
            t  = heightmap[((y - 1) % height) * width + x]
            tr = heightmap[((y - 1) % height) * width + ((x + 1) % width)]
            l  = heightmap[y * width + ((x - 1) % width)]
            r  = heightmap[y * width + ((x + 1) % width)]
            bl = heightmap[((y + 1) % height) * width + ((x - 1) % width)]
            b  = heightmap[((y + 1) % height) * width + x]
            br = heightmap[((y + 1) % height) * width + ((x + 1) % width)]

            dx = (tr + 2*r + br) - (tl + 2*l + bl)
            dy = (bl + 2*b + br) - (tl + 2*t + tr)

            nx = -dx * strength
            ny = -dy * strength  # OpenGL Y-up
            nz = 1.0

            length = math.sqrt(nx*nx + ny*ny + nz*nz)
            nx /= length
            ny /= length
            nz /= length

            # Map [-1,1] -> [0,255]
            r_val = int((nx * 0.5 + 0.5) * 255)
            g_val = int((ny * 0.5 + 0.5) * 255)
            b_val = int((nz * 0.5 + 0.5) * 255)
            pixels.append((max(0, min(255, r_val)),
                           max(0, min(255, g_val)),
                           max(0, min(255, b_val))))
    return pixels


def generate_normal_map(output_path, width=512, height=512, scale=8.0, strength=2.0, seed=0):
    """Generate and save a procedural normal map."""
    # Generate heightmap
    heightmap = []
    for y in range(height):
        for x in range(width):
            h = perlin2d(x / width * scale, y / height * scale, octaves=6, persistence=0.5, seed=seed)
            heightmap.append(h)

    pixels = heightmap_to_normalmap(heightmap, width, height, strength)

    if HAS_PIL:
        img = Image.new('RGB', (width, height))
        img.putdata(pixels)
        img.save(output_path, quality=95)
    else:
        # Fallback: write raw BMP
        save_bmp(output_path.replace('.jpg', '.bmp'), width, height, pixels)

    print(f"Generated normal map: {output_path} ({width}x{height})")


def save_bmp(path, w, h, pixels):
    """Write a simple 24-bit BMP (fallback when PIL is unavailable)."""
    row_size = (w * 3 + 3) & ~3
    pixel_size = row_size * h
    file_size = 54 + pixel_size
    with open(path, 'wb') as f:
        # BMP header
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<HH', 0, 0))
        f.write(struct.pack('<I', 54))
        # DIB header
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<i', w))
        f.write(struct.pack('<i', h))
        f.write(struct.pack('<HH', 1, 24))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', pixel_size))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', 0))
        # Pixel data (bottom-up)
        for y in range(h - 1, -1, -1):
            row = b''
            for x in range(w):
                r, g, b = pixels[y * w + x]
                row += bytes([b, g, r])  # BMP is BGR
            row += b'\x00' * (row_size - w * 3)
            f.write(row)


if __name__ == '__main__':
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    # Sand normal map - fine grain, subtle bumps
    generate_normal_map(
        os.path.join(base, 'textures', 'pbr', 'sand', 'sand_NormalGL.jpg'),
        width=512, height=512, scale=12.0, strength=1.5, seed=42
    )

    # Rusty metal normal map - coarser, more pronounced
    generate_normal_map(
        os.path.join(base, 'textures', 'pbr', 'rusty_metal', 'rusty_metal_NormalGL.jpg'),
        width=512, height=512, scale=6.0, strength=3.0, seed=77
    )

    print("Done! Normal maps generated.")
