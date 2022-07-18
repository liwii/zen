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
#include "opengl.h"
#include "ray.h"

const char *vertex_shader =
    "#version 410\n"
    "uniform mat4 zMVP;\n"
    "uniform mat4 rotate;\n"
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec2 v2UVcoordsIn;\n"
    "out vec2 v2UVcoords;\n"
    "void main()\n"
    "{\n"
    "  v2UVcoords = v2UVcoordsIn;\n"
    "  gl_Position = zMVP * rotate * position;\n"
    "}\n";

const char *fragment_shader =
    "#version 410 core\n"
    "uniform vec4 color;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  outputColor = color;\n"
    "}\n";

const char *texture_fragment_shader =
    "#version 410 core\n"
    "uniform sampler2D userTexture;\n"
    "in vec2 v2UVcoords;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  outputColor = texture(userTexture, v2UVcoords);\n"
    "}\n";

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

void
set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::mat4 mat)
{
  struct wl_array array;
  wl_array_init(&array);
  size_t size = sizeof(float) * 16;
  float *data = (float *)wl_array_add(&array, size);
  memcpy(data, &mat, size);
  zgn_opengl_shader_program_set_uniform_float_matrix(
      shader, location, 4, 4, false, 1, &array);
  wl_array_release(&array);
}

void
set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::vec4 vec)
{
  struct wl_array array;
  wl_array_init(&array);
  size_t size = sizeof(float) * 4;
  float *data = (float *)wl_array_add(&array, size);
  memcpy(data, &vec, size);
  zgn_opengl_shader_program_set_uniform_float_vector(
      shader, location, 4, 1, &array);
  wl_array_release(&array);
}

