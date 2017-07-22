cmake_minimum_required(VERSION 2.8.2)

project(Flit)

include(CTest)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-deprecated-declarations -O3 -Wall -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=haswell -fno-omit-frame-pointer -fPIC")

if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(DownloadProject.cmake)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

download_project(PROJ benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG f662e8be5bc9d40640e10b72092780b401612bf2
  ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})

# When using CMake 2.8.11 or later, header path dependencies
# are automatically added to the gtest and gmock targets.
# For earlier CMake versions, we have to explicitly add the
# required directories to the header search path ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include"
                        "${gmock_SOURCE_DIR}/include")
endif()

add_executable(flit_test flit_test.cc)
target_link_libraries(flit_test gtest benchmark)
add_test(NAME flit_test COMMAND flit_test)
