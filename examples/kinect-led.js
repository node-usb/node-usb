var usb_driver = require("../usb.js");
var assert = require('assert');

var usb = usb_driver.create()

// taken from freenect_internal.h
var VID_MICROSOFT = 0x45e, PID_NUI_CAMERA = 0x02ae, PID_NUI_MOTOR = 0x02b0;

// taken from libfreenect.h
var LED_OPTIONS = new Object();
LED_OPTIONS['OFF'] 		= 0;
LED_OPTIONS['GREEN'] 		= 1;
LED_OPTIONS['RED']		= 2;
LED_OPTIONS['YELLOW']		= 3;
LED_OPTIONS['BLINK_YELLOW']	= 4;
LED_OPTIONS['BLINK_GREEN']	= 5;
LED_OPTIONS['BLINK_RED_YELLOW'] = 6;

// Search for motor device
var motorDevices = usb.find_by_vid_and_pid(VID_MICROSOFT, PID_NUI_MOTOR);
assert.ok((motorDevices.length >= 1));
console.log("Total motor devices found: " + motorDevices.length);

var motor = motorDevices[0];

// get interfaces of motor
var motorInterfaces = motor.getInterfaces();
assert.ok((motorInterfaces.length >= 1));
console.log("Motor contains interfaces: " + motorInterfaces.length);
 
// claim first interface
var motorInterface = motorInterfaces[0];
console.log("Claiming motor interface for further actions");
motorInterface.claim();

console.log("Toggling LED light ...");
// send toggle command 
motor.controlTransfer(new Array(), 0x40, 0x06, LED_OPTIONS['BLINK_GREEN'], 0x0, function(data) {
	console.log("LED toggled");
});
