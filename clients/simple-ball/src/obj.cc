#include "obj.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "buffer.h"
#include "opengl.h"
#include "string.h"

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

Env *
setup_env()
{
  Env *e = (Env *)malloc(sizeof(Env));
  e->projection =
      glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
  e->camera = glm::vec3(4, 3, 3);
  e->view = glm::lookAt(e->camera, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  e->model = glm::mat4(1.0f);
  e->light = glm::vec3(0, 5, 10);
  return e;
}

void
update_env(Env *e)
{
  e->camera = glm::rotate(e->camera, 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
  e->view = glm::lookAt(e->camera, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

void
set_obj_uniform_variables(zgn_opengl_shader_program *shader, Env *e)
{
  set_shader_uniform_variable(shader, "Model", e->model);
  set_shader_uniform_variable(shader, "View", e->view);
  set_shader_uniform_variable(shader, "Projection", e->projection);
  set_shader_uniform_variable(shader, "LightPos", e->light);
  set_shader_uniform_variable(shader, "Camera", e->camera);
}

zgn_opengl_vertex_buffer *
opengl_setup_vertex_buffer(
    zgn_opengl *opengl, wl_shm *shm, Vertex *points, uint points_len)
{
  zgn_opengl_vertex_buffer *vertex_buffer =
      zgn_opengl_create_vertex_buffer(opengl);
  uint points_size = sizeof(Vertex) * points_len;
  buffer *vertex_buffer_data = create_buffer(shm, points_size);
  Vertex *vertices = (Vertex *)vertex_buffer_data->data;
  memcpy(vertices, points, points_size);
  zgn_opengl_vertex_buffer_attach(vertex_buffer, vertex_buffer_data->buffer);
  return vertex_buffer;
}

// bind to the vertex buffer

// ?? texture??

// draw (non-obj specific)
