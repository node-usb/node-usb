var express = require('express');
var app = express.createServer();
var ejs = require('ejs');
var TonelabMiddleware = require('./tonelab-middleware').TonelabMiddleware;
var tonelab = new TonelabMiddleware();

app.configure(function() {
	app.use(express.methodOverride());
	app.use(express.bodyDecoder());
	app.use(app.router);
	app.use(express.staticProvider(__dirname + '/public'));
})

app.configure('dev', function() {
	app.use(express.errorHandler({ dumpExceptions: true, showstack: true}));
});

app.configure('production', function() {
	app.use(express.errorHandler());
});

app.set('view engine', 'ejs');

app.get('/', function(req, res) {
	res.render("index.ejs", {
			layout: false,
			locals: tonelab.getDefaults()
	});
});

app.put('/slot/:slot', function(req, res) {
	var data = JSON.parse(req.rawBody);
	var errors = tonelab.validateSettings(data);
	var r = {"has_errors": false};

	if (errors.length > 0) {
		r.has_errors = true;
		r.errors = errors;
	}

	res.send(JSON.stringify(r));
});

app.get('/slot/:slot', function(req, res) {
});


app.listen(8000);

