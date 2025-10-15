#!/usr/bin/env python3

import argparse
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


def gen_binary(frames, little_endian, pad_size):

    def frame_msbs(frame):
        return [pix >> 8 for pix in frame[1]]

    def frame_lsbs(frame):
        return [pix & 0xFF for pix in frame[1]]

    def frame_interleaved(frame):
        msbs = frame_msbs(frame)
        lsbs = frame_lsbs(frame)
        # print(f'{frame[0]}: {msbs[:10] =}')
        # print(f'{frame[0]}: {lsbs[:10] =}')
        
        seq = [0] * (len(msbs) + len(lsbs))
        seq[little_endian::2] = msbs
        seq[not little_endian::2] = lsbs
        # print(f'{frame[0]}: {seq[:10] = }')
        return seq

    interleaved = sum((frame_interleaved(f) for f in frames), [])
    # print(f'{interleaved[:10] = }')
    np = -len(interleaved) % pad_size
    padding = b'\xff' * np
    return bytes(interleaved) + padding


def write_binary(file, binary):
    assert type(binary) == bytes
    with open(file, 'wb') as out:
        out.write(binary)

def parse_args(args):
    ap = argparse.ArgumentParser(
        prog='gen_raw_image',
        description='Convert C header file to binary data',
    )
    ap.add_argument('file')
    ap.add_argument('-o', '--output', nargs=1, required=True)
    ap.add_argument('-p', '--padding', nargs=1, type=int, default=4096)
    endians = ap.add_mutually_exclusive_group()
    endians.add_argument('-B', '--big-endian', action='store_true')
    endians.add_argument('-l', '--little-endian', action='store_true')
    ns = ap.parse_args(args)
    # print(f'{ns = }')
    return ns

# print(f'{sys.argv = }')
## args = parse_args(sys.argv[1:])
args = parse_args('-o /dev/null --padding=1 images/Intro.h'.split())
frames = parse_header(args.file)
validate(frames)
# big endian is the default.  So only look at little_endian.
## binary = gen_binary(frames, args.little_endian, args.padding)
## write_binary(args.output[0], binary)



from collections import defaultdict

# print(f'{type(frames)=}')
# print(f'{len(frames)=}')
# f0 = frames[0]
# print(f'{type(f0)=}')
# print(f'{len(f0)=}')

# f0a = frames[0][0]
# print(f'{type(f0a)=}')
# print(f'{f0a=}')
# f0b = frames[0][1]
# print(f'{type(f0b)=}')
# print(f'{len(f0b)=}')
# print(f'{f0b[:5]=}')

for frame in frames:
    histo = defaultdict(int)
    for pix in frame[1]:
        histo[pix] += 1
    print(f'frame {frame[0]}:')
    for k in sorted(histo, reverse=True, key=lambda k: histo[k])[:14]:
        r = (k >> (5 + 6) & 0x1f) * 2
        g = k >> 5 & 0x3f
        b = (k & 0x1f) * 2
        print(f'   {k:04x}: {histo[k]} ({r} {g} {b})')
    print()
    if len(histo) > 1:
        break

for frame in frames:
    (fno, pixels) = frame
    if 0xF800 in pixels:
        print(f'0xF800 in f{fno}[{pixels.index(0xF800)}]')
        break