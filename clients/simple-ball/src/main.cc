#include "main.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zigen-shell-client-protocol.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "buffer.h"
#include "cuboid_window.h"
#include "obj.h"
#include "opengl.h"
#include "ray.h"

static void
seat_capabilities(void *data, struct zgn_seat *seat, uint32_t capability)
{
  (void)capability;
  struct app *app = (struct app *)data;
  app->ray = zgn_seat_get_ray(seat);
  add_ray_listener(app);
}

static const struct zgn_seat_listener seat_listener = {
    seat_capabilities,
};

static void
global_registry(void *data, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version)
{
  struct app *app = (struct app *)data;
  if (strcmp(interface, "wl_shm") == 0) {
    app->shm = (struct wl_shm *)wl_registry_bind(
        registry, id, &wl_shm_interface, version);
  } else if (strcmp(interface, "zgn_seat") == 0) {
    app->seat = (zgn_seat *)wl_registry_bind(
        registry, id, &zgn_seat_interface, version);
    zgn_seat_add_listener(app->seat, &seat_listener, app);
  } else if (strcmp(interface, "zgn_opengl") == 0) {
    app->opengl = (zgn_opengl *)wl_registry_bind(
        registry, id, &zgn_opengl_interface, version);
  } else if (strcmp(interface, "zgn_compositor") == 0) {
    app->compositor = (struct zgn_compositor *)wl_registry_bind(
        registry, id, &zgn_compositor_interface, version);
  } else if (strcmp(interface, "zgn_shell") == 0) {
    app->shell = (struct zgn_shell *)wl_registry_bind(
        registry, id, &zgn_shell_interface, version);
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

static void next_frame(struct app *app, uint32_t time);
static void
frame_callback_handler(
    void *data, struct wl_callback *callback, uint32_t callback_data)
{
  struct app *app = (struct app *)data;
  wl_callback_destroy(callback);
  next_frame(app, callback_data);
}

static const struct wl_callback_listener frame_callback_listener = {
    frame_callback_handler,
};

static void
next_frame(struct app *app, uint32_t time)
{
  (void)time;
  if (!app->stopped) {
    update_env(app->e);
    set_obj_uniform_variables(app->shader, app->e);
    zgn_opengl_component_attach_shader_program(app->component, app->shader);
  }

  // obj_update_texture_buffer_data(app->texture_buffer->data, time);

  // zgn_opengl_texture_attach_2d(app->texture, app->texture_buffer->buffer);
  // zgn_opengl_component_attach_texture(app->front_component, app->texture);

  wl_callback *frame_callback = zgn_virtual_object_frame(app->obj);
  wl_callback_add_listener(frame_callback, &frame_callback_listener, app);
  zgn_virtual_object_commit(app->obj);
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
  app.obj = virtual_object;

  struct zgn_opengl_shader_program *shader;
  shader = zgn_opengl_create_shader_program(app.opengl);
  app.shader = shader;

  // struct zgn_opengl_texture *texture;
  // texture = zgn_opengl_create_texture(app.opengl);
  // app.texture = texture;

  struct zgn_opengl_component *component;
  component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  app.component = component;

  // struct buffer *texture_data;
  // texture_data = create_buffer(app.shm, OBJ_TEXTURE_WIDTH * 4,
  //     OBJ_TEXTURE_HEIGHT, OBJ_TEXTURE_WIDTH, WL_SHM_FORMAT_ARGB8888);
  // app.texture_buffer = texture_data;

  size_t vertex_shader_len = strlen(vertex_shader);
  int vertex_shader_fd = get_shared_shader_fd(vertex_shader);

  assert(vertex_shader_fd != -1);

  zgn_opengl_shader_program_set_vertex_shader(
      shader, vertex_shader_fd, vertex_shader_len);

  int fragment_shader_fd = get_shared_shader_fd(fragment_shader);
  assert(fragment_shader_fd != -1);

  zgn_opengl_shader_program_set_fragment_shader(
      shader, fragment_shader_fd, strlen(fragment_shader));
  zgn_opengl_shader_program_link(shader);
  zgn_opengl_component_attach_shader_program(component, shader);

  zgn_opengl_component_set_count(component, OBJ_NUM_COMPONENTS);

  zgn_opengl_component_set_topology(component, ZGN_OPENGL_TOPOLOGY_TRIANGLES);
  zgn_opengl_component_add_vertex_attribute(component, 0, 3,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, p));
  zgn_opengl_component_add_vertex_attribute(component, 1, 2,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, uv));
  zgn_opengl_component_add_vertex_attribute(component, 2, 3,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, norm));

  // int texture_fragment_shader_fd =
  //     get_shared_shader_fd(texture_fragment_shader);
  // assert(texture_fragment_shader_fd != -1);

  // zgn_opengl_shader_program_set_fragment_shader(front_shader,
  //     texture_fragment_shader_fd, strlen(texture_fragment_shader));

  Vertex *points = get_points();
  ushort *indices = vertex_indices();

  opengl_component_add_ushort_element_array_buffer(
      app.opengl, component, app.shm, indices, OBJ_NUM_COMPONENTS);
  zgn_opengl_vertex_buffer *vertex_buffer =
      opengl_setup_vertex_buffer(app.opengl, app.shm, points, OBJ_NUM_POINTS);
  zgn_opengl_component_attach_vertex_buffer(component, vertex_buffer);

  add_cuboid_window(&app, 1.0);

  app.epoll_event.data.ptr = &app;
  app.epoll_event.events = EPOLLIN;
  app.epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  wl_display_roundtrip(app.display);

  app.e = setup_env();
  next_frame(&app, 0.);

  if (epoll_ctl(app.epoll_fd, EPOLL_CTL_ADD, wl_display_get_fd(app.display),
          &app.epoll_event) == -1) {
    fprintf(stderr, "Failed to add wayland event fd to epoll fd\n");
    return -1;
  }
  wl_display_flush(app.display);

  while (true) {
    struct epoll_event events[16];
    int epoll_count = epoll_wait(app.epoll_fd, events, 16, -1);

    for (int i = 0; i < epoll_count; i++) {
      assert(events[i].data.ptr == &app);
      while (wl_display_prepare_read(app.display) != 0) {
        if (errno != EAGAIN) return false;
        wl_display_dispatch_pending(app.display);
      }
      int ret = wl_display_flush(app.display);
      if (ret == -1) return EXIT_FAILURE;
      wl_display_read_events(app.display);
      wl_display_dispatch_pending(app.display);
      ret = wl_display_flush(app.display);
      if (ret == -1) return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
