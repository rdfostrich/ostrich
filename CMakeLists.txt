cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer") # for memory debugging, use with ASAN_OPTIONS=detect_stack_use_after_return=true detect_leaks=true halt_on_error=false

include(FetchContent)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add HDT
# We may want to declare hdt as a library
include_directories(
        deps/hdt/libcds/include/
        deps/hdt/libhdt/include/
        deps/hdt/libhdt/src/
)
file(GLOB_RECURSE HDT_FILES
        "deps/hdt/libhdt/src/*.h"
        "deps/hdt/libhdt/src/*.cpp"
)

# Build settings
set(PROJECT_NAME_STR ostrich)
project(${PROJECT_NAME_STR})

set(SOURCE_FILE_EVALUATE src/test/cpp/bear.cc)
set(SOURCE_FILE_QUERY_VERSION_MATERIALIZED src/main/cpp/query_version_materialized.cc)
set(SOURCE_FILE_QUERY_DELTA_MATERIALIZED src/main/cpp/query_delta_materialized.cc)
set(SOURCE_FILE_QUERY_VERSION src/main/cpp/query_version.cc)
set(SOURCE_FILE_INSERT src/main/cpp/insert.cc)
set(COMMON_FILES
        src/main/cpp/controller/controller.cc src/main/cpp/controller/controller.h
        src/main/cpp/patch/triple.cc src/main/cpp/patch/triple.h
        src/main/cpp/patch/triple_store.cc src/main/cpp/patch/triple_store.h
        src/main/cpp/patch/patch_element.cc src/main/cpp/patch/patch_element.h
        src/main/cpp/patch/patch.cc src/main/cpp/patch/patch.h
        src/main/cpp/patch/patch_tree_value.cc src/main/cpp/patch/patch_tree_value.h
        src/main/cpp/patch/patch_tree_deletion_value.cc src/main/cpp/patch/patch_tree_deletion_value.h
        src/main/cpp/patch/patch_tree_addition_value.cc src/main/cpp/patch/patch_tree_addition_value.h
        src/main/cpp/patch/patch_tree_key_comparator.cc src/main/cpp/patch/patch_tree_key_comparator.h
        src/main/cpp/patch/patch_tree.cc src/main/cpp/patch/patch_tree.h
        src/main/cpp/patch/patch_tree_iterator.cc src/main/cpp/patch/patch_tree_iterator.h
        src/main/cpp/patch/triple_iterator.cc src/main/cpp/patch/triple_iterator.h
        src/main/cpp/patch/positioned_triple_iterator.cc src/main/cpp/patch/positioned_triple_iterator.h
        src/main/cpp/patch/patch.h
        src/main/cpp/dictionary/dictionary_manager.cc src/main/cpp/dictionary/dictionary_manager.h
        src/main/cpp/snapshot/snapshot_manager.cc src/main/cpp/snapshot/snapshot_manager.h
        src/main/cpp/snapshot/vector_triple_iterator.cc src/main/cpp/snapshot/vector_triple_iterator.h
        src/main/cpp/controller/snapshot_patch_iterator_triple_id.cc src/main/cpp/controller/snapshot_patch_iterator_triple_id.h
        src/main/cpp/patch/patch_tree_manager.cc src/main/cpp/patch/patch_tree_manager.h
        src/main/cpp/snapshot/combined_triple_iterator.cc src/main/cpp/snapshot/combined_triple_iterator.h
        src/main/cpp/patch/patch_element_comparator.cc src/main/cpp/patch/patch_element_comparator.h
        src/main/cpp/evaluate/evaluator.cc src/main/cpp/evaluate/evaluator.h
        src/main/cpp/simpleprogresslistener.cc src/main/cpp/simpleprogresslistener.h
        src/main/cpp/controller/patch_builder.cc src/main/cpp/controller/patch_builder.h
        src/main/cpp/controller/patch_builder_streaming.cc src/main/cpp/controller/patch_builder_streaming.h
        src/main/cpp/controller/triple_delta_iterator.cc src/main/cpp/controller/triple_delta_iterator.h
        src/main/cpp/controller/triple_versions_iterator.cc src/main/cpp/controller/triple_versions_iterator.h
        src/main/cpp/controller/snapshot_creation_strategy.cc src/main/cpp/controller/snapshot_creation_strategy.h
        src/main/cpp/patch/patch_element_iterator.cc src/main/cpp/patch/patch_element_iterator.h
        src/main/cpp/patch/triple_comparator.cc src/main/cpp/patch/triple_comparator.h
        src/main/cpp/controller/metadata_manager.cc src/main/cpp/controller/metadata_manager.h
        src/main/cpp/patch/interval_list.h src/main/cpp/patch/variable_size_integer.h)

