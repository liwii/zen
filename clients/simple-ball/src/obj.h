#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>

#include "opengl.h"

const char *const vertex_shader =
    "#version 450 core\n"
    "\n"
    "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
    "layout(location = 1) in vec2 vertexUV;\n"
    "layout(location = 2) in vec3 vertexNorm;\n"
    "\n"
    "out vec2 UV;\n"
    "out vec3 norm;\n"
    "out vec3 fragPos;\n"
    "\n"
    "uniform mat4 zModel;\n"
    "uniform mat4 zView;\n"
    "uniform mat4 zProjection;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = zProjection * zView * zModel * "
    "vec4(vertexPosition_modelspace, 1);\n"
    "\n"
    "    UV = vertexUV;\n"
    "    norm = mat3(transpose(inverse(zModel))) * vertexNorm;\n"
    "    fragPos = vec3(zModel * vec4(vertexPosition_modelspace, 1.0));\n"
    "}\n";

const char *const fragment_shader =
    "#version 450 core\n"
    "\n"
    "in vec2 UV;\n"
    "in vec3 norm;\n"
    "in vec3 fragPos;\n"
    "\n"
    "out vec4 color;\n"
    "\n"
    "uniform vec3 LightPos;\n"
    "uniform vec3 CameraPos;\n"
    //"uniform sampler2D myTextureSampler;\n"
    "\n"
    "void main() {\n"
    "    vec3 n = normalize(norm);\n"
    "    vec3 lightDir = normalize(LightPos - fragPos);\n"
    "    vec3 lightColor = vec3(1.0, 0.8, 0.8);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor * 1.0;\n"
    "\n"
    "    vec3 viewDir = normalize(CameraPos - fragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = 0.3 * spec * lightColor;\n"
    "\n"
    //"    vec3 objectColor = texture(myTextureSampler, UV).rgb;\n"
    "    float ambient = 0.2;\n"
    "    color = vec4((ambient + diffuse + specular) * lightColor, 1.0);\n"
    //" * texture(myTextureSampler, UV).rgb;\n"
    "}\n"
    "\n";

const int OBJ_NUM_POINTS_R = 100;
const int OBJ_NUM_POINTS = OBJ_NUM_POINTS_R * (OBJ_NUM_POINTS_R / 2 + 1);
const int OBJ_NUM_COMPONENTS =
    6 * OBJ_NUM_POINTS_R * (OBJ_NUM_POINTS_R / 2 - 1);

struct Vertex {
  glm::vec3 p;
  glm::vec2 uv;
  glm::vec3 norm;
};

struct Env {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::vec3 camera;
  glm::vec3 light;
};

Vertex *get_points();
u_short *vertex_indices();

Env *setup_env();

void update_env(Env *e);
void set_obj_uniform_variables(zgn_opengl_shader_program *shader, Env *e);

zgn_opengl_vertex_buffer *opengl_setup_vertex_buffer(
    zgn_opengl *opengl, wl_shm *shm, Vertex *points, uint points_len);
#endif  //  OBJ_H
