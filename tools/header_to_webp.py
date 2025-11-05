#!/usr/bin/env python3

import argparse
from collections import defaultdict
import re
import sys


# Good luck getting numpy and PIL into ESP-IDF's Python.  I suggest you use
# a different Python for this script.
import numpy as np
from PIL import Image, ImageDraw


EXPECTED_HEIGHT = 240
EXPECTED_WIDTH = 240
EXPECTED_PIXELS = EXPECTED_HEIGHT * EXPECTED_WIDTH

def parse_frame(frame):
    assert type(frame) is tuple
    assert len(frame) == 2
    fnum = int(frame[0])
    values_text = re.findall(r'0x[0-9A-Fa-f]+', frame[1])
    # print(f'{values_text[:5] = }')
    values = [int(v, base=0x10) for v in values_text]
    return (fnum, values)

def parse_header(name):
    with open(name) as f:
        text = f.read()
    # print(type(text))
    # print(text[:40])
    frames_text = re.findall(r'uint16_t.*?frame_(\d+)_.*?\{(.*?)\}', text, re.DOTALL);
    # print(f'{len(frames_text) = }')
    frames = [parse_frame(f) for f in frames_text]
    # for f in frames[:3]:
    #     print((f[0], f[1][:10]))
    frames = sorted(frames)
    assert all(f[0] == i for (i, f) in enumerate(frames))
    return frames


def validate(frames):
    assert all(f[0] == i for (i, f) in enumerate(frames))
    assert all(len(f[1]) == EXPECTED_PIXELS for f in frames)


def make_image_array(frames):
    images = []
    arrays = []
    for (index, pixels) in frames:
        assert len(pixels) == 240 * 240
        rgb565 = np.array(pixels, dtype='int32').reshape(240, 240)
        r5 = rgb565 >> (5 + 6)
        g6 = rgb565 >> 5 & 0x3f
        b5 = rgb565 & 0x1f
        r8 = r5 << 3 | r5 >> 2
        g8 = g6 << 2 | g6 >> 4
        b8 = b5 << 3 | b5 >> 2
        # rgb888 = r8 << 16 | g8 << 8 | b8
        rgb888 = np.stack((r8, g8, b8),
                          axis=-1,
                          dtype='uint8', casting='unsafe')
        arrays += [rgb888]
        # print(f'# {rgb888.shape = }')
        images += [Image.fromarray(rgb888, mode='RGB')]
        # images[0].save('foo.png')
        # exit()
    array = np.stack(arrays, axis=0)
    print(f'# {array.shape = }')
    return array, images


def save_array(array, out):
    np.save(out, array)


def write_image_seq(images, out, fps):
    print(f'# {out=}')
    images[0].save(out,
                   save_all=True,
                   append_images=images[1:],
                   duration=1000 / fps)


def parse_args(args):
    ap = argparse.ArgumentParser(
        prog='color_histogram',
        description='Extract a color histogram from a C header file',
    )
    ap.add_argument('file')
    ap.add_argument('-o', '--output', nargs=1, required=True)
    ap.add_argument('-f', '--format', nargs=1, help='output file format')
    ap.add_argument('-c', '--components', action='store_true',
                    help='decompose colors into (r, g, b)')
    ns = ap.parse_args(args)

    # print(f'{ns = }')
    return ns

# print(f'{sys.argv = }')
args = parse_args(sys.argv[1:])
print('#', args)
frames = parse_header(args.file)
validate(frames)
# h = collect_histo(frames)
# print_histo(h, args.components)
array, images = make_image_array(frames)
# save_array(array, args.output[0])
write_image_seq(images, args.output[0], 7.5)
