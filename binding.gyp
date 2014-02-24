{
  'variables': {
    'target_arch%': 'ia32', # built for a 32-bit CPU by default
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
        'src/'
      ],
      'conditions' : [
          ['OS=="linux"', {
            'include_dirs+': [
              '<!@(pkg-config libusb-1.0 --cflags-only-I | sed s/-I//g)'
            ],
            'libraries': [
              '<!@(pkg-config libusb-1.0 --libs)'
            ],
            'defines': [
              #'USE_POLL',
            ]
          }],
          ['OS=="mac"', {
            'include_dirs+': [
              '<!@(pkg-config libusb-1.0 --cflags-only-I | sed s/-I//g)'
            ],
            'libraries': [
              # Force static library linkage
              '<!@(pkg-config libusb-1.0 --libs | sed s/-L//g | sed "s/ -l/\/lib/g").a'
            ],
            'xcode_settings': {
              'SDKROOT': 'macosx',
              'MACOSX_DEPLOYMENT_TARGET': '10.5',
            },
          }],
          ['OS=="win"', {
            'variables': {
              # Path to extracted libusbx windows binary package from http://libusbx.org/
              'libusb_path': "C:/Program Files/libusb"
            },
            'defines':[
              'WIN32_LEAN_AND_MEAN'
            ],
            'include_dirs+': [
              '<(libusb_path)/include/libusbx-1.0',
              '<(libusb_path)/include/libusb-1.0',
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
            "conditions" : [
              ["target_arch=='ia32'", {
                'libraries': [
                   '<(libusb_path)/MS32/dll/libusb-1.0.lib'
                ],
                "copies": [
                  {
                    "destination": "<(PRODUCT_DIR)",
                    'files': [ '<(libusb_path)/MS32/dll/libusb-1.0.dll' ]
                  }
                ]
              }],
              ["target_arch=='x64'", {
                'libraries': [
                   '<(libusb_path)/MS64/dll/libusb-1.0.lib'
                ],
                "copies": [
                  {
                    "destination": "<(PRODUCT_DIR)",
                    'files': [ '<(libusb_path)/MS64/dll/libusb-1.0.dll' ]
                  }
                ]
              }]
            ]
          }]
      ]
    }
  ]
}
