var usb = require('./src/binding/usb_bindings.node')



var dev = usb.getDeviceList()[0]

var transf = new usb.Transfer()



