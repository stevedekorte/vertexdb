// Io is the bottle neck for this test - this is more of a performance sanity check

for(v, 1, 10000,
	URL with("http://localhost:9523?action=write&value=" .. v) fetch
	if(v % 1000 == 0, writeln(v))
)

URL with("http://localhost:9523?action=collectGarbage") fetch
