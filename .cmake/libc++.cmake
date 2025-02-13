set(CMAKE_CXX_COMPILER "clang++" CACHE FILEPATH "C++ compiler")
set(CMAKE_C_COMPILER "clang" CACHE FILEPATH "C compiler")
set(CLANG_TIDY "clang-tidy" CACHE FILEPATH "Clang Tidy binary")
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++ -fexperimental-library -fexperimental-modules-reduced-bmi -fcolor-diagnostics" CACHE FILEPATH "Default C++ Flags")
