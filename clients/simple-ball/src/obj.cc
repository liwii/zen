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
  Vertex *points = (Vertex *)malloc(sizeof(Vertex) * 4);
  points[0] = Vertex{glm::vec3(0, 0, 0), glm::vec2(0, 0), glm::vec3(0, 0, 1)};
  points[1] = Vertex{glm::vec3(1, 0, 0), glm::vec2(1, 0), glm::vec3(0, 0, 1)};
  points[2] = Vertex{glm::vec3(0, 1, 0), glm::vec2(0, 1), glm::vec3(0, 0, 1)};
  points[3] = Vertex{glm::vec3(1, 1, 0), glm::vec2(1, 1), glm::vec3(0, 0, 1)};
  return points;
}