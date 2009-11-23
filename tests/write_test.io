// tests for the write action

assertEquals := method(a, b, 
	if(a != b, 
		Exception raise(call message argAt(0) .. " == " .. a .. " instead of " .. b)
	)
)

base := "http://localhost:8080"

// two ways to write

URL with(base .. "/?action=write&key=_testkey1&value=testval1") fetch
URL with(base .. "/?action=write&key=_testkey2") post("testval2")
assertEquals(URL with(base .. "/?action=read&key=_testkey1") fetch, "\"testval1\"")
assertEquals(URL with(base .. "/?action=read&key=_testkey2") fetch, "\"testval2\"")

// can create empty keys

URL with(base .. "/?action=write&key=_testkey3&value=") fetch
URL with(base .. "/?action=write&key=_testkey4") post("")
assertEquals(URL with(base .. "/?action=read&key=_testkey3") fetch, "\"\"")
assertEquals(URL with(base .. "/?action=read&key=_testkey4") fetch, "\"\"")