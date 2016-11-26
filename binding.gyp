{
  'variables': {
    'use_udev%': 1,
    'use_system_libusb%': 'false',
  },
  'targets': [
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    },
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
