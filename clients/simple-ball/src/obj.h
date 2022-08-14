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
const int OBJ_NUM_POINTS_R = 100;
const int OBJ_NUM_POINTS = OBJ_NUM_POINTS_R * (OBJ_NUM_POINTS_R / 2 + 1);
const int OBJ_NUM_COMPONENTS =
    6 * OBJ_NUM_POINTS_R * (OBJ_NUM_POINTS_R / 2 - 1);

Vertex *get_points();
u_short *vertex_indices();

void obj_update_texture_buffer_data(void *texture_buffer_data, uint32_t time);

#endif  //  OBJ_H
