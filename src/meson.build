sources = files (
    'pishutdown.c'
)

add_global_arguments('-Wno-unused-result', language : 'c')

gtk = dependency ('gtk+-3.0')
deps = [ gtk ]

executable ('lxde-pi-shutdown-helper', sources, dependencies: deps, install: true)
