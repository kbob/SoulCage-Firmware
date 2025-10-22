#pragma once

#include "board_defs.h"
#include "packed_color.h"

const size_t IMAGE_WIDTH = 240;
const size_t IMAGE_HEIGHT = 240;

typedef PackedColorEE<DISPLAY_PIXEL_ORDER, DISPLAY_PIXEL_ENDIAN> pixel_type;
typedef pixel_type image_row_type[IMAGE_WIDTH];
typedef image_row_type image_type[IMAGE_HEIGHT];
