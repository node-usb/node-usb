{
  'variables': {
    'use_udev%': 1,
    'use_system_libusb%': 'false',
    'module_name': 'usb_bindings',
    'module_path': './src/binding'
  },
  'targets': [
    {
      # Based on https://chromium.googlesource.com/chromium/src/+/master/third_party/libusb/libusb.gyp
      'target_name': 'libusb',
      'type': 'static_library',
      'sources': [
        'libusb_config/config.h',
        'libusb/libusb/core.c',
        'libusb/libusb/descriptor.c',
        'libusb/libusb/hotplug.c',
        'libusb/libusb/io.c',
        'libusb/libusb/libusb.h',
        'libusb/libusb/libusbi.h',
        'libusb/libusb/strerror.c',
        'libusb/libusb/sync.c',
        'libusb/libusb/version_nano.h',
        'libusb/libusb/version.h',
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
      'cflags': [
        '-w',
      ],
      'conditions': [
        [ 'OS == "linux" or OS == "android" or OS == "mac"', {
          'sources': [
            'libusb/libusb/os/events_posix.c',
            'libusb/libusb/os/events_posix.h',
            'libusb/libusb/os/threads_posix.c',
            'libusb/libusb/os/threads_posix.h',
          ],
          'defines': [
            'PLATFORM_POSIX=1',
            'PRINTF_FORMAT(a, b)=__attribute__ ((__format__ (__printf__, a, b)))',
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
            'HAVE_CLOCK_GETTIME=1',
            'OS_LINUX=1',
            '_GNU_SOURCE=1',
            'USBI_TIMERFD_AVAILABLE=1',
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
            'libusb/Xcode/config.h',
          ],
          'include_dirs': [
            'libusb/Xcode',
          ],
          'defines': [
            'OS_DARWIN=1',
          ],
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '-arch x86_64',
              '-arch arm64'
            ],
            'CLANG_CXX_LIBRARY': 'libc++',
            'MACOSX_DEPLOYMENT_TARGET': '10.7'
          }
        }],
        [ 'OS == "win"', {
          'sources': [
            'libusb/libusb/os/events_windows.c',
            'libusb/libusb/os/events_windows.h',
            'libusb/libusb/os/threads_windows.c',
            'libusb/libusb/os/threads_windows.h',
            'libusb/libusb/os/windows_common.c',
            'libusb/libusb/os/windows_common.h',
            'libusb/libusb/os/windows_usbdk.c',
            'libusb/libusb/os/windows_usbdk.h',
            'libusb/libusb/os/windows_winusb.c',
            'libusb/libusb/os/windows_winusb.h',
            'libusb/msvc/config.h',
          ],
          'include_dirs!': [
            'libusb_config',
          ],
          'include_dirs': [
            'libusb/msvc',
          ],
          'msvs_disabled_warnings': [ 4267 ],
          'msvs_settings': {
              'VCCLCompilerTool': {
                'AdditionalOptions': [ '/source-charset:utf-8' ],
              },
            }
        }],
      ],
    },
  ]
}
