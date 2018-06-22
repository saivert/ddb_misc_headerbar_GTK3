project('ddb_misc_headerbar_GTK3', 'c', version : '0.1')

gnome = import('gnome')
gtkdep = dependency('gtk+-3.0', version: '>= 3.10.8')

simple_resources = gnome.compile_resources('resources',
  'src/headerbarui.gresource.xml',
  source_dir : './src',
  c_name : 'headerbarui')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

myfiles = [
  'src/ddbheaderbar.c',
  'src/headerbarui.c',
  simple_resources
]

shared_library(meson.project_name(), myfiles, dependencies : [gtkdep, m_dep], name_prefix: '', c_args : '-fPIC')

