#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "opengl.h"

const int OBJ_TEXTURE_WIDTH = 256;
const int OBJ_TEXTURE_HEIGHT = 256;
const int OBJ_NUM_POINTS = 8;

Vertex *get_points();

void obj_update_texture_buffer_data(void *texture_buffer_data, uint32_t time);

#endif  //  OBJ_H
