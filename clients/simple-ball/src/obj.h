#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "opengl.h"

const int OBJ_FRAME_INDICES_NUM = 6;
const int OBJ_FRONT_INDICES_NUM = 6;

const u_short obj_frame_indices[6] = {1, 7, 3, 1, 7, 5};

const u_short obj_front_indices[6] = {0, 1, 2, 2, 1, 3};

const int OBJ_TEXTURE_WIDTH = 256;
const int OBJ_TEXTURE_HEIGHT = 256;
const int OBJ_NUM_POINTS = 4;

Vertex *get_points();

void obj_update_texture_buffer_data(void *texture_buffer_data, uint32_t time);

#endif  //  OBJ_H
