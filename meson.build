project(
  'axis-desktop',
  ['c', 'cpp'],
  version: '2.0.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 0.59.0'
)

# Core GNOME / GTK 4 / libadwaita dependencies
gtk4_dep = dependency('gtk4', version: '>= 4.6.0', required: true)
libadwaita_dep = dependency('libadwaita-1', version: '>= 1.2.0', required: true)

# Native Terminal dependencies (GTK3 & VTE)
gtk3_dep = dependency('gtk+-3.0', required: false)
vte_dep = dependency('vte-2.91', required: false)

# Python 3 dependency
py_mod = import('python')
py_inst = py_mod.find_installation('python3')

prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
bindir = prefix / get_option('bindir')

# Theme stylesheets installation
theme_dir = datadir / 'axis' / 'theme'
install_data(
  files(
    'data/theme/axis-tokens.css',
    'data/theme/axis-components.css',
    'data/theme/axis-dark.css',
    'data/theme/axis-light.css'
  ),
  install_dir: theme_dir
)

# HarmonyOS-inspired Icon Theme installation
icons_dir = datadir / 'icons'
install_subdir(
  'data/icons/AxisHarmony',
  install_dir: icons_dir
)
install_data(
  files('data/icons/index.theme'),
  install_dir: icons_dir / 'AxisHarmony'
)

# AxisOS Core Applications and Modules installation
axis_lib_dir = datadir / 'axis'
install_data(
  files(
    'src/theme_manager.py',
    'src/hardware_info.py',
    'src/installer.py',
    'src/settings.py',
    'src/shell.py'
  ),
  install_dir: axis_lib_dir
)

# Native C++ Terminal executable
if gtk3_dep.found() and vte_dep.found()
  executable(
    'axis-terminal',
    'os-build/apps/axis-terminal/main.cpp',
    dependencies: [gtk3_dep, vte_dep],
    install: true,
    install_dir: bindir
  )
endif


# Executable entrypoint wrappers
configure_file(
  input: 'data/bin/axis-shell.in',
  output: 'axis-shell',
  configuration: {'AXIS_DIR': axis_lib_dir},
  install: true,
  install_dir: bindir,
  install_mode: 'rwxr-xr-x'
)

configure_file(
  input: 'data/bin/axis-settings.in',
  output: 'axis-settings',
  configuration: {'AXIS_DIR': axis_lib_dir},
  install: true,
  install_dir: bindir,
  install_mode: 'rwxr-xr-x'
)

configure_file(
  input: 'data/bin/axis-installer.in',
  output: 'axis-installer',
  configuration: {'AXIS_DIR': axis_lib_dir},
  install: true,
  install_dir: bindir,
  install_mode: 'rwxr-xr-x'
)

summary({
  'Prefix': prefix,
  'Datadir': datadir,
  'Bindir': bindir,
  'Theme Directory': theme_dir,
  'Icons Directory': icons_dir / 'AxisHarmony'
}, section: 'AxisOS HarmonyOS Desktop')
