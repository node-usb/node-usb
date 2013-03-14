{
  'targets': [  
    {
      'target_name': 'usb_bindings',
      'sources': [ 
        './src/node_usb.cc',
        './src/device.cc',
        './src/transfer.cc',
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
        '-std=c++0x',
        '-g'
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
                    ],
                    'defines': [
                      'USE_POLL',
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
