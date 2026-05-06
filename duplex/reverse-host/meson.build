cc = meson.get_compiler('c')

reverse_host_sources = files(
    'src/reverse_host.c',
)

reverse_host_deps = []

if host_machine.system() == 'windows'
    reverse_host_sources += files(
        'src/win/duplex_input_win.c',
        'src/win/capture_dxgi_stub.c',
    )
    reverse_host_deps += cc.find_library('ws2_32', required: true)
    reverse_host_deps += cc.find_library('user32', required: true)
else
    reverse_host_sources += files('src/unix/duplex_input_stub.c')
endif

executable(
    'sc-reverse-host',
    reverse_host_sources,
    include_directories: include_directories('include'),
    dependencies: reverse_host_deps,
    install: false,
    c_args: host_machine.system() == 'windows' ? ['-DWIN32_LEAN_AND_MEAN'] : [],
)
