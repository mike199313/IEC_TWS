project(
    'inventec-vgpio',
    'cpp',
    default_options: [
        'warning_level=3',
        'cpp_std=c++20'
    ],
    license: 'Apache-2.0',
    version: '0.1',
    meson_version: '>=0.57.0',
)

add_project_arguments(
    '-Wno-psabi',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_NO_RTTI',
    '-DBOOST_NO_TYPEID',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_ASIO_DISABLE_THREADS',
    language: 'cpp',
)

cpp_args = []

deps = [
  dependency('systemd'),
  dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep']),
  dependency('phosphor-logging', fallback: ['phosphor-logging', 'phosphor_logging_dep']),
]

executable(
  'inventec-vgpio',
  'src/inventec-vgpio.cpp',
  include_directories: include_directories('include'),
  cpp_args: cpp_args,
  dependencies: deps,
  install: true,
  install_dir: get_option('bindir'))

install_data(
  'config/inventec-vgpio.json',
  install_dir: '/usr/share/inventec-vgpio/')
