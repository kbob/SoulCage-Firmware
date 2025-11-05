#!/usr/bin/env python

import argparse
import numpy as np
from PIL import Image
import sys


def parse_args(args):
    ap = argparse.ArgumentParser(
        prog='resample',
        description='Resample an image, interpolating frames'
    )
    ap.add_argument('infile')
    ap.add_argument('-o', '--output', nargs=1, required=True)
    ns = ap.parse_args(args)
    return ns

def repeat(array, n):
    return np.repeat(array, n, axis=0)


def interpolate(array, n):
    nf = array.shape[0]
    result_shape = list(array.shape)
    result_shape[0] *= n
    result = np.empty(result_shape, dtype=np.uint8)
    for i in range(nf):
        f0 = array[i, ...].astype(np.uint32)
        f1 = array[(i + 1) % nf, ...].astype(np.uint32)
        for j in range(n):
            f = (((n - j) * f0 + j * f1) // n).astype(np.uint8)
            result[n * i + j, ...] = f
    return result


def quantize(array):
    array[..., 0] &= 0xF8;
    array[..., 1] &= 0xFC;
    array[..., 2] &= 0xF8;


def sequence_to_images(sequence):
    return [Image.fromarray(frame, mode='RGB') for frame in sequence]


def write_image_seq(images, out, fps):
    print(f'# {out=}')
    images[0].save(out,
                   save_all=True,
                   append_images=images[1:],
                   duration=1000 / fps)


args = parse_args(sys.argv[1:])
infile = args.infile
outfile = args.output[0]

array = np.load(infile)
print(f'{array.shape=}')
repeated = repeat(array, 8)
print(f'{repeated.shape=}')
interpolated = interpolate(array, 8)
quantize(interpolated)
print(f'{interpolated.shape=}')
side_by_side = np.concatenate([repeated, interpolated], axis=2)
print(f'{side_by_side.shape=}')

for i in range(interpolated.shape[0]):
    repeated[i, 0:10, 0:10, 0] = i % 256

images = sequence_to_images(side_by_side)
# images = sequence_to_images(interpolated)
# images = sequence_to_images(repeated)
print(f'{images[0]=}')
# itmp = sequence_to_images(array)
# write_image_seq(itmp, f'tmp.{outfile}', 60)
write_image_seq(images, outfile, 60)
