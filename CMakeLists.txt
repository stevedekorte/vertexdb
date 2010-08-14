cmake_minimum_required(VERSION 2.8)

project(vertexdb)

add_subdirectory(source/basekit)
add_definitions(-O0 -g)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/")

file(GLOB MAIN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c")
file(GLOB PDB_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/pdb/*.c")
file(GLOB BASEKIT_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/basekit/source/*.c")
file(GLOB STORE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/store/*.c")
file(GLOB HTTPSERVER_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/httpserver/*.c")
file(GLOB BASEKIT_SIMD_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/source/basekit/source/simd_cph/include")

set(SRCS ${MAIN_SRCS} ${PDB_SRCS} ${BASEKIT_SRCS} ${STORE_SRCS} ${HTTPSERVER_SRCS} ${BASEKIT_SIMD_SRCS})

add_executable(vertexdb ${SRCS})

# Initializing libEvent
find_package(Event)
target_link_libraries(vertexdb ${EVENT_LIBRARY})
include_directories(${EVENT_INCLUDE_DIR})

# Initializing TokyoCabinet
find_package(TokyoCabinet)
target_link_libraries(vertexdb ${TOKYOCABINET_LIBRARY})
include_directories(${TOKYOCABINET_INCLUDE_DIR})

# Initializing YAJL
find_package(Yajl)
target_link_libraries(vertexdb ${YAJL_LIBRARY})
include_directories(${YAJL_INCLUDE_DIR})

# Add libdl
find_library( DL_LIB NAMES ${CMAKE_DL_LIBS} )
target_link_libraries(vertexdb ${CMAKE_DL_LIBS})
MESSAGE( STATUS "using dl library: ${DL_LIB}" )

include_directories(source source/pdb source/httpserver source/basekit/source source/store source/basekit/source/simd_cph/include)

# installing vertextdb binary
install(TARGETS vertexdb DESTINATION bin)
