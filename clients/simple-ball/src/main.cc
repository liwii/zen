#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zigen-shell-client-protocol.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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
  struct zgn_shell *shell;
  struct zgn_virtual_object *obj;
  struct zgn_virtual_object *ray_focus_obj;
  struct zgn_opengl_texture *texture;
  struct buffer *texture_buffer;
  struct zgn_opengl_component *front_component;
  struct zgn_opengl_component *frame_component;
  struct zgn_opengl_shader_program *front_shader;
  struct zgn_opengl_shader_program *frame_shader;
  float delta_theta;
  float delta_phi;
  glm::mat4 rotate;
  glm::quat quaternion;
  struct zgn_seat *seat;
  struct zgn_ray *ray;
  bool stopped;
  struct epoll_event epoll_event;
  int epoll_fd;
};

struct ColorBGRA {
  uint8_t b, g, r, a;
};

struct buffer {
  off_t size;
  int fd;
  void *data;
  struct wl_shm_pool *pool;
  struct wl_buffer *buffer;
};

static void
ray_enter(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *origin,
    struct wl_array *direction)
{
  struct app* app = (struct app *) data;
  app->ray_focus_obj = virtual_object;
  (void)ray;
  (void)serial;
  (void)origin;
  (void)direction;
}

static void
ray_leave(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  struct app* app = (struct app *) data;
  app->ray_focus_obj = NULL;
  (void)ray;
  (void)serial;
  (void)virtual_object;
}

static void
ray_motion(void *data, struct zgn_ray *ray, uint32_t time,
    struct wl_array *origin, struct wl_array *direction)
{
  (void)data;
  (void)ray;
  (void)time;
  (void)origin;
  (void)direction;
}

static void
ray_button(void *data, struct zgn_ray *ray, uint32_t serial, uint32_t time,
    uint32_t button, uint32_t state)
{
  struct app* app = (struct app *) data;
  if (app->ray_focus_obj == NULL) return;
  if(state == ZGN_RAY_BUTTON_STATE_PRESSED) {
    app->stopped = !app->stopped;
  }
  (void)ray;
  (void)serial;
  (void)time;
  (void)button;
  (void)state;
}


static const struct zgn_ray_listener ray_listener = {
  ray_enter,
  ray_leave,
  ray_motion,
  ray_button,
};

static void
seat_capabilities(void *data, struct zgn_seat *seat, uint32_t capability)
{
  (void) capability;
  struct app* app = (struct app*) data;
  app->ray = zgn_seat_get_ray(seat);
  zgn_ray_add_listener(app->ray, &ray_listener, app);
  
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
static void frame_callback_handler(
  void *data, struct wl_callback *callback, uint32_t callback_data
) {
  struct app *app = (struct app *) data;
  wl_callback_destroy(callback);
  next_frame(app, callback_data);
}

static const struct wl_callback_listener frame_callback_listener = {
  frame_callback_handler,
};

void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader, const char *location, glm::mat4 mat) {
  struct wl_array array;
  wl_array_init(&array);
  size_t size = sizeof(float) * 16;
  float *data = (float *)wl_array_add(&array, size);
  memcpy(data, &mat, size);
  zgn_opengl_shader_program_set_uniform_float_matrix(shader, location, 4, 4, false, 1, &array);
  wl_array_release(&array);
}

void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader, const char *location, glm::vec4 vec) {
  struct wl_array array;
  wl_array_init(&array);
  size_t size = sizeof(float) * 4;
  float *data = (float *)wl_array_add(&array, size);
  memcpy(data, &vec, size);
  zgn_opengl_shader_program_set_uniform_float_vector(shader, location, 4, 1, &array);
  wl_array_release(&array);
}


static void next_frame(struct app *app, uint32_t time) {
  if(!app->stopped) {
    app->delta_theta += (float)(rand() - RAND_MAX / 2) / (float)RAND_MAX;
    app->delta_theta = app->delta_theta > 10    ? 10
                  : app->delta_theta < -10 ? -10
                                        : app->delta_theta;
    app->delta_phi += (float)(rand() - RAND_MAX / 2) / (float)RAND_MAX;
    app->delta_phi = app->delta_phi > 10 ? 10 : app->delta_phi < -10 ? -10 : app->delta_phi;
    app->rotate = glm::rotate(app->rotate, app->delta_theta * 0.001f, glm::vec3(1.0f, 0.0, 0.0f));
    app->rotate = glm::rotate(app->rotate, app->delta_phi * 0.001f, glm::vec3(0.0f, 1.0, 0.0f));
    set_shader_uniform_variable(app->frame_shader, "rotate", glm::toMat4(app->quaternion) * app->rotate);
    set_shader_uniform_variable(app->front_shader, "rotate", glm::toMat4(app->quaternion) * app->rotate);
    zgn_opengl_component_attach_shader_program(app->frame_component, app->frame_shader);
    zgn_opengl_component_attach_shader_program(app->front_component, app->front_shader);
  }


  (void) time;
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

  //front_component_->Attach(texture_);
  wl_callback *frame_callback = zgn_virtual_object_frame(app->obj);
  wl_callback_add_listener(frame_callback, &frame_callback_listener, app);
  zgn_virtual_object_commit(app->obj);
}


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

