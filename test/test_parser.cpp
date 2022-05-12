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
    EXPECT_EQ(item.name, sasm::instruction_set::name::NOP);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::implied);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAccumulator) {
    test_parser parser("ROL");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ROL);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::accumulator);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionImmediate) {
    test_parser parser("ADC #$10");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ADC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::immediate);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropage) {
    test_parser parser("ADC $10");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ADC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::zeropage);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropageX) {
    test_parser parser("LDY $10,X");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::LDY);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::zeropage_x);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionZeropageY) {
    test_parser parser("LDX $10,Y");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::LDX);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::zeropage_y);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsolute) {
    test_parser parser("ADC $1234");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ADC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::absolute);
    EXPECT_EQ(item.operand, 0x1234);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsoluteX) {
    test_parser parser("LDY $1234,X");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::LDY);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::absolute_x);
    EXPECT_EQ(item.operand, 0x1234);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionAbsoluteY) {
    test_parser parser("LDX $1234,Y");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::LDX);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::absolute_y);
    EXPECT_EQ(item.operand, 0x1234);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndirect) {
    test_parser parser("JMP ($1234)");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::JMP);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::indirect);
    EXPECT_EQ(item.operand, 0x1234);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndexedIndirect) {
    test_parser parser("ADC ($10,X)");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ADC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::indexed_indirect);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionIndirectIndexed) {
    test_parser parser("ADC ($10),Y");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::ADC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::indirect_indexed);
    EXPECT_EQ(item.operand, 0x10);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionRelative) {
    test_parser parser("BCC *+$10");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::BCC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::relative);
    EXPECT_EQ(item.operand, 0x10);
    EXPECT_TRUE(item.relative_operand);
    
    EXPECT_TRUE(parser.get().eof());
}

TEST_F(TestParser, InstructionRelativeAbsoluteOperand) {
    {
    test_parser parser("BCC $1234");
    
    const auto item = parser.get();
    EXPECT_EQ(item.kind, sasm::parser_token::instruction);
    EXPECT_EQ(item.name, sasm::instruction_set::name::BCC);
    EXPECT_EQ(item.mode, sasm::instruction_set::addressing_mode::relative);
    EXPECT_EQ(item.operand, 0x1234);
    EXPECT_FALSE(item.relative_operand);
    
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
    EXPECT_EQ(sasm::parser::parse_literal("10"), 10);
    EXPECT_EQ(sasm::parser::parse_literal("$10"), 0x10);
    EXPECT_EQ(sasm::parser::parse_literal("%10"), 0b10);
}

TEST_F(TestParser, ParseSign) {
    EXPECT_EQ(sasm::parser::parse_sign("+"), 1);
    EXPECT_EQ(sasm::parser::parse_sign("-"), -1);
}

TEST_F(TestParser, ParseOperation) {
    EXPECT_EQ(sasm::instruction_set::parse_operation("NOP"),
              sasm::instruction_set::name::NOP);
    EXPECT_EQ(sasm::instruction_set::parse_operation("invalid"),
              sasm::instruction_set::name::unknown);
}
