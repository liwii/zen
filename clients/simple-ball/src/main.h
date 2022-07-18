#ifndef MAIN_H
#define MAIN_H

#include <sys/epoll.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zigen-shell-client-protocol.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

#endif  //  MAIN_H