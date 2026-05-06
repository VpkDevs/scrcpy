cc = meson.get_compiler('c')

reverse_host_sources = files(
    'src/reverse_host.c',
)

reverse_host_deps = []
reverse_host_args = host_machine.system() == 'windows' ? ['-DWIN32_LEAN_AND_MEAN'] : []

if host_machine.system() == 'windows'
    reverse_host_sources += files(
        'src/win/duplex_input_win.c',
    )
    reverse_host_deps += cc.find_library('ws2_32', required: true)
    reverse_host_deps += cc.find_library('user32', required: true)

    lavc = dependency('libavcodec', required: false)
    lavu = dependency('libavutil', required: false)
    sws = dependency('libswscale', required: false)

    have_duplex_video = lavc.found() and lavu.found() and sws.found()
    if have_duplex_video
        reverse_host_sources += files(
            'src/win/capture_dxgi.c',
            'src/win/encode_lavc.c',
        )
        reverse_host_deps += lavc
        reverse_host_deps += lavu
        reverse_host_deps += sws
        reverse_host_deps += cc.find_library('d3d11', required: true)
        reverse_host_deps += cc.find_library('dxgi', required: true)
        reverse_host_args += ['-DHAVE_DUPLEX_VIDEO']
    endif
else
    reverse_host_sources += files('src/unix/duplex_input_stub.c')
endif

executable(
    'sc-reverse-host',
    reverse_host_sources,
    include_directories: include_directories('include'),
    dependencies: reverse_host_deps,
    install: false,
    c_args: reverse_host_args,
)
