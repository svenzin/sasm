#pragma once

#include <sasm/lexer.h>

#include <string>

namespace sasm {

namespace instruction_set {

enum class name {
    undefined, unknown,
    ADC,
    BCC,
    JMP,
    LDX, LDY,
    NOP,
    ROL,
};

enum class addressing_mode {
    undefined, unknown,
    implied,
    accumulator,
    immediate,
    zeropage, zeropage_x, zeropage_y,
    absolute, absolute_x, absolute_y,
    indirect,
    indexed_indirect,
    indirect_indexed,
    relative,
};

name parse_operation(const std::string& content) {
    using enum name;
    static const std::map<std::string, name> conversion {
        { "ADC", ADC }, { "BCC", BCC }, { "JMP", JMP },
        { "LDX", LDX }, { "LDY", LDY },
        { "NOP", NOP }, { "ROL", ROL }
    };
    const auto it = conversion.find(content);
    if (it != conversion.end()) {
        return it->second;
    }
    return unknown;
}

}

struct symbol_t {
    enum symbol_kind {
        definition,
        reference,
    };
    symbol_kind kind;
    std::string name;

    static symbol_t make_definition(const std::string& content) { return { definition, content }; }
    static symbol_t make_reference(const std::string& content) { return { reference, content }; }
};

struct operand {
    enum operand_kind { value_kind, symbol_kind };
    operand_kind kind;
    int value;
    symbol_t symbol;
    bool is_value() const { return kind == value_kind; }
    bool is_symbol() const { return kind == symbol_kind; }
    static operand make_value(int content) {
        operand op;
        op.kind = value_kind;
        op.value = content;
        return op;
    }
    static operand make_symbol(const std::string& content) {
        operand op;
        op.kind = symbol_kind;
        op.symbol = symbol_t::make_reference(content);
        return op;
    }
};

struct parser_token {
    enum statement_kind {
        unknown,
        end_of_file,
        label,
        instruction,
    };
    statement_kind kind;
    instruction_set::name name = instruction_set::name::undefined;
    instruction_set::addressing_mode mode = instruction_set::addressing_mode::undefined;
    int operand;
    bool relative_operand = false;
    std::string content;

    bool eof() const { return kind == end_of_file; }

    static parser_token make_unknown() {
        parser_token token;
        token.kind = unknown;
        return token;
    }

    static parser_token make_eof() {
        parser_token token;
        token.kind = end_of_file;
        return token;
    }

    static parser_token make_label(const std::string& name) {
        parser_token token;
        token.kind = label;
        token.content = name;
        return token;
    }

    static parser_token make_instruction(instruction_set::name operation,
                                         instruction_set::addressing_mode mode,
                                         int operand = 0,
                                         bool is_relative = false) {
        parser_token token;
        token.kind = instruction;
        token.name = operation;
        token.mode = mode;
        token.operand = operand;
        token.relative_operand = is_relative;
        return token;
    }
};

class parser {
    lexer* m_lexer;
    auto get_token() {
        auto token = m_lexer->get();
        while (token.is_trivia) token = m_lexer->get();
        return token;
    }

public:
    static int parse_literal(const std::string& content) {
        size_t length;
        if (content[0] == '$') {
            try {
                return std::stoi(content.substr(1), &length, 16);
            }
            catch (std::out_of_range & e) {
                return 0;
            }
        } else if (content[0] == '%') {
            try {
                return std::stoi(content.substr(1), &length, 2);
            }
            catch (std::out_of_range & e) {
                return 0;
            }
        } else {
            try {
                return std::stoi(content, &length, 10);
            }
            catch (std::out_of_range & e) {
                return 0;
            }
        }
        return 0;
    }
    static int parse_sign(const std::string& content) {
        if (content == "+") return 1;
        if (content == "-") return -1;
        return 0;
    }
    static bool is_ubyte(int data) { return (0 <= data) && (data <= 0xFF); }
    static bool is_sbyte(int data) { return (-0x80 <= data) && (data <= 0x7F); }
    static bool is_uword(int data) { return (0 <= data) && (data <= 0xFFFF); }
    
    static bool is_zeropage(int address) { return is_ubyte(address); }

    explicit parser(lexer* lexer)
    : m_lexer(lexer)
    {}

