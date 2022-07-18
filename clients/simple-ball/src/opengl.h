#ifndef OPENGL_H
#define OPENGL_H

#include <wayland-client.h>
#include <zigen-opengl-client-protocol.h>

#include <glm/glm.hpp>

int get_shared_shader_fd(const char *shader);

void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::mat4 mat);
void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::vec4 vec);

void opengl_component_add_ushort_element_array_buffer(zgn_opengl *opengl,
    zgn_opengl_component *component, wl_shm *shm, u_short *indices,
    uint indices_len);
#endif  //  OPENGL_H