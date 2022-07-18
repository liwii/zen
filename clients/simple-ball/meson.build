project(
    'simple-ball', 'c', 'cpp',
    version: '0.0.1',
    license: 'MIT',
    meson_version: '>= 0.52.1',
    default_options: ['warning_level=3', 'werror=true', 'optimization=2', 'cpp_std=gnu++14' ],
)

wayland_client_dep = dependency('wayland-client')

subdir('src')