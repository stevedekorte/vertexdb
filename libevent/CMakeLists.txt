# -----------------------------------------------------------------------------
# Copyright (c) 2009 Cross Language Inc., All Rights Reserved.
#
# vendor/libevent
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

project(libevent)

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf directories. 
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# -----------------------------------------------------------------------------

if(WIN32)
    include_directories(BEFORE WIN32-Code compat .)
    add_definitions(/DHAVE_CONFIG_H /D_CRT_SECURE_NO_DEPRECATE)
    set(EXTRA_FILES WIN32-Code/win32.c WIN32-Code/config.h WIN32-Code/event-config.h)
endif()

add_library(libevent STATIC
    buffer.c
    evbuffer.c
    evdns.c
    event.c
    event_tagging.c
    evrpc.c
    evutil.c
    http.c
    log.c
    signal.c
    strlcpy.c
    evdns.h
    event-internal.h
    event.h
    evhttp.h
    evrpc-internal.h
    evrpc.h
    evsignal.h
    evutil.h
    http-internal.h
    min_heap.h
    strlcpy-internal.h
    ${EXTRA_FILES}
    )

foreach(HDR event.h evdns.h evrpc.h evutil.h evsignal.h)
    CONFIGURE_FILE(${HDR} ${PROJECT_BINARY_DIR}/include/libevent/${HDR} COPYONLY)
endforeach()

add_subdirectory(test)
add_subdirectory(sample)
