#include <gtest/gtest.h>

#include <sasm/parser.h>

class TestParser : public ::testing::Test {
public:
    struct test_parser {
        sasm::reader m_reader;
        sasm::lexer m_lexer;
        sasm::parser m_parser;
        explicit test_parser(const std::string& content)
        : m_reader(content)
        , m_lexer(&m_reader)
        , m_parser(&m_lexer)
        {}

        auto get() { return m_parser.get(); }
    };

    static constexpr auto BYTE = sasm::dtype::u8;
    static constexpr auto SBYTE = sasm::dtype::i8;
    static constexpr auto WORD = sasm::dtype::u16;
    
    static void CheckValue(const sasm::operand_t& actual,
                           int expected,
                           sasm::dtype::etype type = sasm::dtype::any) {
        ASSERT_TRUE(actual.is_value());
        EXPECT_EQ(actual.get_value(), expected);
        EXPECT_EQ(actual.type, type);
    }
    static void CheckReference(const sasm::operand_t& actual,
                               const std::string& expected,
                               sasm::dtype::etype type = sasm::dtype::any) {
        ASSERT_TRUE(actual.is_reference());
        EXPECT_EQ(actual.get_reference(), expected);
        EXPECT_EQ(actual.type, type);
    }
    static void CheckExpression(const sasm::operand_t& actual,
                                sasm::dtype::etype type = sasm::dtype::any) {
        EXPECT_TRUE(actual.is_expression());
        EXPECT_EQ(actual.type, type);
    }
};

TEST_F(TestParser, Empty) {
    test_parser parser("");
    EXPECT_TRUE(parser.get().eof());
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, Label) {
    test_parser parser("label:");

    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::label);
    EXPECT_EQ(item.content, "label");
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionImplied) {
    test_parser parser("NOP");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::NOP);
    EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::no_op);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAccumulator) {
    test_parser parser("ROL");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ROL);
    EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::no_op);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionImmediate) {
    test_parser parser(R"(
        ADC #$10
        ADC #REF
        ADC #-EXPR
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ADC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::immediate);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF", BYTE);
    
     item = parser.get();
    check(item);
    CheckExpression(item.instr.operand, BYTE);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropage) {
    test_parser parser(R"(
        ADC $10
        ADC REF
        ADC -EXPR
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ADC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);

    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");

    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropageX) {
    test_parser parser(R"(
        LDY $10,X
        LDY REF,X
        LDY -EXPR,X
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::LDY);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct_x);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropageY) {
    test_parser parser(R"(
        LDX $10,Y
        LDX REF,Y
        LDX -EXPR,Y
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::LDX);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct_y);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsolute) {
    test_parser parser(R"(
        ADC $1234
        ADC REF
        ADC -EXPR
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ADC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x1234, WORD);
        
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsoluteX) {
    test_parser parser(R"(
        LDY $1234,X
        LDY REF,X
        LDY -EXPR,X
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::LDY);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct_x);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x1234, WORD);
        
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsoluteY) {
    test_parser parser(R"(
        LDX $1234,Y
        LDX REF,Y
        LDX -EXPR,Y
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::LDX);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct_y);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x1234, WORD);
        
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF");
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndirect) {
    test_parser parser(R"(
        JMP ($1234)
        JMP (REF)
        JMP (-EXPR)
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::JMP);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::indirect);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x1234, WORD);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF", WORD);
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand, WORD);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndexedIndirect) {
    test_parser parser(R"(
        ADC ($10,X)
        ADC (REF,X)
        ADC (-EXPR,X)
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ADC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::indirect_x);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF", BYTE);
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand, BYTE);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndirectIndexed) {
    test_parser parser(R"(
        ADC ($10),Y
        ADC (REF),Y
        ADC (-EXPR),Y
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::ADC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::indirect_y);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, BYTE);
    
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF", BYTE);
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand, BYTE);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionRelative) {
    test_parser parser(R"(
        BCC *+$10
        BCC *+REF
        BCC *-EXPR
    )");
    
    const auto check = [this] (const sasm::parser_token& item) {
        EXPECT_EQ(item.kind, sasm::parser_token::instruction);
        EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::BCC);
        EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::relative);
    };

    auto item = parser.get();
    check(item);
    CheckValue(item.instr.operand, 0x10, SBYTE);
        
    item = parser.get();
    check(item);
    CheckReference(item.instr.operand, "REF", SBYTE);
    
    item = parser.get();
    check(item);
    CheckExpression(item.instr.operand, SBYTE);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionRelativeAbsoluteOperand) {
    {
    test_parser parser("BCC $1234");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.instr.name, sasm::instruction_set::instruction_name::BCC);
    EXPECT_EQ(item.instr.style, sasm::instruction_set::addressing_style::direct);
    CheckValue(item.instr.operand, 0x1234, WORD);
    
    EXPECT_TRUE(parser.get().eof());
    }
}

