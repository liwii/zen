#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <zigen-opengl-client-protocol.h>

#include <glm/glm.hpp>

#include "buffer.h"

int
get_shared_shader_fd(const char *shader)
{
  size_t shader_len = strlen(shader);
  int shader_fd = create_shared_fd(shader_len);
  if (shader_fd == -1) {
    return -1;
  }

  void *shader_data =
      mmap(NULL, shader_len, PROT_WRITE, MAP_SHARED, shader_fd, 0);
  if (shader_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    return EXIT_FAILURE;
  }
  memcpy(shader_data, shader, shader_len);
  munmap(shader_data, shader_len);

  return shader_fd;
};

static void
wl_array_init_(wl_array *array, uint elements, void *tensor)
{
  wl_array_init(array);
  size_t size = sizeof(float) * elements;
  float *data = (float *)wl_array_add(array, size);
  memcpy(data, tensor, size);
}

void
set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::mat4 mat)
{
  struct wl_array array;
  wl_array_init_(&array, 16, (void *)&mat);
  zgn_opengl_shader_program_set_uniform_float_matrix(
      shader, location, 4, 4, false, 1, &array);
  wl_array_release(&array);
}

void
set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::vec4 vec)
{
  struct wl_array array;
  wl_array_init_(&array, 4, (void *)&vec);
  zgn_opengl_shader_program_set_uniform_float_vector(
      shader, location, 4, 1, &array);
  wl_array_release(&array);
}
