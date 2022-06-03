add_library(libsasm reader.cpp lexer.cpp parser_base.cpp parser.cpp dtype.cpp)

target_compile_features(libsasm PRIVATE cxx_std_20)
