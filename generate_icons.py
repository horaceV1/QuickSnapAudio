#!/usr/bin/env python3
"""Generate icon files for QuickSnapAudio."""
import struct
import zlib
import os

def create_png(width, height, filename):
    """Create a simple PNG icon with a speaker/lightning bolt design."""
    pixels = []
    cx, cy = width // 2, height // 2
    
    for y in range(height):
        row = []
        for x in range(width):
            # Background: dark circle
            dx, dy = x - cx, y - cy
            dist = (dx*dx + dy*dy) ** 0.5
            radius = min(width, height) // 2 - 2
            
            if dist <= radius:
                # Inside circle - gradient background
                t = dist / radius
                r = int(30 + t * 15)
                g = int(30 + t * 20)
                b = int(46 + t * 30)
                a = 255
                
                # Lightning bolt shape
                bolt_points = [
                    (cx - 2, cy - radius * 0.6),
                    (cx + 4, cy - radius * 0.05),
                    (cx - 1, cy - radius * 0.05),
                    (cx + 3, cy + radius * 0.6),
                    (cx - 3, cy + radius * 0.05),
                    (cx + 2, cy + radius * 0.05),
                ]
                
                # Simple lightning check - draw a bright shape
                nx = (x - cx) / radius
                ny = (y - cy) / radius
                
                # Lightning bolt approximation
                in_bolt = False
                if -0.5 < ny < 0.5:
                    if ny < 0:
                        left = -0.15 + ny * 0.3
                        right = 0.25 + ny * 0.1
                        in_bolt = left < nx < right
                    else:
                        left = -0.25 + ny * 0.1
                        right = 0.15 - ny * 0.3
                        in_bolt = left < nx < right
                elif -0.65 < ny <= -0.5:
                    in_bolt = -0.05 < nx < 0.2
                elif 0.5 <= ny < 0.65:
                    in_bolt = -0.2 < nx < 0.05
                
                if in_bolt:
                    # Lightning yellow-gold
                    r, g, b = 249, 226, 175
                    a = 255
                
                # Speaker outline on the left
                if -0.7 < ny < 0.7 and -0.65 < nx < -0.25:
                    snx = (nx + 0.65) / 0.4
                    if abs(ny) < 0.15 + snx * 0.35:
                        r, g, b = 137, 180, 250
                        a = 255
                
                row.append((r, g, b, a))
            else:
                row.append((0, 0, 0, 0))
        pixels.append(row)
    
    # Encode PNG
    def make_png(w, h, px):
        def chunk(chunk_type, data):
            c = chunk_type + data
            crc = zlib.crc32(c) & 0xffffffff
            return struct.pack('>I', len(data)) + c + struct.pack('>I', crc)
        
        sig = b'\x89PNG\r\n\x1a\n'
        ihdr = struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)
        
        raw = b''
        for row in px:
            raw += b'\x00'  # filter byte
            for r, g, b, a in row:
                raw += struct.pack('BBBB', r, g, b, a)
        
        compressed = zlib.compress(raw)
        
        return sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', compressed) + chunk(b'IEND', b'')
    
    data = make_png(width, height, pixels)
    with open(filename, 'wb') as f:
        f.write(data)

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    res_dir = os.path.join(script_dir, 'resources')
    
    create_png(256, 256, os.path.join(res_dir, 'icon.png'))
    print("Generated icon.png (256x256)")
    
    # For Windows .ico, just copy the PNG (modern Windows supports PNG in ICO)
    # Create a proper ICO file with PNG payload
    png_data = open(os.path.join(res_dir, 'icon.png'), 'rb').read()
    
    ico_header = struct.pack('<HHH', 0, 1, 1)  # Reserved, Type=ICO, Count=1
    ico_entry = struct.pack('<BBBBHHII', 
        0, 0,  # Width=256 (0=256), Height=256
        0, 0,  # ColorPalette, Reserved
        1, 32,  # Planes, BitsPerPixel
        len(png_data),  # Size
        22  # Offset (6 header + 16 entry)
    )
    
    with open(os.path.join(res_dir, 'icon.ico'), 'wb') as f:
        f.write(ico_header + ico_entry + png_data)
    
    print("Generated icon.ico")