TEST_F(TestParser, Unknown_UnknownToken) {
    test_parser parser("<>");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::unknown);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseLiteral) {
    EXPECT_EQ(sasm::parse_literal("10"), 10);
    EXPECT_EQ(sasm::parse_literal("$10"), 0x10);
    EXPECT_EQ(sasm::parse_literal("%10"), 0b10);
}

TEST_F(TestParser, ParseSign) {
    EXPECT_EQ(sasm::parse_sign("+"), 1);
    EXPECT_EQ(sasm::parse_sign("-"), -1);
}

TEST_F(TestParser, ParseOperation) {
    EXPECT_EQ(sasm::instruction_set::parse_operation("NOP"),
              sasm::instruction_set::instruction_name::NOP);
    EXPECT_EQ(sasm::instruction_set::parse_operation("invalid"),
              sasm::instruction_set::instruction_name::unknown);
}

TEST_F(TestParser, ParseDefines) {
    test_parser parser(R"(
        .define A $10        ; define value
        .define AA A         ; transitive value
        .define B reference  ; define reference
        .define BB B         ; transitive reference
    )");
    
    auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::define);
    EXPECT_EQ(item.content, "A");
    CheckValue(item.operand, 0x10);
    
    item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::define);
    EXPECT_EQ(item.content, "AA");
    CheckReference(item.operand, "A");

    item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::define);
    EXPECT_EQ(item.content, "B");
    CheckReference(item.operand, "reference");

    item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::define);
    EXPECT_EQ(item.content, "BB");
    CheckReference(item.operand, "B");

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseAlign) {
    test_parser parser(R"(
        .align $10
        .define A $20
        .align A
    )");
    
    auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::align);
    CheckValue(item.operand, 0x10);
    
    item = parser.get();
    item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::align);
    CheckReference(item.operand, "A");

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseByte) {
    test_parser parser(R"(
        .byte $20
        .byte $80, $A0
    )");
    
    const auto check = [this] (const sasm::parser_token& token, int expected) {
        EXPECT_EQ(token.kind, sasm::parser_token::data);
        CheckValue(token.operand, expected, BYTE);
    };

    check(parser.get(), 0x20);
    check(parser.get(), 0x80);
    check(parser.get(), 0xA0);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseWord) {
    test_parser parser(R"(
        .word $20
        .word $80, $A0
    )");
    
    const auto check = [this] (const sasm::parser_token& token, int expected) {
        EXPECT_EQ(token.kind, sasm::parser_token::data);
        CheckValue(token.operand, expected, WORD);
    };

    check(parser.get(), 0x20);
    check(parser.get(), 0x80);
    check(parser.get(), 0xA0);

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseImport) {
    test_parser parser(".import IMPORTED_SYMBOL");

    auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::import_symbol);
    EXPECT_EQ(item.content, "IMPORTED_SYMBOL");

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseExport) {
    test_parser parser(".export EXPORTED_SYMBOL");

    auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::export_symbol);
    EXPECT_EQ(item.content, "EXPORTED_SYMBOL");

    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, ParseExpression) {
    // Only support "-x" for now?
    {
        test_parser parser("A + 5 * B");

        sasm::expression_t expr;
        sasm::try_parse_expression(parser.m_parser, expr);
    }

    // auto item = parser.get();
    // EXPECT_EQ(item.kind, sasm::parser_token::define);
    // EXPECT_EQ(item.content, "A");

    // item = parser.get();
    // EXPECT_EQ(item.kind, sasm::parser_token::define);
    // EXPECT_EQ(item.content, "B");

    // item = parser.get();
    // EXPECT_EQ(item.kind, sasm::parser_token::define);
    // EXPECT_EQ(item.content, "C");

    // EXPECT_TRUE(parser.get().eof());
}
