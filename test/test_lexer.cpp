#include <gtest/gtest.h>

#include <sasm/lexer.h>

class TestLexer : public ::testing::Test {
public:
    template <sasm::lexer_token::token_type Type>
    static void CheckSingle(const std::string& content) {
        sasm::reader reader(content);
        sasm::lexer lexer(&reader);

        const auto token = lexer.get();
        EXPECT_EQ(token.type, Type);
        EXPECT_EQ(token.content, content);
        
        EXPECT_TRUE(lexer.get().eof());
    }
};

TEST_F(TestLexer, Empty) {
    sasm::reader reader("");
    sasm::lexer lexer(&reader);

    EXPECT_TRUE(lexer.get().eof());
    
    EXPECT_TRUE(lexer.get().eof());
}

TEST_F(TestLexer, Whitespace) {
    const auto check = &CheckSingle<sasm::lexer_token::whitespace>;
    check("    ");
    check("\t");
}

TEST_F(TestLexer, EndOfLine) {
    const auto check = &CheckSingle<sasm::lexer_token::end_of_line>;
    check("\n");
    check("\r\n");
}

TEST_F(TestLexer, Identifier) {
    const auto check = &CheckSingle<sasm::lexer_token::identifier>;
    check("abcd");
    check("_abcd_");
    check("a1");
    check("_1");
}

TEST_F(TestLexer, OffsetAndWidth) {
    sasm::reader reader(R"(
        	
        ident1    
    )");
    sasm::lexer lexer(&reader);

    // \n
    auto c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::end_of_line);
    EXPECT_EQ(c.content, "\n");
    EXPECT_EQ(c.offset, 0);
    EXPECT_EQ(c.width, 1);

    // "        \t"
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::whitespace);
    EXPECT_EQ(c.content, "        \t");
    EXPECT_EQ(c.offset, 1);
    EXPECT_EQ(c.width, 9);

    // \n
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::end_of_line);
    EXPECT_EQ(c.content, "\n");
    EXPECT_EQ(c.offset, 10);
    EXPECT_EQ(c.width, 1);

    // "        "
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::whitespace);
    EXPECT_EQ(c.content, "        ");
    EXPECT_EQ(c.offset, 11);
    EXPECT_EQ(c.width, 8);

    // "ident1"
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::identifier);
    EXPECT_EQ(c.content, "ident1");
    EXPECT_EQ(c.offset, 19);
    EXPECT_EQ(c.width, 6);

    // "    "
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::whitespace);
    EXPECT_EQ(c.content, "    ");
    EXPECT_EQ(c.offset, 25);
    EXPECT_EQ(c.width, 4);

    // \n
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::end_of_line);
    EXPECT_EQ(c.content, "\n");
    EXPECT_EQ(c.offset, 29);
    EXPECT_EQ(c.width, 1);

    // "    "
    c = lexer.get();
    EXPECT_EQ(c.type, sasm::lexer_token::whitespace);
    EXPECT_EQ(c.content, "    ");
    EXPECT_EQ(c.offset, 30);
    EXPECT_EQ(c.width, 4);

    // end
    EXPECT_TRUE(lexer.get().eof());
}

TEST_F(TestLexer, Comment) {
    {
        sasm::reader reader(";nospace");
        sasm::lexer lexer(&reader);

        const auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::comment);
        EXPECT_EQ(token.content, ";nospace");
        
        EXPECT_TRUE(lexer.get().eof());
    }
    {
        sasm::reader reader("has ; spaces ");
        sasm::lexer lexer(&reader);

        auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_EQ(token.content, "has");
        
        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_EQ(token.content, " ");
        
        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::comment);
        EXPECT_EQ(token.content, "; spaces ");
        
        EXPECT_TRUE(lexer.get().eof());
    }
    {
        sasm::reader reader(";multiple\n;comments");
        sasm::lexer lexer(&reader);

        auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::comment);
        EXPECT_EQ(token.content, ";multiple");
        
        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::end_of_line);
        
        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::comment);
        EXPECT_EQ(token.content, ";comments");
        
        EXPECT_TRUE(lexer.get().eof());

        auto $x = 1;
    }
}

TEST_F(TestLexer, Literal) {
    const auto check = &CheckSingle<sasm::lexer_token::literal>;
    check("3210");
    check("$1a2B");
    check("%10");
}

TEST_F(TestLexer, Keyword) {
    const auto check = &CheckSingle<sasm::lexer_token::keyword>;
    check("X");
    check("Y");
}

TEST_F(TestLexer, Symbol) {
    const auto check = &CheckSingle<sasm::lexer_token::symbol>;
    check(".");
    check(":");
    check("(");
    check(")");
    check(",");
    check("+");
    check("-");
    check("#");
    check("*");
}

TEST_F(TestLexer, Unknown) {
    const auto check = &CheckSingle<sasm::lexer_token::unknown>;
    check("<");
    check("<invalid>");
    {
        sasm::reader reader("<> <>");
        sasm::lexer lexer(&reader);

        auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::unknown);
        EXPECT_EQ(token.content, "<>");

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_EQ(token.content, " ");

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::unknown);
        EXPECT_EQ(token.content, "<>");

        EXPECT_TRUE(lexer.get().eof());
    }
}

TEST_F(TestLexer, FlagWhitespaceBefore) {
    {
        sasm::reader reader("a b\n c d");
        sasm::lexer lexer(&reader);

        auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_FALSE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_FALSE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_TRUE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::end_of_line);
        EXPECT_FALSE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_FALSE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_TRUE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_FALSE(token.whitespace_before);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_TRUE(token.whitespace_before);

        EXPECT_TRUE(lexer.get().eof());
    }
}

TEST_F(TestLexer, FlagFirstOnLine) {
    {
        sasm::reader reader("a a\na a");
        sasm::lexer lexer(&reader);

        auto token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_TRUE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_FALSE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_FALSE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::end_of_line);
        EXPECT_FALSE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_TRUE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::whitespace);
        EXPECT_FALSE(token.first_on_line);

        token = lexer.get();
        EXPECT_EQ(token.type, sasm::lexer_token::identifier);
        EXPECT_FALSE(token.first_on_line);

        EXPECT_TRUE(lexer.get().eof());
    }
}

TEST_F(TestLexer, FlagIsTrivia) {
    const auto check = [] (const std::string& content,
                           bool is_trivia) {
        sasm::reader reader(content);
        sasm::lexer lexer(&reader);

        const auto token = lexer.get();
        EXPECT_EQ(token.content, content);
        EXPECT_EQ(token.is_trivia, is_trivia) << content;
        
        EXPECT_TRUE(lexer.get().end_of_file) << content;
    };
    check("<>", false);     // unknown
    check("", false);       // eof
    check("\n", false);     // eol
    check(" ", true);       // whitespace
    check("ident", false);  // identifier
    check(";comment", true);// comment
    check("10", false);     // literal
    check("X", false);      // keyword
    check("#", false);      // symbol
}
