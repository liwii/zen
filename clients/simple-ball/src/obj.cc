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

void
add_point(Vertex *v, int i, int j)
{
  float theta = glm::radians(360.0f / OBJ_NUM_POINTS_R * j);
  float psi = glm::radians(360.0f / OBJ_NUM_POINTS_R * i);

  (v->p).x = glm::sin(theta) * glm::cos(psi);
  (v->p).y = glm::cos(theta);
  (v->p).z = glm::sin(theta) * glm::sin(psi);

  (v->uv).x = 1.0f - 1.0f / OBJ_NUM_POINTS_R * i;
  (v->uv).y = 1.0f - 1.0f / (OBJ_NUM_POINTS_R / 2) * j;

  (v->norm).x = glm::sin(theta) * glm::cos(psi);
  (v->norm).y = glm::cos(theta);
  (v->norm).z = glm::sin(theta) * glm::sin(psi);
}

int
idx(int i, int j)
{
  return j * OBJ_NUM_POINTS_R + i;
}

u_short *
vertex_indices()
{
  u_short *indices = (u_short *)malloc(sizeof(u_short) * OBJ_NUM_COMPONENTS);
  for (int i = 0; i < OBJ_NUM_POINTS_R; i++) {
    for (int j = 0; j < OBJ_NUM_POINTS_R / 2 - 1; j++) {
      // draw two triangles
      int base = (i * (OBJ_NUM_POINTS_R / 2 - 1) + j) * 6;
      indices[base] = (u_short)idx(i + 1, j);
      indices[base + 1] = (u_short)idx(i, j + 1);
      indices[base + 2] = (u_short)idx(i + 1, j + 1);
      indices[base + 3] = (u_short)idx(i + 1, j + 1);
      indices[base + 4] = (u_short)idx(i, j + 1);
      indices[base + 5] = (u_short)idx(i, j + 2);
    }
  }

  return indices;
}

Vertex *
get_points()
{
  Vertex *points = (Vertex *)malloc(
      sizeof(Vertex) * OBJ_NUM_POINTS_R * (OBJ_NUM_POINTS_R / 2 + 1));
  for (int i = 0; i < OBJ_NUM_POINTS_R; ++i) {
    for (int j = 0; j <= (OBJ_NUM_POINTS_R / 2); ++j) {
      add_point(&points[idx(i, j)], i, j);
    }
  }
  return points;
}