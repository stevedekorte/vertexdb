#!/usr/local/bin/io

assertEquals := method(a, b, 
	if(a != b, 
		Exception raise(call message argAt(0) .. " == " .. a .. " instead of " .. b)
	)
)


URL with("http://localhost:8080/?method=select&op=rm") fetch

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

// test select keys/values/pairs

assertEquals(URL with("http://localhost:8080/test?method=select&op=keys") fetch, """["_a","_b","_c"]""")
assertEquals(URL with("http://localhost:8080/test?method=select&op=values") fetch, """["1","2","3"]""")

assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs") fetch, """[["_a","1"],["_b","2"],["_c","3"]]""")
assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs&after=_a") fetch,  """[["_b","2"],["_c","3"]]""")
assertEquals(URL with("http://localhost:8080/test?method=select&op=pairs&before=_c") fetch, """[["_b","2"],["_a","1"]]""")
assertEquals(URL with("http://localhost:8080/test?method=size") fetch, "3")

// test rm

URL with("http://localhost:8080/test?method=rm&key=_a") fetch
//assertEquals(URL with("http://localhost:8080/test?method=read&key=_a") fetch, null)
assertEquals(URL with("http://localhost:8080/test?method=size") fetch, "2")

// test select rm

URL with("http://localhost:8080/test?method=write&key=_a") post("1")
URL with("http://localhost:8080/test?method=select&op=rm&after=_a") fetch
assertEquals(URL with("http://localhost:8080/test?method=size") fetch, "1")

/*
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
