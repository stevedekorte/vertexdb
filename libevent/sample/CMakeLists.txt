# -----------------------------------------------------------------------------
# Copyright (c) 2009 Cross Language Inc., All Rights Reserved.
#
# vendor/libevent/sample
# -----------------------------------------------------------------------------

add_executable(event-test event-test.c)
target_link_libraries(event-test ws2_32 libevent)

add_executable(signal-test signal-test.c)
target_link_libraries(signal-test ws2_32 libevent)

if(NOT WIN32)
    add_executable(time-test time-test.c)
    target_link_libraries(time-test libevent)
endif()
