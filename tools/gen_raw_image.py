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


def reformat(frames, format):
    reformatted_frames = []
    for (fno, pixels) in frames:
        if format == 'rgb565':
            npixels = pixels
        elif format == 'bgr565':
            npixels = []
            for p in pixels:
                r = p >> 11 & 0x1F;
                g = p & 0x07E0;
                b = p & 0x1F;
                np = b << 11 | g | r
                # if r != b:
                #     print(f'{p=:04x} {r=:04x} {g=:04x} {b=:04x} {np=:04x}')
                npixels.append(np)
        else:
            assert False, f'unknown pixel format {format}'
        reformatted_frames.append((fno, npixels))
    return reformatted_frames


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
    formats = ['rgb565', 'bgr565']  # add more later
    ap.add_argument('-f', '--format', choices=formats, default='rgb565')
    ns = ap.parse_args(args)

    # print(f'{ns = }')
    return ns

# print(f'{sys.argv = }')
args = parse_args(sys.argv[1:])
frames = parse_header(args.file)
validate(frames)
frames = reformat(frames, args.format)
# big endian is the default.  So only look at little_endian.
binary = gen_binary(frames, args.little_endian, args.padding)
write_binary(args.output[0], binary)
