#pragma once

#include <sasm/reader.h>

namespace sasm {

struct lexer_token {
    enum token_type {
        unknown,
        end_of_file,
        end_of_line,
        whitespace,
        identifier,
        comment,
        literal,
        keyword,
        symbol,
    };
    token_type type;
    std::string content;
    size_t offset;
    size_t width;
    bool whitespace_before;
    bool first_on_line;
    bool is_trivia;

    bool eof() const;
    template <token_type kind> bool is() const {
        return (type == kind);
    }
    template <token_type kind_1, token_type kind_2> bool is() const {
        return (type == kind_1) || (type == kind_2);
    }
    template <token_type kind> bool is(const std::string& txt) const {
        return is<kind>() && (content == txt);
    }
    template <token_type kind> bool is(const std::string& txt1, const std::string& txt2) const {
        return is<kind>(txt1) || is<kind>(txt2);
    }
};

class lexer {
    reader* m_reader;
    character m_current;
    bool m_was_whitespace;
    bool m_was_end_of_line;

    static bool is_whitespace(char c);
    static bool is_identifier_head(char c);
    static bool is_identifier(char c);
    static bool is_comment_head(char c);
    static bool is_decimal_head(char c);
    static bool is_decimal(char c);
    static bool is_hexadecimal_head(char c);
    static bool is_hexadecimal(char c);
    static bool is_binary_head(char c);
    static bool is_binary(char c);
    static bool is_symbol(char c);

public:
    explicit lexer(reader* reader);

    lexer_token get();
};

}
