{test, wait, next} = require('./test-helper.coffee')

test "It runs functions", -> console.log("function ran")

test "It handles async callbacks", ->
	fn = ->
		console.log("in callback")
		next()
	setTimeout(fn, 500)
	wait(1000)
	
test "Another test runs only after async", ->

test "Times out", -> wait()
		
	
