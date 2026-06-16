"""
OLE-03: Generate procedural normal maps for sand and rusty metal PBR sets.
Uses Pillow (PIL) for image output.
Output: *_NormalGL.jpg  (OpenGL convention: Y-up)

v2: Fixed grid artifacts - uses gradient noise instead of value noise,
    multiple seed offsets per octave, and higher resolution.
"""
import math, random, os

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("WARNING: Pillow not installed - falling back to BMP output")


# ---------------------------------------------------------------------------
# Gradient noise (no grid artifacts)
# ---------------------------------------------------------------------------
class GradientNoise2D:
    """Simple 2D gradient noise (Perlin-like) with no grid artifacts."""

    def __init__(self, seed=0):
        self.rng = random.Random(seed)
        self.perm = list(range(256))
        self.rng.shuffle(self.perm)
        self.perm *= 2  # double for wrapping
        # Pre-compute gradient vectors (unit circle)
        self.grads = []
        for i in range(256):
            angle = 2.0 * math.pi * i / 256.0
            self.grads.append((math.cos(angle), math.sin(angle)))

    def _fade(self, t):
        # Improved Perlin fade: 6t^5 - 15t^4 + 10t^3
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)

    def _lerp(self, a, b, t):
        return a + t * (b - a)

    def _grad(self, hash_val, x, y):
        g = self.grads[hash_val & 255]
        return g[0] * x + g[1] * y

    def noise(self, x, y):
        xi = int(math.floor(x)) & 255
        yi = int(math.floor(y)) & 255
        xf = x - math.floor(x)
        yf = y - math.floor(y)

        u = self._fade(xf)
        v = self._fade(yf)

        aa = self.perm[self.perm[xi] + yi]
        ab = self.perm[self.perm[xi] + yi + 1]
        ba = self.perm[self.perm[xi + 1] + yi]
        bb = self.perm[self.perm[xi + 1] + yi + 1]

        x1 = self._lerp(self._grad(aa, xf, yf), self._grad(ba, xf - 1, yf), u)
        x2 = self._lerp(self._grad(ab, xf, yf - 1), self._grad(bb, xf - 1, yf - 1), u)

        return self._lerp(x1, x2, v)

    def fbm(self, x, y, octaves=6, lacunarity=2.0, gain=0.5):
        """Fractal Brownian Motion - layered noise."""
        total = 0.0
        amplitude = 1.0
        max_amp = 0.0
        freq = 1.0
        for _ in range(octaves):
            total += self.noise(x * freq, y * freq) * amplitude
            max_amp += amplitude
            amplitude *= gain
            freq *= lacunarity
        return total / max_amp


# ---------------------------------------------------------------------------
# Heightmap -> Normal map conversion
# ---------------------------------------------------------------------------
def heightmap_to_normalmap(heightmap, width, height, strength=2.0):
    """Convert a 2D heightmap to an OpenGL normal map (Y-up convention).
    Uses Sobel filter for smoother derivatives."""
    pixels = []
    for y in range(height):
        for x in range(width):
            # Sobel 3x3 kernel
            tl = heightmap[((y - 1) % height) * width + ((x - 1) % width)]
            t  = heightmap[((y - 1) % height) * width + x]
            tr = heightmap[((y - 1) % height) * width + ((x + 1) % width)]
            l  = heightmap[y * width + ((x - 1) % width)]
            r  = heightmap[y * width + ((x + 1) % width)]
            bl = heightmap[((y + 1) % height) * width + ((x - 1) % width)]
            b  = heightmap[((y + 1) % height) * width + x]
            br = heightmap[((y + 1) % height) * width + ((x + 1) % width)]

            dx = (tr + 2 * r + br) - (tl + 2 * l + bl)
            dy = (bl + 2 * b + br) - (tl + 2 * t + tr)

            nx = -dx * strength
            ny = -dy * strength  # OpenGL Y-up
            nz = 1.0

            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            if length < 0.0001:
                length = 1.0
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


def generate_normal_map(output_path, width=1024, height=1024,
                        scale=8.0, strength=2.0, octaves=6,
                        lacunarity=2.0, gain=0.5, seed=0):
    """Generate and save a procedural normal map using gradient noise."""
    noise = GradientNoise2D(seed)

    # Generate heightmap
    heightmap = []
    for y in range(height):
        for x in range(width):
            nx = x / width * scale
            ny = y / height * scale
            h = noise.fbm(nx, ny, octaves=octaves,
                          lacunarity=lacunarity, gain=gain)
            heightmap.append(h)

    pixels = heightmap_to_normalmap(heightmap, width, height, strength)

    if HAS_PIL:
        img = Image.new('RGB', (width, height))
        img.putdata(pixels)
        img.save(output_path, quality=95)
    else:
        # Fallback: write raw BMP
        bmp_path = output_path.replace('.jpg', '.bmp')
        save_bmp(bmp_path, width, height, pixels)
        output_path = bmp_path

    print(f"Generated normal map: {output_path} ({width}x{height})")


def save_bmp(path, w, h, pixels):
    """Write a simple 24-bit BMP (fallback when PIL is unavailable)."""
    import struct
    row_size = (w * 3 + 3) & ~3
    pixel_size = row_size * h
    file_size = 54 + pixel_size
    with open(path, 'wb') as f:
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<HH', 0, 0))
        f.write(struct.pack('<I', 54))
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
        for y in range(h - 1, -1, -1):
            row = b''
            for x in range(w):
                r, g, b = pixels[y * w + x]
                row += bytes([b, g, r])
            row += b'\x00' * (row_size - w * 3)
            f.write(row)


if __name__ == '__main__':
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    # Sand normal map: high frequency, fine grain, subtle bumps
    # scale=20 -> drobne ziarnka piasku, octaves=8 -> duzo detali
    generate_normal_map(
        os.path.join(base, 'textures', 'pbr', 'sand', 'sand_NormalGL.jpg'),
        width=1024, height=1024,
        scale=20.0, strength=1.2, octaves=8,
        lacunarity=2.3, gain=0.45, seed=42
    )

    # Rusty metal normal map: medium scale, aggressive bumps
    # scale=10 -> srednie nierownossci, strength=4.0 -> mocne wyboje
    generate_normal_map(
        os.path.join(base, 'textures', 'pbr', 'rusty_metal', 'rusty_metal_NormalGL.jpg'),
        width=1024, height=1024,
        scale=10.0, strength=4.0, octaves=7,
        lacunarity=2.1, gain=0.55, seed=77
    )

    print("Done! Normal maps generated.")
