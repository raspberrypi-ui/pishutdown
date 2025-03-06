project ('pishutdown','c')

share_dir = join_paths(get_option('prefix'), 'share')
resource_dir = join_paths(share_dir, 'pishutdown')
ui_dir = join_paths(resource_dir, 'ui')
desktop_dir = join_paths(share_dir, 'applications')

gettext_package = meson.project_name()

i18n = import('i18n')

add_project_arguments('-DGETTEXT_PACKAGE="' + gettext_package + '"', language : [ 'c', 'cpp' ])
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + share_dir + '/locale"', language : [ 'c', 'cpp' ])
add_project_arguments('-DPACKAGE_DATA_DIR="' + resource_dir + '"', language : [ 'c', 'cpp' ])
add_project_arguments('-DPACKAGE_UI_DIR="' + ui_dir + '"', language : [ 'c', 'cpp' ])

subdir('src')
subdir('data')
subdir('po')