    size_t m_current = 0;
    std::vector<lexer_token> m_buffer;
    std::vector<size_t> m_scopes;
    void push_scope() {
        m_scopes.push_back(m_current);
    }
    auto stage_token() {
        if (m_current == m_buffer.size()) {
            m_buffer.push_back(get_token());
        }
        return m_buffer[m_current++];
    }
    void cancel() {
        m_current = m_scopes.back();
        m_scopes.pop_back();
    }
    void accept() {
        m_current = 0;
        m_buffer.clear();
        m_scopes.clear();
    }

    parser_token get() {
        using enum lexer_token::token_type;
        {
            push_scope();
            if (stage_token().eof()) {
                accept();
                return parser_token::make_eof();
            }
            cancel();
        }

        {
            push_scope();
            lexer_token ident;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>(":")) {
                accept();
                return parser_token::make_label(ident.content);
            }
            cancel();
        }

        { // relative
            push_scope();
            lexer_token ident, sign, offset;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>("*")
                && ((sign = stage_token()).is<symbol>("+", "-"))
                && (offset = stage_token()).is<literal>()
            ) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::relative,
                    parse_sign(sign.content) * parse_literal(offset.content),
                    true
                );
            }
            cancel();
        }
        { // indirect y
            push_scope();
            lexer_token ident, address;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>("(")
                && (address = stage_token()).is<literal>()
                && stage_token().is<symbol>(")")
                && stage_token().is<symbol>(",")
                && stage_token().is<keyword>("Y")
            ) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::indirect_indexed,
                    parse_literal(address.content)
                );
            }
            cancel();
        }
        { // indirect x
            push_scope();
            lexer_token ident, address;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>("(")
                && (address = stage_token()).is<literal>()
                && stage_token().is<symbol>(",")
                && stage_token().is<keyword>("X")
                && stage_token().is<symbol>(")")
            ) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::indexed_indirect,
                    parse_literal(address.content)
                );
            }
            cancel();
        }
        { // indirect
            push_scope();
            lexer_token ident, address;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>("(")
                && (address = stage_token()).is<literal>()
                && stage_token().is<symbol>(")")
            ) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::indirect,
                    parse_literal(address.content)
                );
            }
            cancel();
        }
        { // absolute_x, absolute_y, zeropage_x, zeropage_y
            push_scope();
            lexer_token ident, address, index;
            if ((ident = stage_token()).is<identifier>()
                && (address = stage_token()).is<literal>()
                && stage_token().is<symbol>(",")
                && (index = stage_token()).is<keyword>()
            ) {
                accept();
                const auto parsed_address = parse_literal(address.content);
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    (is_zeropage(parsed_address) ?
                        (index.is<keyword>("X") ?
                            instruction_set::addressing_mode::zeropage_x
                            : (index.is<keyword>("Y") ?
                                instruction_set::addressing_mode::zeropage_y
                                : instruction_set::addressing_mode::unknown
                            )
                        )
                        : (index.is<keyword>("X")
                            ? instruction_set::addressing_mode::absolute_x
                            : (index.is<keyword>("Y") ?
                                instruction_set::addressing_mode::absolute_y
                                : instruction_set::addressing_mode::unknown
                            )
                        )
                    ),
                    parsed_address
                );
            }
            cancel();
        }
        { // absolute, zeropage, relative
            push_scope();
            lexer_token ident, address, index;
            if ((ident = stage_token()).is<identifier>()
                && (address = stage_token()).is<literal>()
            ) {
                accept();
                const auto parsed_address = parse_literal(address.content);
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    (is_zeropage(parsed_address) ?
                        instruction_set::addressing_mode::zeropage
                        : instruction_set::addressing_mode::absolute
                    ),
                    parsed_address
                );
            }
            cancel();
        }
        { // immediate
            push_scope();
            lexer_token ident, address, index;
            if ((ident = stage_token()).is<identifier>()
                && stage_token().is<symbol>("#")
                && (address = stage_token()).is<literal>()
            ) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::immediate,
                    parse_literal(address.content)
                );
            }
            cancel();
        }
        { // implied, accumulator
            push_scope();
            lexer_token ident, address, index;
            if ((ident = stage_token()).is<identifier>()) {
                accept();
                return parser_token::make_instruction(
                    instruction_set::parse_operation(ident.content),
                    instruction_set::addressing_mode::implied
                );
            }
            cancel();
        }

        accept();
        return parser_token::make_unknown();
    }
};

}
