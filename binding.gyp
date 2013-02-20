{
  'targets': [  
    {
      'target_name': 'usb_bindings',
      'sources': [ 
        './src/node_usb.cc',
        './src/bindings.cc',
        './src/usb.cc',
        './src/device.cc',
        './src/interface.cc',
        './src/endpoint.cc',
        './src/transfer.cc',
        './src/stream.cc',  
      ],
      'cflags': [
        '-O3',
        '-Wall',
        '-Werror',
      ],
      'cflags_cc': [
        '-O3',
        '-Wall',
        '-Werror',
        '-std=gnu++0x'
      ],      
      'defines': [
        '_FILE_OFFSET_BITS=64',
        '_LARGEFILE_SOURCE',
      ],
      'include_dirs+': [
        'src/'
      ],
      'link_settings': {
        'conditions' : [
            ['OS=="linux"',
                {
                    'libraries': [
                      '-lusb-1.0'
                    ]
                }
            ],
            ['OS=="mac"',
                {
                    'libraries': [
                      '-lusb-1.0'
                    ]
                }
            ]
        ]
      }  
    }
  ]
}
