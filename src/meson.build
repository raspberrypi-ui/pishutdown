sources = files (
    'pishutdown.c',
    'activate.c'
)

gtk = dependency ('gtk+-3.0')
xml = dependency ('libxml-2.0')
deps = [ deps, gtk, xml ]

executable ('pishutdown', sources, dependencies: deps, install: true)
