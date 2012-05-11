test_queue = []
waiting = false

exports.test = test = (desc, func) ->
	test_queue.push {desc, func}
	if test_queue.length == 1 then start()
	
exports.wait = wait = (timeout=1000) ->
	if not waiting
		waiting = setTimeout((-> console.error("Test timed out")), timeout)
	
exports.next = next = ->
	clearTimeout(waiting)
	waiting = false
	test_queue.shift()
	start()
	
start = ->
	t = test_queue[0]
	if t
		console.info("Testing #{t.desc}")	
		t.func.call(undefined)
		if not waiting then next()

