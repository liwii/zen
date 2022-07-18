#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "buffer.h"

int
get_shared_shader_fd(const char* shader)
{
  size_t shader_len = strlen(shader);
  int shader_fd = create_shared_fd(shader_len);
  if (shader_fd == -1) {
    return -1;
  }

  void* shader_data =
      mmap(NULL, shader_len, PROT_WRITE, MAP_SHARED, shader_fd, 0);
  if (shader_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    return EXIT_FAILURE;
  }
  memcpy(shader_data, shader, shader_len);
  munmap(shader_data, shader_len);

  return shader_fd;
};