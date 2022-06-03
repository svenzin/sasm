#include <sasm/reader.h>

#include <tuple>

namespace sasm {

static const character end_of_file{-1, -1, -1};

auto ctuple(const character& c) {
    return std::tie(c.offset, c.width, c.value);
}

bool character::eof() const {
    return ctuple(*this) == ctuple(end_of_file);
}

reader::reader(const std::string& content)
: m_input(content)
, m_offset(0)
{}

character reader::get() {
    char c;
    if (m_input.get(c)) {
        character result{m_offset, 1, c};
        ++m_offset;
        return result;
    }
    return end_of_file;
}

}
