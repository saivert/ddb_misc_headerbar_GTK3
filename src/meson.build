simple_resources = gnome.compile_resources('resources',
  'headerbarui.gresource.xml',
  c_name : 'headerbarui')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

myfiles = [
  'ddbheaderbar.c',
  'headerbarui.c',
  simple_resources
]

shared_library(meson.project_name(), myfiles, dependencies : [gtkdep, m_dep], name_prefix: '', c_args : '-fPIC')