static void
next_frame(struct app *app, uint32_t time)
{
  if (!app->stopped) {
    app->delta_theta += (float)(rand() - RAND_MAX / 2) / (float)RAND_MAX;
    app->delta_theta = app->delta_theta > 10    ? 10
                       : app->delta_theta < -10 ? -10
                                                : app->delta_theta;
    app->delta_phi += (float)(rand() - RAND_MAX / 2) / (float)RAND_MAX;
    app->delta_phi = app->delta_phi > 10    ? 10
                     : app->delta_phi < -10 ? -10
                                            : app->delta_phi;
    app->rotate = glm::rotate(
        app->rotate, app->delta_theta * 0.001f, glm::vec3(1.0f, 0.0, 0.0f));
    app->rotate = glm::rotate(
        app->rotate, app->delta_phi * 0.001f, glm::vec3(0.0f, 1.0, 0.0f));
    set_shader_uniform_variable(app->frame_shader, "rotate",
        glm::toMat4(app->quaternion) * app->rotate);
    set_shader_uniform_variable(app->front_shader, "rotate",
        glm::toMat4(app->quaternion) * app->rotate);
    zgn_opengl_component_attach_shader_program(
        app->frame_component, app->frame_shader);
    zgn_opengl_component_attach_shader_program(
        app->front_component, app->front_shader);
  }

  (void)time;
  struct ColorBGRA *pixel = (struct ColorBGRA *)app->texture_buffer->data;
  glm::vec2 position = {1, 1};
  for (int x = 0; x < 256; x++) {
    for (int y = 0; y < 256; y++) {
      int r2 = (position.x * UINT8_MAX - x) * (position.x * UINT8_MAX - x) +
               (position.y * UINT8_MAX - y) * (position.y * UINT8_MAX - y);
      pixel->a = r2 < 1024 ? 0 : UINT8_MAX;
      pixel->r = x;
      pixel->g = y;
      pixel->b = UINT8_MAX;
      pixel++;
    }
  }

  zgn_opengl_texture_attach_2d(app->texture, app->texture_buffer->buffer);
  zgn_opengl_component_attach_texture(app->front_component, app->texture);

  // front_component_->Attach(texture_);
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

  struct zgn_opengl_vertex_buffer *vertex_buffer;
  vertex_buffer = zgn_opengl_create_vertex_buffer(app.opengl);

  struct zgn_opengl_shader_program *frame_shader, *front_shader;
  frame_shader = zgn_opengl_create_shader_program(app.opengl);
  front_shader = zgn_opengl_create_shader_program(app.opengl);
  app.frame_shader = frame_shader;
  app.front_shader = front_shader;

  struct zgn_opengl_texture *texture;
  texture = zgn_opengl_create_texture(app.opengl);

  app.texture = texture;

  struct zgn_opengl_component *frame_component, *front_component;
  frame_component =
      zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  front_component =
      zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  app.front_component = front_component;
  app.frame_component = frame_component;

  struct zgn_opengl_element_array_buffer *frame_element_array,
      *front_element_array;
  frame_element_array = zgn_opengl_create_element_array_buffer(app.opengl);
  front_element_array = zgn_opengl_create_element_array_buffer(app.opengl);

  struct buffer *vertex_buffer_data, *frame_element_array_data,
      *front_element_array_data, *texture_data;

  vertex_buffer_data = create_buffer(&app, sizeof(Vertex) * 8);
  frame_element_array_data = create_buffer(&app, sizeof(u_short) * 24);
  front_element_array_data = create_buffer(&app, sizeof(u_short) * 24);
  texture_data = create_buffer(&app, 256 * 4, 256, 256, WL_SHM_FORMAT_ARGB8888);

  app.texture_buffer = texture_data;

  size_t vertex_shader_len = strlen(vertex_shader);
  int vertex_shader_fd = get_shared_shader_fd(vertex_shader);
  if (vertex_shader_fd == -1) {
    return EXIT_FAILURE;
  }

  zgn_opengl_shader_program_set_vertex_shader(
      frame_shader, vertex_shader_fd, vertex_shader_len);
  zgn_opengl_shader_program_set_vertex_shader(
      front_shader, vertex_shader_fd, vertex_shader_len);

  int fragment_shader_fd = get_shared_shader_fd(fragment_shader);
  if (fragment_shader_fd == -1) {
    return EXIT_FAILURE;
  }
  zgn_opengl_shader_program_set_fragment_shader(
      frame_shader, fragment_shader_fd, strlen(fragment_shader));
  zgn_opengl_shader_program_link(frame_shader);
  set_shader_uniform_variable(
      frame_shader, "color", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
  glm::mat4 rotate(1.0f);
  app.rotate = rotate;
  set_shader_uniform_variable(frame_shader, "rotate", app.rotate);
  zgn_opengl_component_attach_shader_program(frame_component, frame_shader);
  zgn_opengl_component_set_count(frame_component, 24);
  zgn_opengl_component_set_topology(frame_component, ZGN_OPENGL_TOPOLOGY_LINES);
  zgn_opengl_component_add_vertex_attribute(frame_component, 0, 3,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, p));

  int texture_fragment_shader_fd =
      get_shared_shader_fd(texture_fragment_shader);

  if (texture_fragment_shader_fd == -1) {
    return EXIT_FAILURE;
  }

  zgn_opengl_shader_program_set_fragment_shader(front_shader,
      texture_fragment_shader_fd, strlen(texture_fragment_shader));
  zgn_opengl_shader_program_link(front_shader);
  set_shader_uniform_variable(front_shader, "rotate", app.rotate);
  zgn_opengl_component_attach_shader_program(front_component, front_shader);
  zgn_opengl_component_set_count(front_component, 6);
  zgn_opengl_component_set_topology(
      front_component, ZGN_OPENGL_TOPOLOGY_TRIANGLES);
  zgn_opengl_component_add_vertex_attribute(front_component, 0, 3,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, p));
  zgn_opengl_component_add_vertex_attribute(front_component, 1, 2,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex),
      offsetof(Vertex, u));

  Vertex points[8];

  float length = 0.2f;
  int i = 0;
  for (int x = -1; x < 2; x += 2) {
    for (int y = -1; y < 2; y += 2) {
      for (int z = -1; z < 2; z += 2) {
        points[i].p.x = length * x;
        points[i].p.y = length * y;
        points[i].p.z = length * z;
        points[i].u = x < 0 ? 0 : 1;
        points[i].v = y < 0 ? 0 : 1;
        i++;
      }
    }
  }

  u_short frame_indices[24] = {
      0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7, 0, 2, 1, 3, 4, 6, 5, 7};
  u_short *frame_array_indices = (u_short *)frame_element_array_data->data;
  memcpy(frame_array_indices, frame_indices, sizeof(frame_indices));
  zgn_opengl_element_array_buffer_attach(frame_element_array,
      frame_element_array_data->buffer,
      ZGN_OPENGL_ELEMENT_ARRAY_INDICES_TYPE_UNSIGNED_SHORT);
  zgn_opengl_component_attach_element_array_buffer(
      frame_component, frame_element_array);

  u_short front_indices[6] = {1, 7, 3, 1, 7, 5};
  u_short *front_array_indices = (u_short *)front_element_array_data->data;
  memcpy(front_array_indices, front_indices, sizeof(front_indices));
  zgn_opengl_element_array_buffer_attach(front_element_array,
      front_element_array_data->buffer,
      ZGN_OPENGL_ELEMENT_ARRAY_INDICES_TYPE_UNSIGNED_SHORT);
  zgn_opengl_component_attach_element_array_buffer(
      front_component, front_element_array);

  Vertex *vertices = (Vertex *)vertex_buffer_data->data;
  memcpy(vertices, points, sizeof(Vertex) * 8);
  zgn_opengl_vertex_buffer_attach(vertex_buffer, vertex_buffer_data->buffer);
  zgn_opengl_component_attach_vertex_buffer(frame_component, vertex_buffer);
  zgn_opengl_component_attach_vertex_buffer(front_component, vertex_buffer);
  app.delta_theta = 0.;
  app.delta_phi = 0.;

  add_cuboid_window(&app, length);

  app.epoll_event.data.ptr = &app;
  app.epoll_event.events = EPOLLIN;
  app.epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  wl_display_roundtrip(app.display);
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
