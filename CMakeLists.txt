cmake_minimum_required(VERSION 3.3)
project(tpfpatch_store)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main/cpp/test.cc)

add_executable(tpfpatch_store ${SOURCE_FILES})

find_library(KYOTO_SHARED_LIBRARY kyotocabinet PATHS ${KC_ROOT}/lib)
find_path(KYOTO_INCLUDE_DIR kchashdb.h PATHS ${KC_ROOT}/include)
if (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)
    message("FATAL_ERROR Kyoto Cabinet not found!")
    set(Kyoto-NOTFOUND 1)
    set(Kyoto-FOUND)
else (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)
    if (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
        message("FATAL_ERROR Kyoto Cabinet not found!")
        set(Kyoto-NOTFOUND 1)
        set(Kyoto-FOUND)
    else (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
        set(Kyoto-FOUND 1)
        set(Kyoto-NOTFOUND)
        message(STATUS "Kyoto Cabinet Found")
        # get the static version
        string(REGEX REPLACE "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" ".a" KYOTO_STATIC_LIBRARY ${KYOTO_SHARED_LIBRARY})
        message(STATUS "Kyoto Cabinet: ${KYOTO_STATIC_LIBRARY}" )
        target_link_libraries(tpfpatch_store ${KYOTO_SHARED_LIBRARY})
        include_directories(${KYOTO_INCLUDE_DIR})
        add_definitions(-DHAS_KYOTO)
    endif (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
endif (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)

