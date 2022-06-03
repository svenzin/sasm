#pragma once

#include <sasm/lexer.h>

#include <vector>

namespace sasm {

class parser_base_t {
    lexer* m_lexer;
    auto get_token();

    size_t m_current;
    std::vector<lexer_token> m_buffer;
    std::vector<size_t> m_scopes;

public:
    explicit parser_base_t(lexer* lexer);

    lexer_token stage_token();
    void unstage_token();

    void push_scope();
    void accept_scope();
    void cancel_scope();

    void accept();
    void reset();
};

}
