cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include("ext/hunter/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.12.40.tar.gz"
    SHA1 "0ccc3f3fd571676a1804723984598f9f90a4d6bc"
)

# Build settings
set(PROJECT_NAME_STR tpfpatch_store)
project(${PROJECT_NAME_STR})

include_directories(deps/hdt/libcds-v1.0.12/includes/ deps/hdt/hdt-lib/include/ deps/hdt/hdt-lib/src/dictionary/)

set(HDT_FILES
    #    deps/hdt/hdt-lib/include/Iterator.hpp
    #    deps/hdt/hdt-lib/include/Dictionary.hpp
    #    deps/hdt/hdt-lib/include/HDTSpecification.hpp
    #    deps/hdt/hdt-lib/src/dictionary/PlainDictionary.hpp
        deps/hdt/hdt-lib/src/util/crc16.cpp
        deps/hdt/hdt-lib/src/util/propertyutil.cpp
        deps/hdt/hdt-lib/src/hdt/ControlInformation.cpp
        deps/hdt/hdt-lib/src/hdt/HDTSpecification.cpp
        deps/hdt/hdt-lib/src/dictionary/PlainDictionary.cpp
        )
set(SOURCE_FILES
        src/main/cpp/test.cc)
set(COMMON_FILES
        src/main/cpp/patch/triple.cc src/main/cpp/patch/triple.h
        src/main/cpp/patch/patch_element.cc src/main/cpp/patch/patch_element.h
        src/main/cpp/patch/patch.cc src/main/cpp/patch/patch.h
        src/main/cpp/patch/patch_tree_value.cc src/main/cpp/patch/patch_tree_value.h
        src/main/cpp/patch/patch_tree_key_comparator.cc src/main/cpp/patch/patch_tree_key_comparator.h
        src/main/cpp/patch/patch_tree.cc src/main/cpp/patch/patch_tree.h
        src/main/cpp/patch/patch_tree_iterator.cc src/main/cpp/patch/patch_tree_iterator.h
        src/main/cpp/patch/patch.h
        src/main/cpp/dictionary/dictionary.cc
        )
set(TEST_FILES
        src/test/cpp/patch/triple.cc
        src/test/cpp/patch/patch_element.cc
        src/test/cpp/patch/patch.cc
        src/test/cpp/patch/patch_tree_value.cc
        src/test/cpp/patch/patch_tree_key_comparator.cc
        src/test/cpp/patch/patch_tree.cc
        src/test/cpp/dictionary/dictionary.cc
        )

add_executable(${PROJECT_NAME_STR} ${HDT_FILES} ${COMMON_FILES} ${SOURCE_FILES})

# Add Kyoto Cabinet
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
        target_link_libraries(${PROJECT_NAME_STR} ${KYOTO_SHARED_LIBRARY})
        include_directories(${KYOTO_INCLUDE_DIR})
        add_definitions(-DHAS_KYOTO)
    endif (KYOTO_INCLUDE_DIR MATCHES KYOTO_INCLUDE_DIR-NOTFOUND)
endif (KYOTO_SHARED_LIBRARY MATCHES KYOTO_LIBRARY-NOTFOUND)

# Add Boost
# find_package(Boost CONFIG REQUIRED COMPONENTS container)

# 3rdparty Libraries
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)
#include(CMakeLists-External.txt)

# Add gtest
add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
enable_testing()
set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
include_directories(${GTEST_INCLUDE_DIRS})

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/test/cpp/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_FILES} ${HDT_FILES} ${COMMON_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)

if(NOT WIN32)
    target_link_libraries(${PROJECT_TEST_NAME}
            ${GTEST_LIBS_DIR}/libgtest.a
            ${GTEST_LIBS_DIR}/libgtest_main.a
            )
else()
    target_link_libraries(${PROJECT_TEST_NAME}
            debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FIND_LIBRARY_SUFFIXES}
            optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FIND_LIBRARY_SUFFIXES}
            )
    target_link_libraries(${PROJECT_TEST_NAME}
            debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_FIND_LIBRARY_SUFFIXES}
            optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_FIND_LIBRARY_SUFFIXES}
            )
endif()

target_link_libraries(${PROJECT_TEST_NAME} ${LIBSTDCXX})
target_link_libraries(${PROJECT_TEST_NAME} ${KYOTO_SHARED_LIBRARY})
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(test1 ${PROJECT_TEST_NAME})
