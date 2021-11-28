#ifndef ZEN_CLIENT_ZUKOU_H
#define ZEN_CLIENT_ZUKOU_H

#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zigen-shell-client-protocol.h>

#include <glm/glm.hpp>

namespace zukou {
class App
{
 public:
  App();
  ~App();

  bool Connect(const char *socket);
  void GlobalRegistryHandler(struct wl_registry *registry, uint32_t id,
      const char *interface, uint32_t version);
  void GlobalRegistryRemover(struct wl_registry *registry, uint32_t id);
  void ShmFormat(struct wl_shm *wl_shm, uint32_t format);
  bool Run();
  void Terminate();
  inline struct wl_display *display();
  inline struct wl_registry *registry();
  inline struct zgn_compositor *compositor();
  inline struct zgn_shell *shell();
  inline struct wl_shm *shm();
  inline struct zgn_opengl *opengl();

 private:
  struct wl_display *display_;
  struct wl_registry *registry_;
  struct zgn_compositor *compositor_;
  struct zgn_shell *shell_;
  struct wl_shm *shm_;
  struct zgn_opengl *opengl_;
  bool running_ = false;
};

inline struct wl_display *
App::display()
{
  return display_;
}

inline struct wl_registry *
App::registry()
{
  return registry_;
}

inline struct zgn_compositor *
App::compositor()
{
  return compositor_;
}

inline struct zgn_shell *
App::shell()
{
  return shell_;
}

inline struct wl_shm *
App::shm()
{
  return shm_;
}

inline struct zgn_opengl *
App::opengl()
{
  return opengl_;
}

class Buffer
{
 public:
  Buffer(App *app, size_t size);
  ~Buffer();

 protected:
  size_t size_;
  int fd_;
  void *data_;
  struct wl_shm_pool *pool_;
  struct wl_buffer *wl_buffer_;
};

class VirtualObject
{
 public:
  VirtualObject(App *app);
  virtual ~VirtualObject();
  void NextFrame();
  void Commit();
  virtual void Frame(uint32_t time);
  inline App *app();
  inline struct zgn_virtual_object *virtual_object();
  inline struct wl_callback *frame_callback();

 protected:
  App *app_;
  struct zgn_virtual_object *virtual_object_;
  struct wl_callback *frame_callback_;
};

inline App *
VirtualObject::app()
{
  return app_;
}

inline struct zgn_virtual_object *
VirtualObject::virtual_object()
{
  return virtual_object_;
}

inline struct wl_callback *
VirtualObject::frame_callback()
{
  return frame_callback_;
}

class CuboidWindow : public VirtualObject
{
 public:
  CuboidWindow(App *app, glm::vec3 half_size);
  virtual ~CuboidWindow();
  virtual void Configure(uint32_t serial, glm::vec3 half_size);
  struct zgn_cuboid_window *cuboid_window();
  glm::vec3 half_size();

 protected:
  struct zgn_cuboid_window *cuboid_window_;
  glm::vec3 half_size_;
};

inline struct zgn_cuboid_window *
CuboidWindow::cuboid_window()
{
  return cuboid_window_;
}

inline glm::vec3
CuboidWindow::half_size()
{
  return half_size_;
}

class OpenGLVertexBuffer : public Buffer
{
 public:
  OpenGLVertexBuffer(App *app, size_t size);
  ~OpenGLVertexBuffer();
  void BufferUpdated();
  inline App *app();
  inline struct zgn_opengl_vertex_buffer *vertex_buffer();
  inline void *data();

 private:
  App *app_;
  struct zgn_opengl_vertex_buffer *vertex_buffer_;
};

inline App *
OpenGLVertexBuffer::app()
{
  return app_;
}

inline struct zgn_opengl_vertex_buffer *
OpenGLVertexBuffer::vertex_buffer()
{
  return vertex_buffer_;
}

inline void *
OpenGLVertexBuffer::data()
{
  return data_;
}

class OpenGLShaderProgram
{
 public:
  OpenGLShaderProgram(App *app);
  ~OpenGLShaderProgram();
  bool SetVertexShader(const char *source, size_t len);
  bool SetFragmentShader(const char *source, size_t len);
  void Link();
  inline struct zgn_opengl_shader_program *shader();

 private:
  struct zgn_opengl_shader_program *shader_;
  int vertex_shader_fd_;
  int fragment_shader_fd_;
};

inline struct zgn_opengl_shader_program *
OpenGLShaderProgram::shader()
{
  return shader_;
}

class OpenGLComponent
{
 public:
  OpenGLComponent(App *app, VirtualObject *virtual_object);
  ~OpenGLComponent();
  void Attach(OpenGLVertexBuffer *vertex_buffer);
  void Attach(OpenGLShaderProgram *shader_program);
  void SetMin(uint32_t min);
  void SetCount(uint32_t count);
  void SetTopology(enum zgn_opengl_topology topology);
  void AddVertexAttribute(uint32_t index, uint32_t size,
      enum zgn_opengl_vertex_attribute_type type, uint32_t normalized,
      uint32_t stride, uint32_t pointer);
  inline App *app();
  inline struct zgn_opengl_component *component();

 private:
  App *app_;
  struct zgn_opengl_component *component_;
};

inline App *
OpenGLComponent::app()
{
  return app_;
}

inline struct zgn_opengl_component *
OpenGLComponent::component()
{
  return component_;
}

glm::vec3 glm_vec3_from_wl_array(struct wl_array *array);

void glm_vec3_to_wl_array(glm::vec3 v, struct wl_array *array);

}  // namespace zukou

#endif  //  ZEN_CLIENT_ZUKOU_H
