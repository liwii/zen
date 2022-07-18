#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>

#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 p;
  float u, v;
};

struct app {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_shm *shm;
  struct zgn_compositor *compositor;
  struct zgn_opengl *opengl;
};


static void
global_registry(void *data, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version)
{
  struct app *app = (struct app *)data;
  if (strcmp(interface, "wl_shm") == 0) {
    app->shm = (struct wl_shm *)wl_registry_bind(
        registry, id, &wl_shm_interface, version);
  } else if (strcmp(interface, "zgn_opengl") == 0) {
    app->opengl = (zgn_opengl *)wl_registry_bind(
        registry, id, &zgn_opengl_interface, version);
  } else if (strcmp(interface, "zgn_compositor") == 0) {
    app->compositor = (struct zgn_compositor *)wl_registry_bind(
        registry, id, &zgn_compositor_interface, version);
  }
}

static void
global_registry_remove(void *data, struct wl_registry *registry, uint32_t id)
{
  struct app *app = (struct app *)data;
  (void)app;
  (void)registry;
  (void)id;
}
const struct wl_registry_listener registry_listener = {
    global_registry,
    global_registry_remove,
};

static void next_frame(struct zgn_virtual_object *obj);
static void frame_callback_handler(
  void *data, struct wl_callback *callback, uint32_t callback_data
) {
  (void)callback_data;
  struct zgn_virtual_object *obj = (struct zgn_virtual_object *) data;
  wl_callback_destroy(callback);
  next_frame(obj);
}

static const struct wl_callback_listener frame_callback_listener = {
  frame_callback_handler,
};

static void next_frame(struct zgn_virtual_object *obj) {
  wl_callback *frame_callback = zgn_virtual_object_frame(obj);
  wl_callback_add_listener(frame_callback, &frame_callback_listener, obj);
  zgn_virtual_object_commit(obj);
}

struct buffer {
  off_t size;
  int fd;
  void *data;
  struct wl_shm_pool *pool;
  struct wl_buffer *buffer;
};

static int create_shared_fd(off_t size) {
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

static struct buffer* create_buffer(app *app, off_t size) {
  struct buffer *buf = (struct buffer*) malloc(sizeof(struct buffer));

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

  buf->pool = wl_shm_create_pool(app->shm, buf->fd, size);
  buf->buffer = wl_shm_pool_create_buffer(buf->pool, 0, size, 1, size, 0);

  return buf;
}


int
main(void)
{
  struct app app;
  app.display = wl_display_connect("zigen-0");

  if (app.display == nullptr) {
    fprintf(stderr, "Cannot connect to the socket: zigen-0\n");
    return EXIT_FAILURE;
  }

  app.registry = wl_display_get_registry(app.display);
  if (app.registry == nullptr) {
    fprintf(stderr, "Cannot get the registry\n");
    return EXIT_FAILURE;
  }
  wl_registry_add_listener(app.registry, &registry_listener, &app);
  wl_display_dispatch(app.display);


  struct zgn_virtual_object *virtual_object;
  virtual_object = zgn_compositor_create_virtual_object(app.compositor);

  struct zgn_opengl_vertex_buffer *vertex_buffer;
  vertex_buffer = zgn_opengl_create_vertex_buffer(app.opengl);

  struct zgn_opengl_shader_program *frame_shader, *front_shader;
  frame_shader = zgn_opengl_create_shader_program(app.opengl);
  front_shader = zgn_opengl_create_shader_program(app.opengl);

  struct zgn_opengl_texture *texture;
  texture = zgn_opengl_create_texture(app.opengl);

  struct zgn_opengl_component *frame_component, *front_component;
  frame_component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  front_component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);

  struct zgn_opengl_element_array_buffer *frame_element_array, *front_element_array;
  frame_element_array = zgn_opengl_create_element_array_buffer(app.opengl);
  front_element_array = zgn_opengl_create_element_array_buffer(app.opengl);

  struct buffer *vertex_buffer_data, *frame_element_array_data, *front_element_array_data;

  vertex_buffer_data = create_buffer(&app, sizeof(Vertex) * 8);
  frame_element_array_data = create_buffer(&app, sizeof(u_short) * 24);
  front_element_array_data = create_buffer(&app, sizeof(u_short) * 24);

  (void)vertex_buffer;
  (void)frame_component;
  (void)front_component;
  (void)frame_element_array;
  (void)front_element_array;
  (void)vertex_buffer_data;
  (void)frame_element_array_data;
  (void)front_element_array_data;
  (void)frame_shader;
  (void)front_shader;
  (void)texture;
  

  wl_display_roundtrip(app.display);
  next_frame(virtual_object);

  while(true) {
    while(wl_display_prepare_read(app.display) != 0) {
      if (errno != EAGAIN) return false;
      wl_display_dispatch_pending(app.display);
    }
    int ret = wl_display_flush(app.display);
    if (ret == -1) return EXIT_FAILURE;
    wl_display_read_events(app.display);
    wl_display_dispatch_pending(app.display);
  }

  return EXIT_SUCCESS;
}
