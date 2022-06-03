#pragma once

#include <sasm/dtype.h>
#include <sasm/lexer.h>
#include <sasm/parser_base.h>
#include <sasm/expression.h>

#include <string>
#include <deque>
#include <map>
#include <vector>

namespace sasm {

using operand_t = expression_t;

class parser;
class lighweight_parser {
    friend parser;
    parser& m_parser;
    explicit lighweight_parser(parser& p);
public:
    void reset();
    lexer_token get();
    bool try_get_operand(operand_t& operand, dtype::etype type = dtype::any);
};

namespace instruction_set {

enum class instruction_name {
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

enum class addressing_style {
    undefined, unknown,
    no_op,
    immediate,
    direct, direct_x, direct_y,
    indirect, indirect_x, indirect_y,
    relative,
};

static instruction_name parse_operation(const std::string& content) {
    using enum instruction_name;
    static const std::map<std::string, instruction_name> conversion {
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

struct instruction {
    instruction_name name = instruction_name::undefined;
    addressing_style style = addressing_style::undefined;
    operand_t operand;
};

static bool is_zeropage(int address) { return dtype::is_u8(address); }

static bool try_parse_instruction(lighweight_parser& p, instruction& instr) {
    using enum lexer_token::token_type;
    { // relative
        lexer_token ident, sign, offset;
        if ((ident = p.get()).is<identifier>()
            && p.get().is<symbol>("*")
            && ((sign = p.get()).is<symbol>("+", "-"))
            && p.try_get_operand(instr.operand, dtype::i8)
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::relative;
            if (sign.is<symbol>("-")) {
                instr.operand.negate();
            }
            return true;
        }
    }
    p.reset();
    { // indirect y
        lexer_token ident;
        if ((ident = p.get()).is<identifier>()
            && p.get().is<symbol>("(")
            && p.try_get_operand(instr.operand, dtype::u8)
            && p.get().is<symbol>(")")
            && p.get().is<symbol>(",")
            && p.get().is<keyword>("Y")
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::indirect_y;
            return true;
        }
    }
    p.reset();
    { // indirect x
        lexer_token ident;
        if ((ident = p.get()).is<identifier>()
            && p.get().is<symbol>("(")
            && p.try_get_operand(instr.operand, dtype::u8)
            && p.get().is<symbol>(",")
            && p.get().is<keyword>("X")
            && p.get().is<symbol>(")")
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::indirect_x;
            return true;
        }
    }
    p.reset();
    { // indirect
        lexer_token ident;
        if ((ident = p.get()).is<identifier>()
            && p.get().is<symbol>("(")
            && p.try_get_operand(instr.operand, dtype::u16)
            && p.get().is<symbol>(")")
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::indirect;
            return true;
        }
    }
    p.reset();
    { // absolute_x, absolute_y, zeropage_x, zeropage_y
        lexer_token ident, index;
        if ((ident = p.get()).is<identifier>()
            && p.try_get_operand(instr.operand)
            && p.get().is<symbol>(",")
            && (index = p.get()).is<keyword>()
        ) {
            instr.name = parse_operation(ident.content);
            if (index.is<keyword>("X")) {
                instr.style = addressing_style::direct_x;
            } else if (index.is<keyword>("Y")) {
                instr.style = addressing_style::direct_y;
            } else {
                instr.style = addressing_style::unknown;
            }
            if (instr.operand.is_value()) {
                instr.operand.type = is_zeropage(instr.operand.get_value()) ? dtype::u8 : dtype::u16;
            }
            return true;
        }
    }
    p.reset();
    { // absolute, zeropage, relative
        lexer_token ident, index;
        if ((ident = p.get()).is<identifier>()
            && p.try_get_operand(instr.operand)
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::direct;
            if (instr.operand.is_value()) {
                instr.operand.type = is_zeropage(instr.operand.get_value()) ? dtype::u8 : dtype::u16;
            }
            return true;
        }
    }
    p.reset();
    { // immediate
        lexer_token ident, index;
        operand_t address;
        if ((ident = p.get()).is<identifier>()
            && p.get().is<symbol>("#")
            && p.try_get_operand(instr.operand, dtype::u8)
        ) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::immediate;
            return true;
        }
    }
    p.reset();
    { // implied, accumulator
        lexer_token ident, address, index;
        if ((ident = p.get()).is<identifier>()) {
            instr.name = parse_operation(ident.content);
            instr.style = addressing_style::no_op;
            return true;
        }
    }
    p.reset();
    return false;
}

}

struct parser_token {
    enum statement_kind {
        unknown,
        end_of_file,
        instruction,
        label,
        define, align, data, import_symbol, export_symbol,
    };
    statement_kind kind;

    instruction_set::instruction instr;
    operand_t operand;
    std::string content;

    bool eof() const { return kind == end_of_file; }

    template <statement_kind K>
    static parser_token make() {
        parser_token token;
        token.kind = K;
        return token;
    }

    template <statement_kind K>
    static parser_token make(const std::string& content) {
        auto token = make<K>();
        token.content = content;
        return token;
    }

    template <statement_kind K>
    static parser_token make(const operand_t& value) {
        auto token = make<K>();
        token.operand = value;
        return token;
    }

    static parser_token make_unknown() { return make<unknown>(); }
    static parser_token make_eof() { return make<end_of_file>(); }

    static parser_token make_label(const std::string& name) { return make<label>(name); }
    static parser_token make_import(const std::string& name) { return make<import_symbol>(name); }
    static parser_token make_export(const std::string& name) { return make<export_symbol>(name); }

