#include "obj.h"

#include "opengl.h"

void
obj_update_texture_buffer_data(void *texture_buffer_data, uint32_t time)
{
  struct ColorBGRA *pixel = (struct ColorBGRA *)texture_buffer_data;
  for (int x = 0; x < OBJ_TEXTURE_WIDTH; x++) {
    for (int y = 0; y < OBJ_TEXTURE_HEIGHT; y++) {
      // The distance from origin  * 2
      int r2 = x * x + y * y;
      int opacity_distance_square =
          (OBJ_TEXTURE_WIDTH * OBJ_TEXTURE_WIDTH +
              OBJ_TEXTURE_HEIGHT * OBJ_TEXTURE_HEIGHT) /
          4;
      pixel->a = r2 < opacity_distance_square ? 0 : UINT8_MAX;
      pixel->r = x;
      pixel->g = y;
      pixel->b = (time / 10) % UINT8_MAX;
      pixel++;
    }
  }
}

Vertex *
get_points()
{
  Vertex *points = (Vertex *)malloc(sizeof(Vertex) * OBJ_NUM_POINTS);
  float length = 0.2f;
  int i = 0;
  for (int x = -1; x < 2; x += 2) {
    for (int y = -1; y < 2; y += 2) {
      for (int z = -1; z < 2; z += 2) {
        points[i].p.x = length * x;
        points[i].p.y = length * y;
        points[i].p.z = length * z;
        points[i].u = x < 0 ? 0 : 1;
        points[i].v = y < 0 ? 0 : 1;
        i++;
      }
    }
  }
  return points;
}