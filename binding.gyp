{
  'variables': {
    'use_udev%': 1,
    'use_system_libusb%': 'false',
    'v8_enable_pointer_compression': 0,
    'v8_enable_31bit_smis_on_64bit_arch': 0,
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

      'conditions' : [
          ['use_system_libusb=="false" and OS!="freebsd"', {
            'dependencies': [
              'libusb.gypi:libusb',
            ],
          }],
          ['use_system_libusb=="true" or OS=="freebsd"', {
            'include_dirs+': [
              '<!@(pkg-config libusb-1.0 --cflags-only-I | sed s/-I//g)'
            ],
            'libraries': [
              '<!@(pkg-config libusb-1.0 --libs)'
            ],
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'OTHER_CFLAGS': [ '-std=c++1y', '-stdlib=libc++' ],
              'OTHER_LDFLAGS': [ '-framework', 'CoreFoundation', '-framework', 'IOKit' ],
              'SDKROOT': 'macosx',
              'MACOSX_DEPLOYMENT_TARGET': '10.7',
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
  ]
}
