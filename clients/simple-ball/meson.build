project(
    'simple-ball', 'c', 'cpp',
    version: '0.0.1',
    license: 'MIT',
    meson_version: '>= 0.58',
    default_options: ['warning_level=3', 'werror=true', 'optimization=2', 'cpp_std=gnu++14' ],
)

wayland_client_dep = dependency('wayland-client')
wayland_scanner_dep = dependency('wayland-scanner')
zgn_protocol_dep = dependency('zigen-protocols')

pkgconfig = import('pkgconfig')

subdir('protocols')
subdir('src')