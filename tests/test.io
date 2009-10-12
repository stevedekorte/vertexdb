#!/usr/local/bin/io

assertEquals := method(a, b, 
	if(a != b, 
		Exception raise(call message argAt(0) .. " == " .. a .. " instead of " .. b)
	)
)

// test size

assertEquals(URL with("http://localhost:8080/?method=size") fetch, "0")

// test mkdir, write

URL with("http://localhost:8080/test?method=mkdir") fetch
URL with("http://localhost:8080/test?method=write&key=_a") post("1")
URL with("http://localhost:8080/test?method=write&key=_b") post("2")
URL with("http://localhost:8080/test?method=write&key=_c") post("3")

// test read

assertEquals(URL with("http://localhost:8080/test?method=read&key=_a") fetch, "\"1\"")
assertEquals(URL with("http://localhost:8080/test?method=read&key=_b") fetch, "\"2\"")
assertEquals(URL with("http://localhost:8080/test?method=read&key=_c") fetch, "\"3\"")

// test select

assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs") fetch, """[["_a","1"],["_b","2"],["_c","3"]]""")
assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs&after=_a") fetch,  """[["_b","2"],["_c","3"]]""")
assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs&before=_c") fetch, """[["_b","2"],["_a","1"]]""")
assertEquals(URL with("http://localhost:8080/test?method=size") fetch, "3")

// test rm

URL with("http://localhost:8080/test?method=rm&key=_a") fetch
//assertEquals(URL with("http://localhost:8080/test?method=read&key=_a") fetch, null)
assertEquals(URL with("http://localhost:8080/test?method=size") fetch, "2")

/*
URL with("http://localhost:8080/test?put=a") post("1")
URL with("http://localhost:8080/test?removeTo=b") fetch
assertEquals(URL with("http://localhost:8080/test?count") fetch, "1")

URL with("http://localhost:8080/?out=test") fetch
assertEquals(URL with("http://localhost:8080/test?count") fetch, "0")
URL with("http://localhost:8080/test/foo?put=a") post("1")
URL with("http://localhost:8080/test/bar?put=a") post("1")
assertEquals(URL with("http://localhost:8080/test?count") fetch, "2")
assertEquals(URL with("http://localhost:8080/test?subpaths") fetch, """[["bar", 1],["foo", 1]]""")

userId := URL with("http://localhost:8080/?newUser") fetch

c1 := URL with("http://localhost:8080/users/" .. userId .. "/items/unseen?count") fetch
c2 := URL with("http://localhost:8080/public/items?count") fetch
assertEquals(c1, c2)
*/

/*
Object squareBrackets := Object getSlot("list")
Object curlyBrackets := Object getSlot("list")

assertEquals(id1, id2)
*/
