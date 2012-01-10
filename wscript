import os
import Options, Utils
from os import unlink, symlink, chdir, popen
from os.path import exists

srcdir = '.'
blddir = 'build'
VERSION = '0.1'
REVISION = popen("git log | head -n1 | awk '{printf \"%s\", $2}'").readline()

def set_options(opt):
	opt.tool_options("compiler_cxx")
	opt.tool_options("compiler_cc")
	opt.add_option('--debug', action='store', default=False, help='Enable debugging output')

def configure(conf):
	conf.check_tool('compiler_cxx')
	conf.check_tool("compiler_cc")	
	conf.check_tool('node_addon')

	conf.check_cfg(package='libusb-1.0', uselib_store='USB10', mandatory=1, args='--cflags --libs')
	conf.env.append_unique('CPPFLAGS', ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"])
	conf.env.append_unique('CXXFLAGS', ["-Wall"])
	conf.env.append_value('CPPFLAGS_NODE', ['-DEV_MULTIPLICITY=1'])

def build(bld):
	obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
	obj.target = 'usb_bindings'
	obj.source = './src/node_usb.cc ./src/usb.cc ./src/device.cc ./src/interface.cc ./src/endpoint.cc'
	obj.includes = bld.env['CPPPATH_USB10'] 
	obj.lib = bld.env['LIB_USB10']
	obj.name = "node-usb"
	obj.defines = ['NODE_USB_REVISION="' + REVISION + '"']

	if (Options.options.debug != False) and (Options.options.debug == 'true'):
		obj.defines.append('ENABLE_DEBUG=1')
		
def shutdown():
	t = 'usb_bindings.node';
	
	if exists('build/default/' + t) and not exists(t):
		symlink('build/default/' + t, t)
