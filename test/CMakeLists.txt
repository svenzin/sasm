add_executable(test_runner test_reader.cpp test_lexer.cpp test_parser.cpp)

target_compile_features(test_runner PRIVATE cxx_std_20)

target_link_libraries(test_runner libsasm gtest gtest_main)
