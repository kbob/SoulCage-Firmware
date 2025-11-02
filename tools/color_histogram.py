#!/usr/bin/env python3

import argparse
from collections import defaultdict
import re
import sys


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


def collect_histo(frames):
    histo = defaultdict(int)
    for (fnum, pixels) in frames:
        for pix in pixels:
            histo[pix] += 1
    return histo


def print_histo(histo, decompose):
    print('color_histogram = {')
    items = sorted(histo.items(), key=lambda i: i[1], reverse=True)
    if decompose:
        for (k, v) in items:
            (r, g, b) = (k >> 11, k >> 5 & 0x3f, k & 0x1f)
            print(f'    ({r}, {g}, {b}): {v}')
    else:
        for (k, v) in items:
            print(f'    {k:#04x}: {v}')
    print('}')


def parse_args(args):
    ap = argparse.ArgumentParser(
        prog='color_histogram',
        description='Extract a color histogram from a C header file',
    )
    ap.add_argument('file')
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
h = collect_histo(frames)
print_histo(h, args.components)
