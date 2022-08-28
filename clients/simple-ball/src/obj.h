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
    "out vec3 cameraPos;\n"
    "out vec3 lightPos;\n"
    "\n"
    "uniform mat4 zModel;\n"
    "uniform mat4 zView;\n"
    "uniform mat4 zProjection;\n"
    "uniform vec3 LightPosRelative;\n"
    "uniform mat4 Rotate;\n"
    "\n"
    "void main() {\n"
    "    mat4 Model = zModel * Rotate;\n"
    "    gl_Position = zProjection * zView * Model * "
    "vec4(vertexPosition_modelspace, 1);\n"
    "\n"
    "    UV = vertexUV;\n"
    "    norm = mat3(transpose(inverse(Model))) * vertexNorm;\n"
    "    fragPos = vec3(Model * vec4(vertexPosition_modelspace, 1.0));\n"
    "    float w = zView[3][3];\n"
    "    cameraPos = vec3(zView[3]) / w;\n"
    "    lightPos = mat3(zModel) * LightPosRelative / w;\n"
    "}\n";

const char *const fragment_shader =
    "#version 450 core\n"
    "\n"
    "in vec2 UV;\n"
    "in vec3 norm;\n"
    "in vec3 fragPos;\n"
    "in vec3 cameraPos;\n"
    "in vec3 lightPos;\n"
    "\n"
    "out vec4 color;\n"
    "\n"
    "uniform sampler2D userTexture;\n"
    "\n"
    "void main() {\n"
    "    vec3 n = normalize(norm);\n"
    "    vec3 lightDir = normalize(lightPos - fragPos);\n"
    "    vec3 lightColor = vec3(1.0, 0.8, 0.8);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor * 1.0;\n"
    "\n"
    "    vec3 viewDir = normalize(cameraPos - fragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = 0.3 * spec * lightColor;\n"
    "\n"
    "    vec4 objectColor = texture(userTexture, UV);\n"
    "    vec3 objectColor3d = vec3(objectColor);\n"
    "    float objecta = objectColor[3];\n"
    "    float ambient = 0.4;\n"
    "    color = vec4((ambient + diffuse + specular) * lightColor * "
    "objectColor3d, objecta);\n"
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
  glm::vec3 light;
  glm::mat4 rotate;
};

Vertex *get_points();
u_short *vertex_indices();

Env *setup_env();

void update_env(Env *e);
void set_obj_uniform_variables(zgn_opengl_shader_program *shader, Env *e);

zgn_opengl_vertex_buffer *opengl_setup_vertex_buffer(
    zgn_opengl *opengl, wl_shm *shm, Vertex *points, uint points_len);
#endif  //  OBJ_H
