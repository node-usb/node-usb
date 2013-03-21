{
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
              'USE_POLL',
            ]
          }],
          ['OS=="mac"', {
            'include_dirs+': [
              '<!@(pkg-config libusb-1.0 --cflags-only-I | sed s/-I//g)'
            ],
            'libraries': [
              '<!@(pkg-config libusb-1.0 --libs)'
            ],
            'defines': [
              'USE_POLL',
            ]
          }],
          ['OS=="win"', {
            'variables': {
              # Path to extracted libusbx windows binary package from http://libusbx.org/
              'libusb_path': "C:/Program Files/libusb"
            },
            'defines':[
              'WIN32_LEAN_AND_MEAN'
            ],
            'libraries': [
               '<(libusb_path)/MS32/static/libusb-1.0.lib'
            ],
            'include_dirs+': [
              '<(libusb_path)/include/libusbx-1.0'
            ],
            'msvs_settings': {
              'VCCLCompilerTool': {
                'AdditionalOptions': [ '/EHsc /MD' ],
              },
            },
          }]
      ]
    }
  ]
}
