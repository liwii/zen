int get_shared_shader_fd(const char *shader);

void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::mat4 mat);
void set_shader_uniform_variable(struct zgn_opengl_shader_program *shader,
    const char *location, glm::vec4 vec);