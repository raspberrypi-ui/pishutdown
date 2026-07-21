sources = files (
    'pishutdown.c',
    'activate.c'
)

gtk = dependency ('gtk4')
deps = [ deps, gtk ]

executable ('pishutdown', sources, dependencies: deps, install: true)
