project('librelay', 'cpp')

prefix_library_path=[get_option('prefix')+'/lib', get_option('prefix')+'/lib/x86_64-linux-gnu', '/usr/local/lib', '/usr/local/lib/x86_64-linux-gnu']

inc_dirs = include_directories('include', 'src', '/usr/local/include', get_option('prefix')+'/include')

compile_args = ['-std=c++20', '-Wextra', '-Werror']

cpp = meson.get_compiler('cpp')
log_dep = dependency('glog')
boost_po_dep = cpp.find_library('boost_program_options')

thread_dep = dependency('threads')
yael_dep = cpp.find_library('yael', dirs: prefix_library_path)
json_dep = cpp.find_library('document', dirs: prefix_library_path)

subdir('src')

librelay = shared_library('relay', client_cpp_files, include_directories: inc_dirs, install: true, cpp_args: compile_args, dependencies: [log_dep, yael_dep])

relaynode = executable('relay-node', relay_node_main + node_cpp_files, include_directories: inc_dirs, install: true, cpp_args: compile_args, dependencies: [yael_dep, boost_po_dep, log_dep, json_dep, thread_dep], link_args: ['-lstdc++fs'])

edgenode = executable('relay-edge-node', edge_node_main + node_cpp_files, include_directories: inc_dirs, install: true, cpp_args: compile_args, dependencies: [yael_dep, boost_po_dep, log_dep, json_dep, thread_dep], link_args: ['-lstdc++fs'])


test_files = files('test/main.cpp')
executable('relay-test', test_files, link_with: librelay, include_directories: inc_dirs, cpp_args:compile_args, dependencies: [log_dep, boost_po_dep, yael_dep])

install_subdir('include/librelay', install_dir : 'include')

# NOTE: gtest on ubuntu still uses deprecated functions so we can't lint the test files yet
clangtidy = find_program('clang-tidy', required: false)

if clangtidy.found()
    tidy_files = client_cpp_files + node_cpp_files

    run_target(
        'tidy',
        command: [
            'scripts/clang-tidy.sh',
            clangtidy.path(),
            meson.source_root(),
            meson.build_root(),
            tidy_files
        ],
    depends: [relaynode, librelay])
endif

