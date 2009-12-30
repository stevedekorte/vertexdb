#!/usr/local/bin/io

VDBAssertion := Object clone do(
    actualBody ::= nil
    expectedBody ::= nil
    actualStatusCode ::= nil
    expectedStatusCode ::= 200
    
    action ::= nil
    variant ::= "default"
    
	port ::= "9523" //DONT CHANGE THIS.  WE WOULDN'T WANT TO ACCIDENTALLY WIPE OUT THE PRODUCTION DB
	host ::= "localhost"
	writeln("running tests on ", host, ":", port)
    baseUrl ::= method("http://" .. host .. ":" .. port)
    basePath ::= "/test"
    path ::= ""
    params ::= List clone
    
    with := method(aVariant,
        self clone setVariant(aVariant)
    )
    
    init := method(
        resend
        params = params clone
    )
    
    queryString := method(
        str := ("?action=" .. action) asMutable
        if(params size > 0,
            str appendSeq("&") appendSeq(params join("&"))
        )
        str
    )
    
    addParams := method(
        params appendSeq(call evalArgs)
        self
    )
    
    url ::= method(
        setUrl(URL with(Sequence with(baseUrl, basePath, path, queryString))) url
    )
    
	raise := method(m,
		URL with(baseUrl .. "?action=log") post(m)
        //Exception raise(m)
		writeln(m)
		Coroutine currentCoro backTraceString println
		System exit
	)

    assert := method(
        setActualBody(url fetch)
        setActualStatusCode(url statusCode)
        
		assertStatusCode
		assertBody
    )

	assertStatusCode := method(
		if(actualStatusCode != expectedStatusCode,
			m := Sequence with(action, " action failed for \"", variant, "\" variant: \n", url url, "\nexpectedStatusCode ", expectedStatusCode asString, "\nactualStatusCode   ", actualStatusCode asString, "\nactualBody         ", actualBody)
			raise(m)
        )
	)
	
	assertBody := method(
		if(actualBody != expectedBody,
            m := Sequence with(action, " action failed for \"", variant, "\" variant: \n", url url, "\nexpectedBody ", expectedBody, "\nactualBody   ", actualBody, "\n")
			raise(m)
        )
	)
)

