#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>

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
  wl_display_roundtrip(app.display);


  struct zgn_virtual_object *virtual_object;
  virtual_object = zgn_compositor_create_virtual_object(app.compositor);

  struct zgn_opengl_vertex_buffer *vertex_buffer;
  vertex_buffer = zgn_opengl_create_vertex_buffer(app.opengl);

  struct zgn_opengl_component *component;
  component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);

  (void)vertex_buffer;
  (void)component;

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
