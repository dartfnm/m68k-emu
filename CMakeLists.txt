cmake_minimum_required(VERSION 3.14)

#set(CMAKE_GENERATOR_TOOLSET "ClangCl") # test compilation on clang-c

if(NOT DEFINED PROJECT_NAME)
  set(M68K_IS_TOP_PROJECT TRUE)
  project(m68k-emu LANGUAGES CXX VERSION 0.1.0)
endif()

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(libs/ELFIO EXCLUDE_FROM_ALL)

file(GLOB_RECURSE M68K_SRC "src/*.cpp")
file(GLOB_RECURSE M68K_HDR "include/*.h*")

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${M68K_HDR} ${M68K_SRC})


add_library(
    m68k-emu
    ${M68K_SRC}
	${M68K_HDR}
)

include_directories(include)

target_link_libraries(m68k-emu PRIVATE elfio)

if(MSVC)
    target_compile_options(m68k-emu PRIVATE /W4 /permissive- /MP)
	# Enable Edit and Continue  for Debug builds
    string(REGEX REPLACE "/Z[iI7]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    add_link_options("/INCREMENTAL")
else()
    target_compile_options(m68k-emu PRIVATE -Wall -Wextra -Wpedantic -ggdb)
    target_link_libraries(m68k-emu PRIVATE -static-libgcc -static-libstdc++)
endif()


if(M68K_IS_TOP_PROJECT)
    enable_testing()
    add_subdirectory(test)

    set(CPACK_PROJECT_NAME ${PROJECT_NAME})
    set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
    include(CPack)
endif()
