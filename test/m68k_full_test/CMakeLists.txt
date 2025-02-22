set(CMAKE_CXX_STANDARD 23)

add_executable(m68k_full_test  full_m68k_test.cpp)

target_link_libraries(m68k_full_test PRIVATE m68k-emu)

