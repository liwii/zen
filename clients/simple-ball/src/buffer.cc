#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

int
create_shared_fd(off_t size)
{
  const char *socket_name = "zen-simple-ball";
  int fd = memfd_create(socket_name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (fd < 0) {
    fprintf(stderr, "Failed to create fd\n");
    return -1;
  }

  unlink(socket_name);

  if (ftruncate(fd, size) < 0) {
    close(fd);
    fprintf(stderr, "Failed to create fd\n");
    return -1;
  }
  return fd;
}

buffer *
create_buffer(wl_shm *shm, off_t size)
{
  buffer *buf = (buffer *)malloc(sizeof(buffer));

  buf->fd = create_shared_fd(size);
  if (buf->fd == -1) {
    fprintf(stderr, "Failed to create buffer\n");
    free(buf);
    return NULL;
  }

  buf->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);

  if (buf->data == MAP_FAILED) {
    close(buf->fd);
    free(buf);
    return NULL;
  }

  buf->pool = wl_shm_create_pool(shm, buf->fd, size);
  buf->buffer = wl_shm_pool_create_buffer(buf->pool, 0, size, 1, size, 0);

  return buf;
}

buffer *
create_buffer(wl_shm *shm, int32_t stride, int32_t height, int32_t width,
    enum wl_shm_format format)
{
  buffer *buf = (buffer *)malloc(sizeof(buffer));

  size_t size = stride * height;
  buf->fd = create_shared_fd(size);
  if (buf->fd == -1) {
    fprintf(stderr, "Failed to create buffer\n");
    free(buf);
    return NULL;
  }

  buf->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, buf->fd, 0);

  if (buf->data == MAP_FAILED) {
    close(buf->fd);
    free(buf);
    return NULL;
  }

  buf->pool = wl_shm_create_pool(shm, buf->fd, size);
  buf->buffer =
      wl_shm_pool_create_buffer(buf->pool, 0, width, height, stride, format);

  return buf;
}

void
update_texture_buffer(buffer *buf, Texture *t)
{
  // t holds the bgr texture data.
  // This function refills buf->data with the bgra value
  // of the texture data.
  ColorBGRA *pixel = (ColorBGRA *)buf->data;
  for (unsigned int i = 0; i < t->imageSize / 3; ++i) {
    pixel->a = UINT8_MAX;
    pixel->b = t->data[3 * i];
    pixel->g = t->data[3 * i + 1];
    pixel->r = t->data[3 * i + 2];
    pixel++;
  }
}