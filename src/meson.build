sources = files (
    'pishutdown.c',
    'activate.c'
)

gtk = dependency ('gtk+-3.0')
deps = [ deps, gtk ]

executable ('pishutdown', sources, dependencies: deps, install: true)
