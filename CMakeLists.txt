set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 4.0)

project(AudioProcessorProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

FetchContent_Declare(
  libsndfile
  GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
  GIT_TAG        1.2.2 
)

FetchContent_MakeAvailable(libsndfile)

set(SOURCE_FILES
    src/svf-runner.cpp
    # src/main.cpp
    src/svf.cpp
    src/envelope-follower.cpp
    src/audio_handler.cpp
    # src/cabinet.cpp
    # src/binaural-panner.cpp
    # # src/binaural-panner-runner.cpp
    # src/stereo-to-mono.cpp
)


add_executable(AudioProcessor ${SOURCE_FILES})

target_link_libraries(AudioProcessor PRIVATE sndfile ${FFTW3_LIBRARIES})

# Extra linkage for std::print
if (WIN32)
    target_link_libraries(AudioProcessor PRIVATE -lstdc++exp)
endif()

target_include_directories(AudioProcessor SYSTEM PRIVATE 
    ${libsndfile_SOURCE_DIR}/include
    ${FFTW3_INCLUDE_DIRS}
)

find_program(CLANG_FORMAT_EXE "clang-format")
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_FORMAT_FILES 
        "src/*.cpp" 
        "src/*.h" 
        "src/*.hpp"
    )

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all files in src/..."
    )
else()
    message(WARNING "clang-format not found.")
endif()

find_program(CLANG_TIDY_EXE "clang-tidy")
if(CLANG_TIDY_EXE)
    add_custom_target(
        tidy
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on source files..."
    )
else()
    message(WARNING "clang-tidy not found.")
endif()
