#include <gtest/gtest.h>

#include <sasm/expression.h>

using namespace sasm::operations;

class TestExpression : public ::testing::Test {
public:
    struct test_parser : public sasm::parser_base_t {
        sasm::reader m_reader;
        sasm::lexer m_lexer;
        explicit test_parser(const std::string& content)
        : m_reader(content)
        , m_lexer(&m_reader)
        , parser_base_t(&m_lexer)
        {}

        sasm::lexer_token get() {
            const auto token = stage_token();
            accept();
            return token;
        }
    };

    void CheckValue(const sasm::expression_item_t& item, int value) {
        ASSERT_TRUE(item.is<sasm::expression_item_t::value>());
        EXPECT_EQ(item.val, value);
    }

    void CheckReference(const sasm::expression_item_t& item, const std::string& reference) {
        ASSERT_TRUE(item.is<sasm::expression_item_t::reference>());
        EXPECT_EQ(item.ref, reference);
    }

    void CheckOperation(const sasm::expression_item_t& item, const sasm::operation_t& operation) {
        ASSERT_TRUE(item.is<sasm::expression_item_t::operation>());
        EXPECT_EQ(item.op.execute, operation.execute);
        EXPECT_EQ(item.op.precedence, operation.precedence);
        EXPECT_EQ(item.op.is_left_associative, operation.is_left_associative);
    }

};

TEST_F(TestExpression, Empty) {
    test_parser parser("");
    sasm::expression_t expr;
    EXPECT_FALSE(sasm::try_parse_expression(parser, expr));
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestExpression, Value) {
    test_parser parser("10");
    sasm::expression_t expr;
    EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
    ASSERT_TRUE(expr.is_value());
    EXPECT_EQ(expr.get_value(), 10);
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestExpression, Reference) {
    test_parser parser("REF");
    sasm::expression_t expr;
    EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
    ASSERT_TRUE(expr.is_reference());
    EXPECT_EQ(expr.get_reference(), "REF");
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestExpression, Chained) {
    test_parser parser("REF, REF");
    {
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_reference());
        EXPECT_EQ(expr.get_reference(), "REF");
    }
    ASSERT_TRUE(parser.get().is<sasm::lexer_token::symbol>(","));
    {
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_reference());
        EXPECT_EQ(expr.get_reference(), "REF");
    }
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestExpression, Simple) {
    // expect { A, B, C, -, + }
    test_parser parser("A + B - C");
    sasm::expression_t expr;
    EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
    ASSERT_TRUE(expr.is_expression());
    ASSERT_EQ(expr.content.size(), 5);
    CheckReference(expr.content[0], "A");
    CheckReference(expr.content[1], "B");
    CheckReference(expr.content[2], "C");
    CheckOperation(expr.content[3], subtraction);
    CheckOperation(expr.content[4], addition);
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestExpression, SimpleWithPrecedence) {
    {
        // expect { A, B, C, *, D, +, + }
        test_parser parser("A + B * C + D");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 7);
        CheckReference(expr.content[0], "A");
        CheckReference(expr.content[1], "B");
        CheckReference(expr.content[2], "C");
        CheckOperation(expr.content[3], multiplication);
        CheckReference(expr.content[4], "D");
        CheckOperation(expr.content[5], addition);
        CheckOperation(expr.content[6], addition);
        EXPECT_TRUE(parser.get().eof());
    }
    {
        // expect { A, B, *, C, D, *, + }
        test_parser parser("A * B + C * D");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 7);
        CheckReference(expr.content[0], "A");
        CheckReference(expr.content[1], "B");
        CheckOperation(expr.content[2], multiplication);
        CheckReference(expr.content[3], "C");
        CheckReference(expr.content[4], "D");
        CheckOperation(expr.content[5], multiplication);
        CheckOperation(expr.content[6], addition);
        EXPECT_TRUE(parser.get().eof());
    }
}

TEST_F(TestExpression, SimpleWithParenthesis) {
    {
        // expect { A, B, +, C, D, +, * }
        test_parser parser("(A + B) * (C + D)");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 7);
        CheckReference(expr.content[0], "A");
        CheckReference(expr.content[1], "B");
        CheckOperation(expr.content[2], addition);
        CheckReference(expr.content[3], "C");
        CheckReference(expr.content[4], "D");
        CheckOperation(expr.content[5], addition);
        CheckOperation(expr.content[6], multiplication);
        EXPECT_TRUE(parser.get().eof());
    }
    {
        // expect { A, B, C, +, D, *, * }
        test_parser parser("A * (B + C) * D");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 7);
        CheckReference(expr.content[0], "A");
        CheckReference(expr.content[1], "B");
        CheckReference(expr.content[2], "C");
        CheckOperation(expr.content[3], addition);
        CheckReference(expr.content[4], "D");
        CheckOperation(expr.content[5], multiplication);
        CheckOperation(expr.content[6], multiplication);
        EXPECT_TRUE(parser.get().eof());
    }
}

TEST_F(TestExpression, Unary) {
    {
        test_parser parser("-A");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 2);
        CheckReference(expr.content[0], "A");
        CheckOperation(expr.content[1], negation);
        EXPECT_TRUE(parser.get().eof());
    }
    {
        test_parser parser("A + -B");
        sasm::expression_t expr;
        EXPECT_TRUE(sasm::try_parse_expression(parser, expr));
        ASSERT_TRUE(expr.is_expression());
        ASSERT_EQ(expr.content.size(), 4);
        CheckReference(expr.content[0], "A");
        CheckReference(expr.content[1], "B");
        CheckOperation(expr.content[2], negation);
        CheckOperation(expr.content[3], addition);
        EXPECT_TRUE(parser.get().eof());
    }
}

TEST_F(TestExpression, Failure) {
    const auto check = [](const std::string& expression) {
        test_parser parser(expression);
        sasm::expression_t expr;
        EXPECT_FALSE(sasm::try_parse_expression(parser, expr)) << expression;
    };
    check("*A");
    check("A+");
    check("(");
    check("(A");
}
