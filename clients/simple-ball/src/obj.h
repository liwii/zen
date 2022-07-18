#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "opengl.h"

const int OBJ_FRAME_INDICES_NUM = 24;
const int OBJ_FRONT_INDICES_NUM = 6;

const u_short obj_frame_indices[24] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7, 0, 2, 1, 3, 4, 6, 5, 7};

const u_short obj_front_indices[6] = {1, 7, 3, 1, 7, 5};

const int OBJ_TEXTURE_WIDTH = 256;
const int OBJ_TEXTURE_HEIGHT = 256;
const int OBJ_NUM_POINTS = 8;

Vertex *get_points();

void obj_update_texture_buffer_data(void *texture_buffer_data, uint32_t time);

#endif  //  OBJ_H