    static parser_token make_alignment(const operand_t& value) { return make<align>(value); }
    static parser_token make_data(const operand_t& value) { return make<data>(value); }
    
    static parser_token make_define(const std::string& name, const operand_t& value) {
        auto token = make<define>(name);
        token.operand = value;
        return token;
    }

    static parser_token make_instruction(const instruction_set::instruction& instr) {
        auto token = make<instruction>();
        token.instr = instr;
        return token;
    }

};

class parser : public parser_base_t {
public:
    explicit parser(lexer* lexer)
    : parser_base_t(lexer)
    {}

    std::deque<parser_token> m_tokens;

    bool parse_label() {
        using enum lexer_token::token_type;
        push_scope();
        lexer_token ident;
        if ((ident = stage_token()).is<identifier>()
            && stage_token().is<symbol>(":")
        ) {
            accept();
            m_tokens.push_back(
                parser_token::make_label(ident.content)
            );
            return true;
        }
        cancel_scope();
        return false;
    }
    bool try_parse_operand(operand_t& operand, dtype::etype type = dtype::any) {
        operand.type = type;
        return try_parse_expression(*this, operand);
    }
    bool parse_define() {
        using enum lexer_token::token_type;
        push_scope();
        lexer_token name;
        operand_t definition;
        if (stage_token().is<symbol>(".")
            && stage_token().is<identifier>("define")
            && (name = stage_token()).is<identifier>()
            && try_parse_operand(definition)
        ) {
            accept();
            m_tokens.push_back(
                parser_token::make_define(name.content, definition)
            );
            return true;
        }
        cancel_scope();
        return false;
    }
    bool parse_align() {
        using enum lexer_token::token_type;
        push_scope();
        operand_t alignment;
        if (stage_token().is<symbol>(".")
            && stage_token().is<identifier>("align")
            && try_parse_operand(alignment)
        ) {
            accept();
            m_tokens.push_back(
                parser_token::make_alignment(alignment)
            );
            return true;
        }
        cancel_scope();
        return false;
    }
    template <lexer_token::token_type _type>
    void skip(const std::string& content) {
        push_scope();
        if (!stage_token().is<_type>(content)) {
            cancel_scope();
        }
    }
    bool parse_data() {
        using enum lexer_token::token_type;
        push_scope();
        if (stage_token().is<symbol>(".")) {
            const auto data_type = stage_token();
            if (data_type.is<identifier>("byte", "word")) {
                auto type = dtype::any;
                if (data_type.is<identifier>("byte")) type = dtype::u8;
                if (data_type.is<identifier>("word")) type = dtype::u16;

                operand_t data;
                if (try_parse_operand(data, type)) {
                    m_tokens.push_back(parser_token::make_data(data));

                    while (true) {
                        push_scope();
                        if (stage_token().is<symbol>(",")
                            && try_parse_operand(data, type)) {
                            m_tokens.push_back(parser_token::make_data(data));
                        } else {
                            cancel_scope();
                            break;
                        }
                    }

                    accept();
                    return true;
                }
            }
        }
        cancel_scope();
        return false;
    }
    bool parse_import() {
        using enum lexer_token::token_type;
        push_scope();
        lexer_token name;
        if (stage_token().is<symbol>(".")
            && stage_token().is<identifier>("import")
            && (name = stage_token()).is<identifier>()
        ) {
            m_tokens.push_back(parser_token::make_import(name.content));
            accept();
            return true;
        }
        cancel_scope();
        return false;
    }
    bool parse_export() {
        using enum lexer_token::token_type;
        push_scope();
        lexer_token name;
        if (stage_token().is<symbol>(".")
            && stage_token().is<identifier>("export")
            && (name = stage_token()).is<identifier>()
        ) {
            m_tokens.push_back(parser_token::make_export(name.content));
            accept();
            return true;
        }
        cancel_scope();
        return false;
    }
    bool parse_to_eol() {
        using enum lexer_token::token_type;
        push_scope();
        bool has_content = false;
        while (!stage_token().is<end_of_line, end_of_file>()) {
            has_content = true;
        }
        accept();
        if (has_content) {
            m_tokens.push_back(
                parser_token::make_unknown()
            );
        }
        return has_content;
    }

    bool parse_instruction() {
        push_scope();
        lighweight_parser lp(*this);
        instruction_set::instruction instr;
        if (try_parse_instruction(lp, instr)) {
            const auto token = parser_token::make_instruction(instr);
            m_tokens.push_back(token);
            accept();
            return true;
        }
        cancel_scope();
        return false;
    }

    bool parse_eof() {
        using enum lexer_token::token_type;
        push_scope();
        // if (stage_token().is<end_of_file>()) {
        if (stage_token().eof()) {
            accept();
            return true;
        }
        cancel_scope();
        return false;
    }

    bool parse_eol() {
        using enum lexer_token::token_type;
        push_scope();
        if (stage_token().is<end_of_line>()) {
            accept();
            return true;
        }
        cancel_scope();
        return false;
    }

    bool parse_line() {
        if (parse_eof()) return false;
        
        const auto has_parsed_directive = (
            parse_define()
            || parse_align()
            || parse_data()
            || parse_import()
            || parse_export()
        );
        if (!has_parsed_directive) {
            parse_label();
            parse_instruction();
        }
        if (parse_eol()) return true;
        if (parse_to_eol()) {
            ;
        }
        return true;
    }

    parser_token get() {
        while (m_tokens.empty()) {
            if (!parse_line()) return parser_token::make_eof();
        }

        const auto head = m_tokens.front();
        m_tokens.pop_front();
        return head;
    }
};

}
