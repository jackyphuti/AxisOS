project(
  'axis-desktop',
  'c',
  version: '2.0.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'optimization=2'
  ],
  meson_version: '>= 0.59.0'
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

# Pure C GTK3 & Wayland Desktop Stack Dependencies
gtk3_dep = dependency('gtk+-3.0', required: true)
vterm_dep = dependency('vterm', required: false)
util_dep = cc.find_library('util', required: false)
gio_dep = dependency('gio-2.0', required: true)
cairo_dep = dependency('cairo', required: true)
pango_dep = dependency('pangocairo', required: true)

prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
bindir = prefix / get_option('bindir')

# 1. Theme Stylesheets Installation
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

# 2. Pure C Desktop Applications Executables
executable(
  'axis-installer',
  'os-build/apps/axis-installer/main.c',
  dependencies: [gtk3_dep, gio_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axisos-settings-panel',
  'os-build/apps/axis-settings/main.c',
  dependencies: [gtk3_dep, gio_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-task-manager',
  'os-build/apps/axis-task-manager/main.c',
  dependencies: [gtk3_dep, pango_dep, cairo_dep, m_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-oobe',
  'os-build/apps/axis-oobe/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'nobara-welcome',
  'os-build/apps/nobara-welcome/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-package-installer',
  'os-build/apps/axis-package-installer/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-scheduler-manager',
  'os-build/apps/axis-scheduler-manager/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-welcome',
  'os-build/apps/axis-welcome/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-software-manager',
  'os-build/apps/axis-software-manager/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

executable(
  'axis-update-manager',
  'os-build/apps/axis-update-manager/main.c',
  dependencies: [gtk3_dep],
  install: true,
  install_dir: bindir
)

if vterm_dep.found()
  executable(
    'axis-terminal',
    'os-build/apps/axis-terminal/main.c',
    dependencies: [gtk3_dep, vterm_dep, util_dep],
    install: true,
    install_dir: bindir
  )
endif

# 3. Executable Entrypoint Binaries (Pure C)
executable(
  'axis-shell',
  'data/bin/axis-shell.c',
  install: true,
  install_dir: bindir
)

executable(
  'axis-settings',
  'data/bin/axis-settings.c',
  install: true,
  install_dir: bindir
)

summary({
  'Prefix': prefix,
  'Datadir': datadir,
  'Bindir': bindir,
  'Language': 'Pure C11 / C17 (-std=gnu11)',
  'GUI Stack': 'GTK 3, Cairo, VTE, Pango',
  'C++ Runtime': 'None (libstdc++ never linked)',
  'Scripting Runtime': 'None (0 interpreted scripts in OS)'
}, section: 'AxisOS Pure C Architecture')
