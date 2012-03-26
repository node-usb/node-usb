// usage without warranty!
var usb_driver = require("../../usb.js"),
	assert = require('assert'),
	http = require('http'),
	qs = require('querystring');

	
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

var MAX_TILT_ANGLE = 31, MIN_TILT_ANGLE = -31;

// Search for motor device
var usb = usb_driver.create()

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

// create simple web GUI. don't blame me - I know it's dirty
http.createServer(function(req, res) {
	res.writeHead(200, {'Content-Type': 'text/html'});
	var incomingBody = "";
	req.on('data', function(data) { incomingBody += data; });
	req.on('end', function() {
		var postData = qs.parse(incomingBody);
		var bulkOutput = "<strong>no data</strong>";

		switch (req.url) {
			case '/updateLed':
				var light = postData['light'];

				if (!light || (typeof(LED_OPTIONS[light]) == undefined)) {
					console.log(" - No light available");
				}
				else {
					var lightId = LED_OPTIONS[light];
				
					// send control information
					motor.controlTransfer(new Buffer(0), 0x40, 0x06, lightId, 0x0, function(data) {
						console.log(" + LED toggled");
					});
				}
				break;
			case '/updateAngle':
				var angle = parseInt(postData['angle']);

				if (isNaN(angle)) {
					angle = 0;
				}

				angle = (angle < MIN_TILT_ANGLE) ? MIN_TILT_ANGLE : ((angle > MAX_TILT_ANGLE) ? MAX_TILT_ANGLE : angle);
				angle = angle * 2;
				console.log("Angle set to " + angle);
				motor.controlTransfer(new Buffer(0), 0x40, 0x31, angle, 0x0, function(data) {
					console.log(" + Angle set");
				});

				break;
			case '/getCoordinates':
				motor.controlTransfer(10 /* read 10 bytes */, 0xC0 /* bmRequestType */, 0x32 /* bRequest */, 0, 0, function(data) {
					for (var i = 0; i < 10; i++) {
						console.log("buffer[" + i + "]: " + data[i])
					}
					console.log("Accelerometer axis:")
					var x = ((data[2] << 8) | data[3]), y = ((data[4] << 8) | data[5]), z = ((data[6] << 8) | data[7])
					x = (x + Math.pow(2,15)) % Math.pow(2,16) - Math.pow(2,15)
					y = (y + Math.pow(2,15)) % Math.pow(2,16) - Math.pow(2,15)
					z = (z + Math.pow(2,15)) % Math.pow(2,16) - Math.pow(2,15)

					console.log("  X:" + x)
					console.log("  Y:" + y)
					console.log("  Z:" + z)
				})
				break;
		}
	});

	var html = "<html><head><title>node-usb :: Microsoft Kinect example</title></head><body><h1>Control your Microsoft Kinect via node.js / node-usb</h1><form method='post' action='updateLed'>";
	html += "Select LED light: <select name='light'>";

	for (var prop in LED_OPTIONS) {
		html += "<option value='" + prop + "'>" + prop + "</option>";
	}

	html += "</select><input type='submit' value='change color' /></form>";
	html += "<form method='post' action='updateAngle'>";
	html += "Set angle (-31 - +31) <input type='text' name='angle' size='2' /><input type='submit' value='change angle'/></form>";
	html += "<a href='/getCoordinates'>Retrieve coordinates from Kinect via control transfer</a>";
	html += "</body></html>";
 
	res.write(html);
	res.end();
}).listen(8080);
