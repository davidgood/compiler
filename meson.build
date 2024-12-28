project('compiler', 'c', version : '0.1', default_options : ['c_std=c23'])

# Output directories
bindir = meson.current_build_dir() / 'bin'
libdir = meson.current_build_dir() / 'lib'

# Default group to build
target_group = get_option('target_group')

if target_group == 'release'
    subdir('src')
elif target_group == 'test'
    subdir('Unity')
    subdir('test')
else
    error('Unknown TARGET_GROUP: @0@'.format(target_group))
endif
