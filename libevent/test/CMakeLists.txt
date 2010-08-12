# -----------------------------------------------------------------------------
# Copyright (c) 2009 Cross Language Inc., All Rights Reserved.
#
# vendor/libevent/test
# -----------------------------------------------------------------------------

if(NOT WIN32)
    find_package(PythonInterp REQUIRED)

    file(TO_NATIVE_PATH ../event_rpcgen.py EVENT_RPCGEN_PY)
    add_custom_command(
        DEPENDS regress.rpc ../event_rpcgen.py
        OUTPUT  regress.gen.c regress.gen.h
        COMMAND ${PYTHON_EXECUTABLE} ${EVENT_RPCGEN_PY} regress.rpc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    set(REGRESS_EXTRA regress_rpc.c regress.gen.c regress.gen.h)
endif()

add_executable(regress 
    regress.h 
    regress.c 
    regress_dns.c
    regress_http.c 
    ${REGRESS_EXTRA}
    )
target_link_libraries(regress ws2_32 libevent)

if(NOT WIN32)
    add_executable(test_init test-init.c)
    target_link_libraries(test_init ws2_32 libevent)
        
    add_executable(test_eof test-eof.c)
    target_link_libraries(test_eof ws2_32 libevent)

    add_executable(test_weof test-weof.c)
    target_link_libraries(test_weof ws2_32 libevent)

    add_executable(test_time test-time.c)
    target_link_libraries(test_time ws2_32 libevent)

    add_executable(bench bench.c)
    target_link_libraries(bench ws2_32 libevent)
endif()
