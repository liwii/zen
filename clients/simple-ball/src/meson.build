srcs_simple_ball = [
    'main.cc',
    'main.h',
    'cuboid_window.cc',
    'cuboid_window.h',
    'ray.cc',
    'ray.h',
    'buffer.cc',
    'buffer.h',
    zigen_protocol_c,
    zigen_client_protocol_h,
    zigen_shell_protocol_c,
    zigen_shell_client_protocol_h,
    zigen_opengl_protocol_c,
    zigen_opengl_client_protocol_h,
]

deps_simple_ball = [
    wayland_client_dep
]

executable(
    'zen-simple-ball',
    srcs_simple_ball,
    dependencies: deps_simple_ball,
)