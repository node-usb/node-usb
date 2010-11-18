import Options, Utils
from os import unlink, symlink, chdir
from os.path import exists

srcdir = '.'
blddir = 'build'
VERSION = '0.1'

# def options(ctx):

def set_options(opt):
	opt.tool_options("compiler_cxx")
	opt.tool_options("compiler_cc")
	opt.add_option('--debug', action='store', default=False, help='Enable debugging output')

def configure(conf):
	conf.check_tool('compiler_cxx')
	conf.check_tool("compiler_cc")	
	conf.check_tool('node_addon')

	conf.check_cfg(package='libusb-1.0', mandatory=1, args='--cflags --libs')
	conf.env.append_unique('CPPFLAGS', ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"])
	conf.env.append_unique('CXXFLAGS', ["-Wall"])
	
	# libusb lbiraries
	conf.check(lib='usb-1.0', libpath=['/lib', '/usr/local/lib/', '/usr/lib']);

def build(bld):
	obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
	obj.target = 'usb_bindings'
	obj.source = './src/node_usb.cc ./src/usb.cc ./src/device.cc ./src/interface.cc ./src/endpoint.cc'
	# TODO include path hard linked; should be build option
	obj.includes = '/usr/include /usr/include/libusb-1.0'
	obj.uselib = ["USB-1.0"]
	obj.lib = ["usb-1.0"]
	obj.name = "node-usb"
	obj.linklags = ['/usr/lib/libusb-1.0a']
	
	if (Options.options.debug != False) and (Options.options.debug == 'true'):
		obj.defines = ['ENABLE_DEBUG=1']

def shutdown():
	t = 'usb_bindings.node';
	
	if exists('build/default/' + t) and not exists(t):
		symlink('build/default/' + t, t)
