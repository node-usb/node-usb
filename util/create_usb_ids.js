var http = require('http');
var linux_usb = http.createClient(80, 'www.linux-usb.org');
var request = linux_usb.request('GET', '/usb.ids', {'host': 'www.linux-usb.org'});
request.end();

function splitEntry(line) {
	var trimed = line.trim();
	var r = new Array();
	r.push(trimed.substr(0, 4));
	r.push(trimed.substr(5));
	return r;
}

var lastManufacturer = undefined;
var result = "";
var closeManufacturer = false;

request.on('response', function(response) {
	response.on('data', function(chunk) {
		var lines = chunk.toString().split("\n");

		for (var i = 0; i < lines.length; i++) {
			if ((lines[i].trim().length == 0) || (lines[i].trim().charAt(0) == "#")) {
				continue;
			}

			entry = splitEntry(lines[i]);
			
			if (lines[i].charAt(0) != "\t") {
				
				if (lastManufacturer != undefined) {
					closeManufacturer = false;
					result += "    }\n";
					result += "  },\n";
				}

				lastManufacturer = entry[0];
				result += "  \"" + lastManufacturer + "\": {\n";
				result += "    name: \"" + entry[1].trim().replace(/\"/g, "\\\"") + "\",\n";
				result += "    products: {\n";
				closeManufacturer = true;

			} else {
				result += "      \"" + entry[0] + "\": {\n";
				result += "         name: \"" + entry[1] .trim().replace(/\"/g, "\\\"") + "\"\n";
				result += "      },\n";
				
			}
		}
		// TODO Classes etc won't be parsed yet
	});

	response.on('end', function() {
		if (closeManufacturer) {
			result += "    }\n";
			result += "  }\n";
		}

		console.log("exports.usb_ids = {\n" + result + "\n};");
	});
});


