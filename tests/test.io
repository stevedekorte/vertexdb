#!/usr/local/bin/io

VDBTest := UnitTest clone do(
	setUp := method(
		url := URL with("http://localhost:8080/?action=transaction")
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
	
	VDBAssertion := Object clone do(
		actualBody ::= nil
		expectedBody ::= nil
		actualStatusCode ::= nil
		expectedStatusCode ::= 200
		
		action ::= nil
		variant ::= "default"
		
		baseUrl ::= "http://localhost:8080"
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
		
		url := method(
			URL with(Sequence with(baseUrl, basePath, path, queryString))
		)
		
		assert := method(
			u := url
			setActualBody(u fetch)
			setActualStatusCode(u statusCode)
			
			if(actualStatusCode != expectedStatusCode,
				Exception raise(Sequence with(action, " action failed for \"", variant, "\" variant: \n", u url, "\nexpectedStatusCode ", expectedStatusCode asString, "\nactualStatusCode   ", actualStatusCode asString, "\n"))
			)
			
			if(actualBody != expectedBody,
				Exception raise(Sequence with(action, " action failed for \"", variant, "\" variant: \n", u url, "\nexpectedBody ", expectedBody, "\nactualBody   ", actualBody, "\n"))
			)
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
		KeysAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""["b","c"]""") assert
		KeysAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
	)
	
	PairsAssertion := SelectAssertion clone setOp("pairs")
	
	testSelectPairs := method(
		PairsAssertion clone setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}],["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
		PairsAssertion with("count") addParams("count=1") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
		PairsAssertion with("after") addParams("after=a") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
		PairsAssertion with("before") addParams("before=b") setExpectedBody("""[["a",{"_a":"1","_b":"2","_c":"3"}]]""") assert
		PairsAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""[["b",{"_a":"4","_b":"5","_c":"6"}],["c",{"_a":"7","_b":"5","_c":"9"}]]""") assert
		PairsAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
	)
	
	ValuesAssertion := SelectAssertion clone setOp("values")

	testSelectValues := method(
		ValuesAssertion clone setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"},{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
		ValuesAssertion with("count") addParams("count=1") setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"}]""") assert
		ValuesAssertion with("after") addParams("after=a") setExpectedBody("""[{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
		ValuesAssertion with("before") addParams("before=b") setExpectedBody("""[{"_a":"1","_b":"2","_c":"3"}]""") assert
		ValuesAssertion with("where") addParams("whereKey=_b", "whereValue=5") setExpectedBody("""[{"_a":"4","_b":"5","_c":"6"},{"_a":"7","_b":"5","_c":"9"}]""") assert
		ValuesAssertion with("non-matching where") addParams("whereKey=_a", "whereValue=10") setExpectedBody("[]") assert
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
		URL with("http://localhost:8080/?action=transaction") post(
"/test/queue/waiting?action=mkdir
/test/queue/waiting/a?action=mkdir
/test/queue/waiting/a?action=write&key=_a&value=1
/test/queue/waiting/b?action=mkdir
/test/queue/waiting/b?action=write&key=_a&value=2
/test/queue/active?action=mkdir
")
		QueuePopToAssertion with("first") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("\"a\"") assert
		KeysAssertion with("first queuePopTo") clone setPath("/queue/active/a") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
		
		QueuePopToAssertion with("second") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("\"b\"") assert
		KeysAssertion with("second queuePopTo") clone setPath("/queue/active/b") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
		
		QueuePopToAssertion with("third") setPath("/queue/waiting") addParams("toPath=/test/queue/active") setExpectedBody("null") assert
		SizeAssertion with("third queuePopTo waiting") clone setPath("/queue/waiting") setExpectedBody("0") assert
		SizeAssertion with("third queuePopTo active") clone setPath("/queue/active") setExpectedBody("2") assert
	)
	
	QueueExpireToAssertion := VDBAssertion clone setAction("queueExpireTo")
	testQueueExpireTo := method(
		URL with("http://localhost:8080/?action=transaction") post(
"/test/queue/waiting?action=mkdir
/test/queue/active?action=mkdir
/test/queue/active/a?action=mkdir
/test/queue/active/a?action=write&key=_a&value=1
/test/queue/active/a?action=write&key=_qexpire&value=0000000000
/test/queue/active/a?action=write&key=_qtime&value=0000000000
/test/queue/active/b?action=mkdir
/test/queue/active/b?action=write&key=_a&value=2
/test/queue/active/b?action=write&key=_qexpire&value=2000000000
/test/queue/active/b?action=write&key=_qtime&value=2000000000
")
		QueueExpireToAssertion with("first") setPath("/queue/active") addParams("toPath=/test/queue/waiting") setExpectedBody("1") assert
		ObjectAssertion with("first queueExpireTo") clone setPath("/queue/waiting/a") setExpectedBody("""{"_a":"1"}""") assert
		
		QueueExpireToAssertion with("second") setPath("/queue/active") addParams("toPath=/test/queue/waiting") setExpectedBody("0") assert
		KeysAssertion with("second queueExpireTo") clone setPath("/queue/active/b") setExpectedBody("""["_a","_qexpire","_qtime"]""") assert
	)
)

VDBTest run