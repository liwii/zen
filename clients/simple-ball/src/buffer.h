#include <sys/types.h>
#include <wayland-client.h>

struct buffer {
  off_t size;
  int fd;
  void *data;
  struct wl_shm_pool *pool;
  struct wl_buffer *buffer;
};

buffer *create_buffer(wl_shm *shm, off_t size);

buffer *create_buffer(wl_shm *shm, int32_t stride, int32_t height,
    int32_t width, enum wl_shm_format format);

int create_shared_fd(off_t size);