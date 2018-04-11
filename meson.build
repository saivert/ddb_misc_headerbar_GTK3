project('ddb_misc_headerbar_GTK3', 'c', version : '0.1')

gnome = import('gnome')
gtkdep = dependency('gtk+-3.0')

simple_resources = gnome.compile_resources('resources',
  'src/headerbarui.gresource.xml',
  source_dir : './src',
  c_name : 'headerbarui')

shared_library(meson.project_name(), ['src/headerbarui.c',simple_resources], dependencies : gtkdep, name_prefix: '')

