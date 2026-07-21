sources = files (
    'pishutdown.c',
    'activate.c'
)

gtk = dependency ('gtk4')
deps = [ deps, gtk ]

executable (meson.project_name(), sources, dependencies: deps, install: true)
