{
  'variables': {
    'use_udev%': 1,
    'use_system_libusb%': 'false'
  },
  'dependencies': [
    "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except"
  ],
  'targets': [
    {
      'target_name': 'usb_bindings',
      'cflags!': [
        '-fno-exceptions'
      ],
      'cflags_cc!': [
        '-fno-exceptions'
      ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7'
      },
      'msvs_settings': {
        'VCCLCompilerTool': {
          'ExceptionHandling': 1
        }
      },
      'sources': [
        'src/node_usb.cc',
        'src/device.cc',
        'src/transfer.cc',
        'src/thread_name.cc'
      ],
      'cflags_cc': [
        '-std=c++14'
      ],
      'defines': [
        '_FILE_OFFSET_BITS=64',
        '_LARGEFILE_SOURCE',
        'NAPI_VERSION=8'
      ],
      'include_dirs+': [
        'src/',
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'conditions' : [
          ['use_system_libusb=="false" and OS!="freebsd"', {
            'dependencies': [
              'libusb.gypi:libusb',
            ]
          }],
          ['use_system_libusb=="true" or OS=="freebsd"', {
            'include_dirs+': [
              '<!@(pkg-config libusb-1.0 --cflags-only-I | sed s/-I//g)'
            ],
            'libraries': [
              '<!@(pkg-config libusb-1.0 --libs)'
            ]
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'OTHER_CFLAGS': [
                '-std=c++1y',
                '-stdlib=libc++',
                '-arch x86_64',
                '-arch arm64'
              ],
              'OTHER_LDFLAGS': [
                '-framework', 'CoreFoundation',
                '-framework', 'IOKit',
                '-arch x86_64',
                '-arch arm64'
              ],
              'SDKROOT': 'macosx',
              'MACOSX_DEPLOYMENT_TARGET': '10.7'
            }
          }],
          ['OS!="win"', {
            'sources': [
              'src/hotplug/libusb.cc'
            ],
          }],
          ['OS=="win"', {
            'sources': [
              'src/hotplug/windows.cc'
            ],
            'defines':[
              'WIN32_LEAN_AND_MEAN'
            ],
            'libraries': [
              'cfgmgr32.lib'
            ],
            'default_configuration': 'Debug',
            'configurations': {
              'Debug': {
                'defines': [
                  'DEBUG',
                  '_DEBUG'
                ],
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'RuntimeLibrary': 1, # static debug
                  }
                }
              },
              'Release': {
                'defines': [
                  'NDEBUG'
                ],
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'RuntimeLibrary': 0, # static release
                  }
                }
              }
            },
            'msvs_settings': {
              'VCCLCompilerTool': {
                'AdditionalOptions': [ '/EHsc' ]
              }
            }
          }
        ]
      ]
    }
  ]
}