VDBTest := UnitTest clone do(
    setUp := method(
        url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        result := url post("/?action=select&op=rm
/test/a?action=mkdir
/test/a?action=write&key=_a&value=1
/test/a?action=write&key=_b&value=2
/test/a?action=write&key=_c&value=3
/test/b?action=mkdir
/test/b?action=write&key=_a&value=4
/test/b?action=write&key=_b&value=5
/test/b?action=write&key=_c&value=6
/test/c?action=mkdir
/test/c?action=write&key=_a&value=7
/test/c?action=write&key=_b&value=5
/test/c?action=write&key=_c&value=9")
        if(url statusCode == 500,
            Exception raise("Error in transaction setting up VDBTest: " .. result)
        )
    )

    //reads
    ReadAssertion := VDBAssertion clone setAction("read")
    
    testRead := method(
        ReadAssertion clone setPath("/a") addParams("key=_a") setExpectedBody("\"1\"") assert
        ReadAssertion clone setPath("/a") addParams("key=_b") setExpectedBody("\"2\"") assert
        ReadAssertion clone setPath("/a") addParams("key=_c") setExpectedBody("\"3\"") assert
        ReadAssertion with("missing key") setPath("/a") addParams("key=_d") setExpectedBody("null") assert
        ReadAssertion with("bad path") setPath("/d") addParams("key=_d") setExpectedBody("\"path does not exist: test/d\"") setExpectedStatusCode(500) assert
    )
    
    SizeAssertion := VDBAssertion clone setAction("size")
    
    testSize := method(
        SizeAssertion clone setExpectedBody("3") assert
    )

    SelectAssertion := VDBAssertion clone do(
        action ::= "select"
        op ::= nil
        
        queryString := method(
            str := resend
            str .. "&op=" .. op
        )
    )
    
    KeysAssertion := SelectAssertion clone setOp("keys")


    testSelectKeys := method(
        KeysAssertion clone setExpectedBody("""["a","b","c"]""") assert
        KeysAssertion with("count") addParams("count=1") setExpectedBody("""["a"]""") assert
        KeysAssertion with("after") addParams("after=a") setExpectedBody("""["b","c"]""") assert
        KeysAssertion with("before") addParams("before=b") setExpectedBody("""["a"]""") assert
		KeysAssertion with("out of range") addParams("after=a", "before=d") setExpectedBody("""["b","c"]""") assert
		KeysAssertion with("out of lower range") addParams("after=@", "before=c") setExpectedBody("""["a","b"]""") assert
		KeysAssertion with("out of lower and upper range") addParams("after=@", "before=d") setExpectedBody("""["a","b","c"]""") assert
		KeysAssertion with("out of upper, upper range") addParams("after=d", "before=f") setExpectedBody("""[]""") assert
        KeysAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""["b","c"]""") assert
		KeysAssertion with("matching range, non matching where") addParams("after=@", "before=b", "whereKey=_a", "whereValue=-1") setExpectedBody("""[]""") assert
        KeysAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
    )
    
    PairsAssertion := SelectAssertion clone setOp("pairs")
    
    testSelectPairs := method(
        PairsAssertion clone setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}],["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        PairsAssertion with("count") addParams("count=1") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
        PairsAssertion with("after") addParams("after=a") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        PairsAssertion with("before") addParams("before=b") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
		PairsAssertion with("out of range") addParams("after=a", "before=d") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        PairsAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        PairsAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
    )
    
    ValuesAssertion := SelectAssertion clone setOp("values")

    testSelectValues := method(
        ValuesAssertion clone setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"},{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
        ValuesAssertion with("count") addParams("count=1") setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"}]""") assert
        ValuesAssertion with("after") addParams("after=a") setExpectedBody("""[{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
        ValuesAssertion with("before") addParams("before=b") setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"}]""") assert
		ValuesAssertion with("out of range") addParams("after=a", "before=d") setExpectedBody("""[{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
        ValuesAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""[{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
        ValuesAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
    )

	testSelectValuesWithBeforeUsingTimstamps := method(
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
	    result := url post("/?action=select&op=rm
/test/2009-10-28@19:49:54.426298?action=mkdir
/test/2009-10-28@20:50:37.286256?action=mkdir
/test/2009-10-28@21:50:40.701332?action=mkdir
/test/2009-10-28@22:50:43.140329?action=mkdir
/test/2009-10-29@04:21:29.198273?action=mkdir
/test/2009-10-29@05:21:31.891355?action=mkdir
/test/2009-10-29@10:34:43.125098?action=mkdir
/test/2009-10-29@11:34:46.198658?action=mkdir
/test/2009-10-29@16:34:57.237118?action=mkdir
/test/2009-10-29@17:35:00.146650?action=mkdir")
	    if(url statusCode == 500,
	        Exception raise("Error in transaction setting up testSelectValuesWithBeforeUsingTimstamps: " .. result)
	    )

		ValuesAssertion with("before") addParams("before=ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ", "count=1") setExpectedBody("[{}]") assert
	)

	CountAssertion := SelectAssertion clone setOp("count")

    testSelectCount := method(
        CountAssertion clone setExpectedBody("3") assert
        CountAssertion with("count") addParams("count=1") setExpectedBody("1") assert
        CountAssertion with("after") addParams("after=a") setExpectedBody("2") assert
        CountAssertion with("before") addParams("before=b") setExpectedBody("1") assert
		CountAssertion with("out of range") addParams("after=a", "before=d") setExpectedBody("2") assert
        CountAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("2") assert
        CountAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("0") assert
    )

	testSelectCountWithAfterUsingTimstamps := method(
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        result := url post("/?action=select&op=rm
/test/2009-10-28@19:49:54.426298?action=mkdir
/test/2009-10-28@20:50:37.286256?action=mkdir
/test/2009-10-28@21:50:40.701332?action=mkdir
/test/2009-10-28@22:50:43.140329?action=mkdir
/test/2009-10-29@04:21:29.198273?action=mkdir
/test/2009-10-29@05:21:31.891355?action=mkdir
/test/2009-10-29@10:34:43.125098?action=mkdir
/test/2009-10-29@11:34:46.198658?action=mkdir
/test/2009-10-29@16:34:57.237118?action=mkdir
/test/2009-10-29@17:35:00.146650?action=mkdir")
        if(url statusCode == 500,
            Exception raise("Error in transaction setting up testSelectCountWithAfterUsingTimstamps: " .. result)
        )

		CountAssertion with("after") addParams("after=2009-10-29@12:46:17.533484") setExpectedBody("2") assert
	)
	
	testSelectCountWithBeforeAndAfter := method(
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
		result := url post("/?action=select&op=rm
/test/2009-10-31@07:03:54.641231?action=mkdir")
		if(url statusCode == 500,
		    Exception raise("Error in transaction setting up testSelectCountWithBeforeAndAfter: " .. result)
		)
		
		CountAssertion addParams("after=2009-10-17@07:14:11.547574", "before=2009-10-31@06:44:11.547646") setExpectedBody("0") assert
	)
    
    SizesAssertion := SelectAssertion clone setOp("sizes")

    testSelectSizes := method(
        SizesAssertion clone setExpectedBody("""{"a":3,"b":3,"c":3}""") assert
        SizesAssertion with("count") addParams("count=1") setExpectedBody("""{"a":3}""") assert
        SizesAssertion with("after") addParams("after=a") setExpectedBody("""{"b":3,"c":3}""") assert
        SizesAssertion with("before") addParams("before=b") setExpectedBody("""{"a":3}""") assert
        SizesAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""{"b":3,"c":3}""") assert
        SizesAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("{}") assert
    )
    
    RmAssertion := SelectAssertion clone setOp("rm")

    testSelectRm := method(
        RmAssertion clone setExpectedBody("3") assert
        PairsAssertion with("rm") setExpectedBody("[]") assert
        setUp
        
        RmAssertion with("count") addParams("count=1") setExpectedBody("1") assert
        PairsAssertion with("rm count") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        setUp
        
        RmAssertion with("after") addParams("after=a") setExpectedBody("2") assert
        PairsAssertion with("rm after") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
        setUp
        
        RmAssertion with("before") addParams("before=b") setExpectedBody("1") assert
        PairsAssertion with("rm before") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
        setUp
        
        RmAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("2") assert
        PairsAssertion with("rm where") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
        setUp
        
        RmAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("0") assert
        PairsAssertion with("rm non-matching where") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}],["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
    )

    ObjectAssertion := SelectAssertion clone setOp("object")

    testSelectObject := method(
        ObjectAssertion clone setPath("/a") setExpectedBody("""{"_a":"1","_b":"2","_c":"3"}""") assert
    )
    
    //writes
    
    WriteAssertion := VDBAssertion clone setAction("write")
    testWrite := method(
        WriteAssertion clone setPath("/a") addParams("key=_d", "value=4") setExpectedBody("null") assert
        ObjectAssertion with("write") clone setPath("/a") setExpectedBody("""{"_a":"1","_b":"2","_c":"3","_d":"4"}""") assert
        
        WriteAssertion with("overwrite") setPath("/a") addParams("key=_d", "value=5") setExpectedBody("null") assert
        ObjectAssertion with("write") clone setPath("/a") setExpectedBody("""{"_a":"1","_b":"2","_c":"3","_d":"5"}""") assert
        
        WriteAssertion with("write append") setPath("/a") addParams("key=_d", "value=6", "mode=append") setExpectedBody("null") assert
        ObjectAssertion with("write") clone setPath("/a") setExpectedBody("""{"_a":"1","_b":"2","_c":"3","_d":"56"}""") assert
    )
    
    LinkAssertion := VDBAssertion clone setAction("link")
    testLink := method(
        LinkAssertion clone setPath("/a") addParams("toPath=/test", "key=d") setExpectedBody("null") assert
        ObjectAssertion with("link") clone setPath("/a") setExpectedBody("""{"_a":"1","_b":"2","_c":"3"}""") assert
    )
    
    QueuePopToAssertion := VDBAssertion clone setAction("queuePopTo")
    testQueuePopTo := method(
        u := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        r := u post(
"/test/queue/waiting?action=mkdir
/test/queue/waiting/a?action=mkdir
/test/queue/waiting/a?action=write&key=_a&value=1
/test/queue/waiting/b?action=mkdir
/test/queue/waiting/b?action=write&key=_a&value=2
/test/queue/active?action=mkdir")

        if(u statusCode != 200,
            Exception raise("setup transaction fails in testQueuePopTo: ", r)
        )

        QueuePopToAssertion with("first") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("\"a\"") assert
        KeysAssertion with("first queuePopTo") clone setPath("/queue/active/a") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
        
        QueuePopToAssertion with("second") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("\"b\"") assert
        KeysAssertion with("second queuePopTo") clone setPath("/queue/active/b") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
        
        QueuePopToAssertion with("third") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("null") assert
        SizeAssertion with("third queuePopTo waiting") clone setPath("/queue/waiting") setExpectedBody("0") assert
        SizeAssertion with("third queuePopTo active") clone setPath("/queue/active") setExpectedBody("2") assert
    )

	testQueuePopToWithTimestamps := method(
		return
		
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        result := url post("/?action=select&op=rm
/test/active?action=mkdir
/test/waiting/2009-10-28@19:49:54.426298?action=mkdir
/test/waiting/2009-10-28@20:50:37.286256?action=mkdir
/test/waiting/2009-10-28@21:50:40.701332?action=mkdir
/test/waiting/2009-10-28@22:50:43.140329?action=mkdir
/test/waiting/2009-10-29@04:21:29.198273?action=mkdir
/test/waiting/2009-10-29@05:21:31.891355?action=mkdir
/test/waiting/2009-10-29@10:34:43.125098?action=mkdir
/test/waiting/2009-10-29@11:34:46.198658?action=mkdir
/test/waiting/2009-10-29@16:34:57.237118?action=mkdir
/test/waiting/2009-10-29@17:35:00.146650?action=mkdir")
        if(url statusCode == 500,
            Exception raise("Error in transaction setting up testQueuePopToWithTimestamps: " .. result)
        )

	    QueuePopToAssertion clone setPath("/waiting") addParams("toPath=/test/active") setExpectedBody("\"2009-10-28@19:49:54.426298\"") assert
		SizeAssertion clone setPath("/active") setExpectedBody("1") assert
		SizeAssertion clone setPath("/waiting") setExpectedBody("9") assert
	)
	
	testQueueExpireToShouldNotEmptyToPath := method(
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        result := url post("/?action=select&op=rm
/test/active?action=mkdir
/test/waiting/2009-10-28@19:49:54.426298?action=mkdir
/test/waiting/2009-10-28@20:50:37.286256?action=mkdir
/test/waiting/2009-10-28@21:50:40.701332?action=mkdir
/test/waiting/2009-10-28@22:50:43.140329?action=mkdir
/test/waiting/2009-10-29@04:21:29.198273?action=mkdir
/test/waiting/2009-10-29@05:21:31.891355?action=mkdir
/test/waiting/2009-10-29@10:34:43.125098?action=mkdir
/test/waiting/2009-10-29@11:34:46.198658?action=mkdir
/test/waiting/2009-10-29@16:34:57.237118?action=mkdir
/test/waiting/2009-10-29@17:35:00.146650?action=mkdir")
        if(url statusCode == 500,
            Exception raise("Error in transaction setting up testQueuePopToWithTimestamps: " .. result)
        )

	    QueueExpireToAssertion clone setPath("/active") addParams("toPath=/test/waiting") setExpectedBody("0") assert
		SizeAssertion clone setPath("/active") setExpectedBody("0") assert
		SizeAssertion clone setPath("/waiting") setExpectedBody("10") assert
	)

    QueueExpireToAssertion := VDBAssertion clone setAction("queueExpireTo")
    testQueueExpireTo := method(
        u := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        r := u post(
"/test/queue/waiting?action=mkdir
/test/queue/active?action=mkdir
/test/queue/active/a?action=mkdir
/test/queue/active/a?action=write&key=_a&value=1
/test/queue/active/a?action=write&key=_qexpire&value=0000000000
/test/queue/active/a?action=write&key=_qtime&value=0000000000
/test/queue/active/b?action=mkdir
/test/queue/active/b?action=write&key=_a&value=2
/test/queue/active/b?action=write&key=_qexpire&value=2000000000
/test/queue/active/b?action=write&key=_qtime&value=2000000000")

        if(u statusCode != 200,
            Exception raise("setup transaction fails in testQueueExpireTo: ", r)
        )
		//assert(URL with("http://localhost:9523/test/active?action=queueExpireTo&toPath=/test/waiting") fetch == "1")
        QueueExpireToAssertion with("first") setPath("/queue/active") addParams("toPath=/test/queue/waiting") setExpectedBody("1") assert
        ObjectAssertion with("first queueExpireTo") setPath("/queue/waiting/a") setExpectedBody("""{"_a":"1"}""") assert
        QueueExpireToAssertion with("second") setPath("/queue/active") addParams("toPath=/test/queue/waiting") setExpectedBody("0") assert
        KeysAssertion with("second queueExpireTo") setPath("/queue/active/b") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
    )

    testAmchart := method(
        u := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        r := u post(
"/test/queue/waiting?action=mkdir
/test/queue/active?action=mkdir
/test/queue/active/a?action=mkdir
/test/queue/active/a?action=write&key=_a&value=1
/test/queue/active/a?action=write&key=_qexpire&value=0000000000
/test/queue/active/a?action=write&key=_qtime&value=0000000000
/test/queue/active/b?action=mkdir
/test/queue/active/b?action=write&key=_a&value=2
/test/queue/active/b?action=write&key=_qexpire&value=2000000000
/test/queue/active/b?action=write&key=_qtime&value=2000000000")

        if(u statusCode != 200,
            Exception raise("setup transaction fails in testAmchart: ", r)
        )
		
		u := URL with(VDBAssertion baseUrl .. "/?action=amchart") 
		u fetch
		if(u statusCode != 200,
            Exception raise("error in amchart fetch")
		)
    )

    testView := method(
        u := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        r := u post(
"/test/queue/waiting?action=mkdir
/test/queue/active?action=mkdir
/test/queue/active/a?action=mkdir
/test/queue/active/a?action=write&key=_a&value=1
/test/queue/active/a?action=write&key=_qexpire&value=0000000000
/test/queue/active/a?action=write&key=_qtime&value=0000000000
/test/queue/active/b?action=mkdir
/test/queue/active/b?action=write&key=_a&value=2
/test/queue/active/b?action=write&key=_qexpire&value=2000000000
/test/queue/active/b?action=write&key=_qtime&value=2000000000")

        if(u statusCode != 200,
            Exception raise("setup transaction fails in testView: ", r)
        )

		u := URL with(VDBAssertion baseUrl .. "/") 
		u fetch
		if(u statusCode != 200,
            Exception raise("error in view fetch")
		)
    )

    testMeta := method(
		URL with(VDBAssertion baseUrl .. "/?action=write&key=_type&value=Root&mode=meta") fetch
        r := URL with(VDBAssertion baseUrl .. "/?action=read&key=_type&mode=meta") fetch
		if(r containsSeq("Root") not, Exception raise("meta data not read"))
    )

	testSpaceInPath := method(
		URL with(VDBAssertion baseUrl .. "/test/spacedPath/foo%20bar?action=mkdir") fetch
		URL with(VDBAssertion baseUrl .. "/test/spacedPath/foo%20bar?action=write&key=_baz&value=bam") fetch
		VDBAssertion clone setVariant("space in path") setPath("/spacedPath/foo%20bar") setAction("read") addParams("key=_baz") setExpectedBody("\"bam\"") assert
		VDBAssertion clone setVariant("space in keys") setPath("/spacedPath") setAction("select") addParams("op=keys") setExpectedBody("""["foo bar"]""") assert
	)

	testAMChart := method(
		r := URL with(VDBAssertion baseUrl .. "/?action=amchart") fetch
		if(r containsSeq("graph") not, Exception raise("chart error"))
		
		r := URL with(VDBAssertion baseUrl .. "/?action=amchart&slot1=bar") fetch
		if(r containsSeq("graph") not, Exception raise("chart error"))
		
		r := URL with(VDBAssertion baseUrl .. "/?action=amchart&subpath=foo&slot1=bar") fetch
		if(r containsSeq("graph") not, Exception raise("chart error"))
	)
)

assert := method(v, v ifFalse(Exception raise("error")))

CollectGarbageTest := UnitTest clone do(
    CollectGarbageAssertion := VDBAssertion clone setAction("collectGarbage") do(
		expectedBodyPrefix ::= nil
		
    	assertBody := method(
    /*
		if(actualBody beginsWithSeq(expectedBodyPrefix) not,
	            m := Sequence with(action, " action failed for \"", variant, "\" variant: \n", url url, "\nexpectedBodyPrefix ", expectedBodyPrefix, "\nactualBody         ", actualBody, "\n")
				raise(m)
	        )
	*/
    	)
    )
    testCollectGarbage := method(
		URL with(VDBAssertion baseUrl .. "/?action=select&op=rm") fetch
		URL with(VDBAssertion baseUrl .. "/?action=collectGarbage") fetch
        URL with(VDBAssertion baseUrl .. "/a/b?action=mkdir") fetch
        URL with(VDBAssertion baseUrl .. "/a/c?action=mkdir") fetch
        URL with(VDBAssertion baseUrl .. "/a/d?action=mkdir") fetch
        URL with(VDBAssertion baseUrl .. "/a?action=select&op=rm") fetch
        CollectGarbageAssertion setBasePath("/") setExpectedBodyPrefix("""{"saved":2,"seconds":""") assert
    )

	testWriteDuringCollection := method(
		URL with(VDBAssertion baseUrl .. "/?action=select&op=rm") fetch
		URL with(VDBAssertion baseUrl .. "/test/gc?action=mkdir") fetch
		
		nodes := List clone
		1000 repeat(i,
			nodes append("/test/gc/" .. i .. "?action=mkdir")
			nodes append("/test/gc/" .. i .. "?action=write&key=_a&value=b")
		)
		
		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
        url post(nodes join("\n"))

		URL with(VDBAssertion baseUrl .. "/?action=collectGarbage") fetch
        if(File with("/tmp/db.tc") exists not,
        	Exception raise("Expected /tmp/db.tc to exist.  Perhaps collect finished already?")
        )

		url := URL with(VDBAssertion baseUrl .. "/?action=transaction")
		url post("/test/gc/9999999999?action=mkdir\n/test/gc/9999999999?action=write&key=_b&value=c")

		while(File with("/tmp/db.tc") exists,
			System sleep(.1)
		)
		
		if(URL with(VDBAssertion baseUrl .. "/test/gc/9999999999?action=read&key=_b") fetch != "\"c\"",
			Exception raise("Expected /test/gc/9999999999 to contain slot _b with value c after GC")
		)
	)
)

VDBTest run
CollectGarbageTest run