static struct buffer* create_buffer(app *app, int32_t stride, int32_t height, int32_t width, enum wl_shm_format format) {
  struct buffer *buf = (struct buffer*) malloc(sizeof(struct buffer));

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

  buf->pool = wl_shm_create_pool(app->shm, buf->fd, size);
  buf->buffer = wl_shm_pool_create_buffer(buf->pool, 0, width, height, stride, format);

  return buf;
}

static void cuboid_window_configure(void *data, struct zgn_cuboid_window *zgn_cuboid_window, uint32_t serial, struct wl_array *half_size_array, struct wl_array *quaternion_array) {
  (void) data;
  (void) half_size_array;
  (void) quaternion_array;
  zgn_cuboid_window_ack_configure(zgn_cuboid_window, serial);
}

static void cuboid_window_moved(void *data, struct zgn_cuboid_window *zgn_cuboid_window, struct wl_array *face_direction_array) {
  (void) data;
  (void) zgn_cuboid_window;
  (void) face_direction_array;
}

static const struct zgn_cuboid_window_listener cuboid_window_listener = {
  cuboid_window_configure,
  cuboid_window_moved,
};



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
  frame_component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  front_component = zgn_opengl_create_opengl_component(app.opengl, virtual_object);
  app.front_component = front_component;
  app.frame_component = frame_component;

  struct zgn_opengl_element_array_buffer *frame_element_array, *front_element_array;
  frame_element_array = zgn_opengl_create_element_array_buffer(app.opengl);
  front_element_array = zgn_opengl_create_element_array_buffer(app.opengl);

  struct buffer *vertex_buffer_data, *frame_element_array_data, *front_element_array_data, *texture_data;

  vertex_buffer_data = create_buffer(&app, sizeof(Vertex) * 8);
  frame_element_array_data = create_buffer(&app, sizeof(u_short) * 24);
  front_element_array_data = create_buffer(&app, sizeof(u_short) * 24);
  texture_data = create_buffer(&app, 256 * 4, 256, 256, WL_SHM_FORMAT_ARGB8888);
  
  app.texture_buffer = texture_data;

  size_t vertex_shader_len = strlen(vertex_shader);
  int vertex_shader_fd = create_shared_fd(vertex_shader_len);
  if(vertex_shader_fd == -1) {
    return EXIT_FAILURE;
  }

  void *vertex_shader_data = mmap(NULL, vertex_shader_len, PROT_WRITE, MAP_SHARED, vertex_shader_fd, 0);
  if(vertex_shader_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    return EXIT_FAILURE;
  }
  memcpy(vertex_shader_data, vertex_shader, vertex_shader_len);
  munmap(vertex_shader_data, strlen(vertex_shader));
  
  zgn_opengl_shader_program_set_vertex_shader(frame_shader, vertex_shader_fd, vertex_shader_len);
  zgn_opengl_shader_program_set_vertex_shader(front_shader, vertex_shader_fd, vertex_shader_len);

  size_t fragment_shader_len = strlen(fragment_shader);
  int fragment_shader_fd = create_shared_fd(fragment_shader_len);
  if(fragment_shader_fd == -1) {
    return EXIT_FAILURE;
  }

  void *fragment_shader_data = mmap(NULL, fragment_shader_len, PROT_WRITE, MAP_SHARED, fragment_shader_fd, 0);
  if(fragment_shader_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    return EXIT_FAILURE;
  }
  memcpy(fragment_shader_data, fragment_shader, fragment_shader_len);
  munmap(fragment_shader_data, fragment_shader_len);
  
  zgn_opengl_shader_program_set_fragment_shader(frame_shader, fragment_shader_fd, fragment_shader_len);
  zgn_opengl_shader_program_link(frame_shader);
  set_shader_uniform_variable(frame_shader, "color", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
  glm::mat4 rotate(1.0f);
  app.rotate = rotate;
  set_shader_uniform_variable(frame_shader, "rotate", app.rotate);
  zgn_opengl_component_attach_shader_program(frame_component, frame_shader);
  zgn_opengl_component_set_count(frame_component, 24);
  zgn_opengl_component_set_topology(frame_component, ZGN_OPENGL_TOPOLOGY_LINES);
  zgn_opengl_component_add_vertex_attribute(frame_component, 0, 3, ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex), offsetof(Vertex, p));

  size_t texture_fragment_shader_len = strlen(texture_fragment_shader);
  int texture_fragment_shader_fd = create_shared_fd(texture_fragment_shader_len);
  if(texture_fragment_shader_fd == -1) {
    return EXIT_FAILURE;
  }

  void *texture_fragment_shader_data = mmap(NULL, texture_fragment_shader_len, PROT_WRITE, MAP_SHARED, texture_fragment_shader_fd, 0);
  if(texture_fragment_shader_data == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    return EXIT_FAILURE;
  }
  memcpy(texture_fragment_shader_data, texture_fragment_shader, texture_fragment_shader_len);
  munmap(texture_fragment_shader_data, texture_fragment_shader_len);

  zgn_opengl_shader_program_set_fragment_shader(front_shader, texture_fragment_shader_fd, texture_fragment_shader_len);
  zgn_opengl_shader_program_link(front_shader);
  set_shader_uniform_variable(front_shader, "rotate", app.rotate);
  zgn_opengl_component_attach_shader_program(front_component, front_shader);
  zgn_opengl_component_set_count(front_component, 6);
  zgn_opengl_component_set_topology(front_component, ZGN_OPENGL_TOPOLOGY_TRIANGLES);
  zgn_opengl_component_add_vertex_attribute(front_component, 0, 3, ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex), offsetof(Vertex, p));
  zgn_opengl_component_add_vertex_attribute(front_component, 1, 2, ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(Vertex), offsetof(Vertex, u));

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
  zgn_opengl_element_array_buffer_attach(frame_element_array, frame_element_array_data->buffer, ZGN_OPENGL_ELEMENT_ARRAY_INDICES_TYPE_UNSIGNED_SHORT);
  zgn_opengl_component_attach_element_array_buffer(frame_component, frame_element_array);

  u_short front_indices[6] = {1, 7, 3, 1, 7, 5};
  u_short *front_array_indices = (u_short *)front_element_array_data->data;
  memcpy(front_array_indices, front_indices, sizeof(front_indices));
  zgn_opengl_element_array_buffer_attach(front_element_array, front_element_array_data->buffer, ZGN_OPENGL_ELEMENT_ARRAY_INDICES_TYPE_UNSIGNED_SHORT);
  zgn_opengl_component_attach_element_array_buffer(front_component, front_element_array);

  Vertex *vertices = (Vertex *)vertex_buffer_data->data;
  memcpy(vertices, points, sizeof(Vertex) * 8);
  zgn_opengl_vertex_buffer_attach(vertex_buffer, vertex_buffer_data->buffer);
  zgn_opengl_component_attach_vertex_buffer(frame_component, vertex_buffer);
  zgn_opengl_component_attach_vertex_buffer(front_component, vertex_buffer);
  app.delta_theta = 0.;
  app.delta_phi = 0.;

  glm::vec3 half_size(length * 1.8);
  app.quaternion = glm::quat();


  struct wl_array half_size_array, quaternion_array;

  wl_array_init(&half_size_array);
  wl_array_init(&quaternion_array);

  size_t half_size_size = sizeof(glm::vec3);
  float *half_size_data = (float *)wl_array_add(&half_size_array, half_size_size);
  memcpy(half_size_data, &half_size, half_size_size);

  size_t quaternion_size = sizeof(glm::quat);
  float *quaternion_data = (float *)wl_array_add(&quaternion_array, quaternion_size);
  memcpy(quaternion_data, &app.quaternion, quaternion_size);

  struct zgn_cuboid_window* cuboid_window = zgn_shell_get_cuboid_window(app.shell, virtual_object, &half_size_array, &quaternion_array);

  wl_array_release(&half_size_array);

  zgn_cuboid_window_add_listener(cuboid_window, &cuboid_window_listener, cuboid_window);

  app.epoll_event.data.ptr = &app;
  app.epoll_event.events = EPOLLIN;
  app.epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  wl_display_roundtrip(app.display);
  next_frame(&app, 0.);

  if(epoll_ctl(app.epoll_fd, EPOLL_CTL_ADD, wl_display_get_fd(app.display), &app.epoll_event) == -1) {
    fprintf(stderr, "Failed to add wayland event fd to epoll fd\n");
    return -1;
  }
  wl_display_flush(app.display);

  while(true) {
    struct epoll_event events[16];
    int epoll_count = epoll_wait(app.epoll_fd, events, 16, -1);

    for(int i = 0; i < epoll_count; i++) {
      assert(events[i].data.ptr == &app);
      while(wl_display_prepare_read(app.display) != 0) {
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
