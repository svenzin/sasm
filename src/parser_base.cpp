#include <sasm/parser_base.h>
#include <sasm/assert.h>

namespace sasm {

auto parser_base_t::get_token() {
    auto token = m_lexer->get();
    while (token.is_trivia) token = m_lexer->get();
    return token;
}

parser_base_t::parser_base_t(lexer* lexer)
: m_lexer(lexer)
, m_current(0)
{}

lexer_token parser_base_t::stage_token() {
    assert(m_current <= m_buffer.size());
    if (m_current == m_buffer.size()) {
        m_buffer.push_back(get_token());
    }
    return m_buffer[m_current++];
}

void parser_base_t::unstage_token() {
    if (m_scopes.empty()) {
        assert(m_current > 0);
    } else {
        assert(m_current > m_scopes.back());
    }
    --m_current;
}

void parser_base_t::push_scope() {
    m_scopes.push_back(m_current);
}

void parser_base_t::accept_scope() {
    assert(!m_scopes.empty());
    m_scopes.pop_back();
    if (m_scopes.empty()) accept();
}

void parser_base_t::cancel_scope() {
    assert(!m_scopes.empty());
    m_current = m_scopes.back();
    m_scopes.pop_back();
}

void parser_base_t::accept() {
    // assert(!m_scopes.empty()); // that's wrong
    if (m_current > 0) {
        std::copy(m_buffer.begin() + m_current,
                    m_buffer.end(),
                    m_buffer.begin());
    }
    m_buffer.resize(m_buffer.size() - m_current);
    m_current = 0;
    m_scopes.clear();
}

void parser_base_t::reset() {
    assert(!m_scopes.empty());
    m_current = m_scopes.back();
}

}
