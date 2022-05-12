#include <sasm/lexer.h>

#include <vector>

namespace sasm {

bool lexer_token::eof() const {
    return type == end_of_file;
}

bool lexer::is_whitespace(char c) {
    return (c == ' ')
        || (c == '\t');
}

bool lexer::is_identifier_head(char c) {
    return std::isalpha(c)
        || (c == '_');
}

bool lexer::is_identifier(char c) {
    return std::isalnum(c)
        || (c == '_');
}

bool lexer::is_comment_head(char c) {
    return (c == ';');
}

bool lexer::is_decimal_head(char c) {
    return std::isdigit(c) && (c != '0');
}

bool lexer::is_decimal(char c) {
    return std::isdigit(c);
}

bool lexer::is_hexadecimal_head(char c) {
    return (c == '$');
}

bool lexer::is_hexadecimal(char c) {
    return std::isxdigit(c);
}

bool lexer::is_binary_head(char c) {
    return (c == '%');
}

bool lexer::is_binary(char c) {
    return (c == '0')
        || (c == '1');
}

bool lexer::is_symbol(char c) {
    static std::string symbols = ".:(),+-#*";
    return symbols.find(c) != std::string::npos;
}

lexer::lexer(reader* reader)
: m_reader(reader)
, m_was_whitespace(false)
, m_was_end_of_line(true)
{
    m_current = m_reader->get();
}

lexer_token lexer::get() {
    std::vector<char> buffer;
    const size_t offset = m_current.offset;
    size_t width = 0;

    const auto whitespace_before = m_was_whitespace;
    const auto first_on_line = m_was_end_of_line;
    
    m_was_whitespace = false;
    m_was_end_of_line = false;

    const auto next = [&] () {
        width += m_current.width;
        buffer.push_back(m_current.value);
        m_current = m_reader->get();
    };

    const auto token = [&] (lexer_token::token_type type, bool is_trivia = false) -> lexer_token {
        return {
            type,
            std::string(buffer.cbegin(), buffer.cend()),
            offset,
            width,
            whitespace_before,
            first_on_line,
            is_trivia
        };
    };
    
    if (is_whitespace(m_current.value)) {
        next();
        while (is_whitespace(m_current.value)) {
            next();
        }
        m_was_whitespace = true;
        return token(lexer_token::whitespace, true);
    }
    
    if (m_current.value == '\r') {
        next();
    }
    if (m_current.value == '\n') {
        next();
        m_was_end_of_line = true;
        return token(lexer_token::end_of_line);
    }

    if (is_identifier_head(m_current.value)) {
        next();
        while (is_identifier(m_current.value)) {
            next();
        }
        auto identifier = token(lexer_token::identifier);
        
        static const std::vector<std::string> keywords {
            "X", "Y",
        };
        if (std::find(keywords.cbegin(),
                        keywords.cend(),
                        identifier.content)
            != keywords.cend()) {
            identifier.type = lexer_token::keyword;
        }

        return identifier;
    }

    if (is_comment_head(m_current.value)) {
        next();
        while (!m_current.eof()
            && !(m_current.value == '\r')
            && !(m_current.value == '\n')) {
            next();
        }
        return token(lexer_token::comment, true);
    }

    if (is_decimal_head(m_current.value)) {
        next();
        while (is_decimal(m_current.value)) {
            next();
        }
        return token(lexer_token::literal);
    }
    if (is_hexadecimal_head(m_current.value)) {
        next();
        while (is_hexadecimal(m_current.value)) {
            next();
        }
        return token(lexer_token::literal);
    }
    if (is_binary_head(m_current.value)) {
        next();
        while (is_binary(m_current.value)) {
            next();
        }
        return token(lexer_token::literal);
    }
    
    if (is_symbol(m_current.value)) {
        next();
        return token(lexer_token::symbol);
    }
    
    if (m_current.eof()) {
        return { lexer_token::end_of_file };
    }

    // Unrecognized content, skip until next separator
    // Current separators are whitespace/eol/eof
    while (!m_current.eof()
        && !is_whitespace(m_current.value)
        && !(m_current.value == '\n')) {
        next();
    }
    return token(lexer_token::unknown);
}

}
