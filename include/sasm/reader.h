#pragma once

#include <sstream>

namespace sasm {

struct character {
    size_t offset;
    size_t width;
    char value;
    
    bool eof() const;
    
    static const character end_of_file;
};

class reader {
    std::istringstream m_input;
    size_t m_offset;

public:
    explicit reader(const std::string& content);

    character get();
};

}
