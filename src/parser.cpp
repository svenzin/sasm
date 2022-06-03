#include <sasm/parser.h>

namespace sasm {

lighweight_parser::lighweight_parser(parser& p)
: m_parser(p)
{}

void lighweight_parser::reset() {
    m_parser.reset();
}

lexer_token lighweight_parser::get() {
    return m_parser.stage_token();
}

bool lighweight_parser::try_get_operand(operand_t& operand, dtype::etype type) {
    return m_parser.try_parse_operand(operand, type);
}

}
