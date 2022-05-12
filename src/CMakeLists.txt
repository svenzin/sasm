add_library(libsasm reader.cpp lexer.cpp)

target_compile_features(libsasm PRIVATE cxx_std_20)
