project('ddb_misc_headerbar_GTK3', 'c', version : '0.1')

gnome = import('gnome')
gtkdep = dependency('gtk+-3.0', version: '>= 3.10.8')

simple_resources = gnome.compile_resources('resources',
  'src/headerbarui.gresource.xml',
  source_dir : './src',
  c_name : 'headerbarui')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

handy_dep = dependency('libhandy-1', required : true)

shared_library(meson.project_name(), ['src/headerbarui.c',simple_resources], dependencies : [gtkdep,m_dep,handy_dep], name_prefix: '',
c_args: ['-DGETTEXT_PACKAGE="Granite"'])

