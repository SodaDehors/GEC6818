"""Rebuild embedded UI glyphs: python tools/generate_font.py FONT_FILE.

Pillow is needed on the build PC only. All glyphs share a measured baseline.
"""
import argparse
import math
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
PADDING = 2

def generate(font_path):
    old = (ROOT / 'inc/font_data.h').read_text(encoding='ascii')
    values = re.search(r'font_codepoints\[FONT_GLYPH_COUNT\] = \{([^}]+)', old)[1]
    chars = ''.join(chr(int(v, 16)) for v in re.findall(r'0x[0-9A-Fa-f]+', values))
    for source in sorted((ROOT / 'src').glob('*.c')):
        chars += ''.join(re.findall(r'[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef]', source.read_text(encoding='utf-8')))
    chars = ''.join(dict.fromkeys(c for c in chars if c.isprintable()))
    all_metrics, all_pixels = [], []
    for size in (14, 22, 32):
        font = ImageFont.truetype(font_path, size)
        boxes = [font.getbbox(c) for c in chars]
        top = min(b[1] for c, b in zip(chars, boxes) if c.strip())
        bottom = max(b[3] for c, b in zip(chars, boxes) if c.strip())
        height = bottom - top + 2 * PADDING
        metrics, pixels = [], bytearray()
        for char, box in zip(chars, boxes):
            advance = max(1, math.ceil(font.getlength(char))) + 2 * PADDING
            width = max(advance, box[2] - box[0] + 2 * PADDING)
            image = Image.new('L', (width, height))
            ImageDraw.Draw(image).text((PADDING - box[0], PADDING - top), char, font=font, fill=255)
            ink = image.getbbox()
            if ink:
                assert ink[0] >= PADDING and ink[1] >= PADDING
                assert ink[2] <= width-PADDING and ink[3] <= height-PADDING, (char, size, ink)
            metrics.append((len(pixels), width, height, max(advance, width)))
            pixels.extend(image.tobytes())
        all_metrics.append(metrics)
        all_pixels.append(pixels)
        print(f'{size}px: {len(chars)} glyphs, line height {height}, verified clear margins')
    lines = ['#ifndef FONT_DATA_H', '#define FONT_DATA_H', '#include <stdint.h>',
             f'#define FONT_GLYPH_COUNT {len(chars)}', '#define FONT_STYLE_COUNT 3',
             'typedef struct { uint32_t offset; uint16_t width; uint16_t height; uint16_t advance; } FontGlyph;',
             'static const uint32_t font_codepoints[FONT_GLYPH_COUNT] = {' + ','.join(f'0x{ord(c):X}' for c in chars) + '};',
             'static const FontGlyph font_glyphs[FONT_STYLE_COUNT][FONT_GLYPH_COUNT] = {']
    for metrics in all_metrics:
        lines += ['    {' + ','.join('{%d,%d,%d,%d}' % g for g in metrics) + '},']
    lines += ['};']
    for i, pixels in enumerate(all_pixels):
        lines += [f'static const uint8_t font_pixels_{i}[] = {{']
        lines += ['    ' + ','.join(str(v) for v in pixels[n:n+32]) + ',' for n in range(0,len(pixels),32)]
        lines += ['};']
    lines += ['#endif']
    (ROOT / 'inc/font_data.h').write_text('\n'.join(lines)+'\n', encoding='ascii')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font')
    generate(parser.parse_args().font)