set(TEST_FILES
        src/test/cpp/controller/controller.cc
        src/test/cpp/patch/triple.cc
        src/test/cpp/patch/patch_element.cc
        src/test/cpp/patch/patch.cc
        src/test/cpp/patch/patch_tree_addition_value.cc
        src/test/cpp/patch/patch_tree_deletion_value.cc
        src/test/cpp/patch/patch_tree_value.cc
        src/test/cpp/patch/patch_tree_key_comparator.cc
        src/test/cpp/patch/patch_tree.cc
        src/test/cpp/patch/patch_tree_manager.cc
        src/test/cpp/dictionary/dictionary_manager.cc
        src/test/cpp/snapshot/snapshot_manager.cc
        src/test/cpp/patch/interval_list.cc
        src/test/cpp/patch/variable_size_integer.cc)

add_library(ostrich STATIC ${HDT_FILES} ${COMMON_FILES})
target_compile_definitions(ostrich PUBLIC -DCOMPRESSED_ADD_VALUES -DCOMPRESSED_DEL_VALUES -DUSE_VSI -DUSE_VSI_T)
#target_compile_definitions(ostrich PUBLIC -DCOMPRESSED_ADD_VALUES -DCOMPRESSED_DEL_VALUES)
#target_compile_definitions(ostrich PUBLIC -DUSE_VSI -DUSE_VSI_T)


# Kyoto Cabinet dependencies
find_library(LZMA lzma REQUIRED)
find_library(LZO lzo2 REQUIRED)

# Add Kyoto Cabinet
find_library(KYOTO_CABINET libkyotocabinet.a REQUIRED)
find_path(KYOTO_INCLUDE_DIR kcplantdb.h REQUIRED)
include_directories(${KYOTO_INCLUDE_DIR})

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add zlib
find_package(ZLIB REQUIRED)

# Add Boost
find_package(Boost REQUIRED COMPONENTS iostreams)

target_link_libraries(ostrich ${KYOTO_CABINET} ${LZMA} ${LZO})
target_link_libraries(ostrich ZLIB::ZLIB)
target_link_libraries(ostrich Threads::Threads)
target_link_libraries(ostrich Boost::iostreams)

# Evaluation executable
add_executable(${PROJECT_NAME_STR}-evaluate ${SOURCE_FILE_EVALUATE})
target_link_libraries(${PROJECT_NAME_STR}-evaluate ostrich)

# 3rdparty Libraries
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

# Add query version materialized executable
add_executable(${PROJECT_NAME_STR}-query-version-materialized ${SOURCE_FILE_QUERY_VERSION_MATERIALIZED})
target_link_libraries(${PROJECT_NAME_STR}-query-version-materialized ostrich)

# Add query delta materialized executable
add_executable(${PROJECT_NAME_STR}-query-delta-materialized ${SOURCE_FILE_QUERY_DELTA_MATERIALIZED})
target_link_libraries(${PROJECT_NAME_STR}-query-delta-materialized ostrich)

# Add query version executable
add_executable(${PROJECT_NAME_STR}-query-version ${SOURCE_FILE_QUERY_VERSION})
target_link_libraries(${PROJECT_NAME_STR}-query-version ostrich)

# Add insert executable
add_executable(${PROJECT_NAME_STR}-insert ${SOURCE_FILE_INSERT})
target_link_libraries(${PROJECT_NAME_STR}-insert ostrich)

# Add gtest
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/67174c7675fe500644490c49e8c822173f7e1f9a.zip
)
FetchContent_MakeAvailable(googletest)

set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/test/cpp/*.cc)
add_executable(${PROJECT_TEST_NAME} ${TEST_FILES})
target_link_libraries(${PROJECT_TEST_NAME} gtest_main)

if(NOT MSVC)
    set(PThreadLib -pthread)
endif()

target_link_libraries(${PROJECT_TEST_NAME} ostrich)

add_test(test1 ${PROJECT_TEST_NAME})
