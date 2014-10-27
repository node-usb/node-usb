{
  'variables': {
    'target_arch%': 'ia32', # built for a 32-bit CPU by default
    'use_udev%': 1,
  },
  'targets': [
    {
      'target_name': 'usb_bindings',
      'sources': [
        './src/node_usb.cc',
        './src/device.cc',
        './src/transfer.cc',
      ],
      'cflags_cc': [
        '-std=c++0x'
      ],
      'defines': [
        '_FILE_OFFSET_BITS=64',
        '_LARGEFILE_SOURCE',
      ],
      'include_dirs+': [
        'src/',
        "<!(node -e \"require('nan')\")",
      ],
      'dependencies': [
        'libusb',
      ],
      'conditions' : [
          ['OS=="linux"', {
            'defines': [
              #'USE_POLL',
            ]
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'OTHER_CFLAGS': [ '--std=c++1y' ],
              'OTHER_LDFLAGS': [ '-framework', 'CoreFoundation', '-framework', 'IOKit' ],
              'SDKROOT': 'macosx',
              'MACOSX_DEPLOYMENT_TARGET': '10.5',
            },
          }],
          ['OS=="win"', {
            'defines':[
              'WIN32_LEAN_AND_MEAN'
            ],
            'default_configuration': 'Debug',
            'configurations': {
              'Debug': {
                'defines': [ 'DEBUG', '_DEBUG' ],
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'RuntimeLibrary': 1, # static debug
                  },
                },
              },
              'Release': {
                'defines': [ 'NDEBUG' ],
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'RuntimeLibrary': 0, # static release
                  },
                },
              }
            },
            'msvs_settings': {
              'VCCLCompilerTool': {
                'AdditionalOptions': [ '/EHsc' ],
              },
            },
          }]
      ]
    },
    {
      # Based on https://chromium.googlesource.com/chromium/src/+/master/third_party/libusb/libusb.gyp
      'target_name': 'libusb',
      'type': 'static_library',
      'sources': [
        'libusb_config/config.h',
        'libusb/libusb/core.c',
        'libusb/libusb/descriptor.c',
        'libusb/libusb/hotplug.c',
        'libusb/libusb/hotplug.h',
        'libusb/libusb/io.c',
        'libusb/libusb/libusb.h',
        'libusb/libusb/libusbi.h',
        'libusb/libusb/strerror.c',
        'libusb/libusb/sync.c',
        'libusb/libusb/version.h',
        'libusb/libusb/version_nano.h',
      ],
      'include_dirs': [
        'libusb_config',
        'libusb/libusb',
        'libusb/libusb/os',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'libusb/libusb',
        ],
      },
      'defines': [
        'ENABLE_LOGGING=1',
      ],
      'conditions': [
        [ 'OS != "win"', {
          'cflags': [
            '-Wno-sign-compare',
          ]
        }],
        [ 'OS == "linux" or OS == "android" or OS == "mac"', {
          'sources': [
            'libusb/libusb/os/poll_posix.c',
            'libusb/libusb/os/poll_posix.h',
            'libusb/libusb/os/threads_posix.c',
            'libusb/libusb/os/threads_posix.h',
          ],
          'defines': [
            'DEFAULT_VISIBILITY=',
            'HAVE_GETTIMEOFDAY=1',
            'HAVE_POLL_H=1',
            'HAVE_SYS_TIME_H=1',
            'LIBUSB_DESCRIBE="1.0.17"',
            'POLL_NFDS_TYPE=nfds_t',
            'THREADS_POSIX=1',
          ],
        }],
        [ 'OS == "linux" or OS == "android"', {
          'sources': [
            'libusb/libusb/os/linux_usbfs.c',
            'libusb/libusb/os/linux_usbfs.h',
          ],
          'defines': [
            'OS_LINUX=1',
            '_GNU_SOURCE=1',
          ],
        }],
        [ 'OS == "linux" and use_udev == 1 or OS == "android"', {
          'sources': [
            'libusb/libusb/os/linux_udev.c',
          ],
          'defines': [
            'HAVE_LIBUDEV=1',
            'USE_UDEV=1',
          ],
          'direct_dependent_settings': {
            'libraries': [
              '-ludev',
            ]
          }
        }],
        [ 'OS == "linux" and use_udev == 0', {
          'sources': [
            'libusb/libusb/os/linux_netlink.c',
          ],
          'defines': [
            'HAVE_LINUX_NETLINK_H',
          ],
          'conditions': [
            ['clang==1', {
              'cflags': [
                '-Wno-pointer-sign',
              ]
            }]
          ],
        }],
        [ 'OS == "mac"', {
          'sources': [
            'libusb/libusb/os/darwin_usb.c',
            'libusb/libusb/os/darwin_usb.h',
          ],
          'defines': [
            'OS_DARWIN=1',
          ],
        }],
        [ 'OS == "win"', {
          'sources': [
            'libusb/libusb/os/poll_windows.c',
            'libusb/libusb/os/poll_windows.h',
            'libusb/libusb/os/threads_windows.c',
            'libusb/libusb/os/threads_windows.h',
            'libusb/libusb/os/windows_common.h',
            'libusb/libusb/os/windows_usb.c',
            'libusb/libusb/os/windows_usb.h',
            'libusb/msvc/config.h',
            'libusb/msvc/inttypes.h',
            'libusb/msvc/stdint.h',
          ],
          'include_dirs!': [
            'libusb_config',
          ],
          'include_dirs': [
            'libusb/msvc',
          ],
          'msvs_disabled_warnings': [ 4267 ],
        }],
      ],
    },
  ]
}
