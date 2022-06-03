#pragma once

#include <sasm/assert.h>
#include <sasm/dtype.h>
#include <sasm/parser_base.h>

#include <string>
#include <deque>
#include <map>
#include <vector>
#include <optional>

namespace sasm {

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

struct expression_item_t;

using value_t = int;
using reference_t = std::string;
struct operation_t {
    using operation_f = bool (std::vector<expression_item_t>& stack);
    operation_f* execute;
    int precedence = 0;
    bool is_unary = false;
    bool is_left_associative = false;

    auto signature() const { return std::tie(execute, precedence, is_left_associative); }
    bool operator==(const operation_t& other) const {
        return this->signature() == other.signature();
    }
    bool operator!=(const operation_t& other) const {
        return !(*this == other);
    }
};

struct expression_item_t {
    enum ekind {
        value, reference, operation,
    } kind;
    value_t val;
    reference_t ref;
    operation_t op;

    template <ekind K> bool is() const { return kind == K; }
};

namespace operations {
    static bool eval_failed(std::vector<expression_item_t>& stack) {
        return false;
    }
    static bool eval_identity(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return true;
    }
    static bool eval_negation(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return false;
    }
    static bool eval_addition(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return false;
    }
    static bool eval_subtraction(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return false;
    }
    static bool eval_multiplication(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return false;
    }
    static bool eval_division(std::vector<expression_item_t>& stack) {
        if (stack.size() == 0) return false;
        return false;
    }

    static const operation_t marker { &eval_failed, 100 };
    static const operation_t identity { &eval_identity, 0, true };
    static const operation_t negation { &eval_negation, 0, true };
    static const operation_t addition { &eval_addition, 2 };
    static const operation_t subtraction { &eval_subtraction, 2 };
    static const operation_t multiplication { &eval_multiplication, 1 };
    static const operation_t division { &eval_division, 1 };
}

static expression_item_t marker { expression_item_t::operation,
    0, "", operations::marker };
static expression_item_t identity { expression_item_t::operation,
    0, "", operations::identity };
static expression_item_t negation { expression_item_t::operation,
    0, "", operations::negation };
static expression_item_t addition { expression_item_t::operation,
    0, "", operations::addition };
static expression_item_t subtraction { expression_item_t::operation,
    0, "", operations::subtraction };
static expression_item_t multiplication { expression_item_t::operation,
    0, "", operations::multiplication };
static expression_item_t division { expression_item_t::operation,
    0, "", operations::division };

struct expression_t {
    std::vector<expression_item_t> content;
    dtype::etype type;

    bool is_value() const {
        return (content.size() == 1)
            && content.front().is<expression_item_t::value>();
    }
    value_t get_value() const {
        return content.front().val;
    }
    
    bool is_reference() const {
        return (content.size() == 1)
            && content.front().is<expression_item_t::reference>();
    }
    reference_t get_reference() const {
        return content.front().ref;
    }
    
    bool is_expression() const {
        return content.size() > 1;
    }

    void negate() {
        content.push_back(negation);
    }
};

static bool validate(const expression_t& expr) {
    int n = 0;
    for (const auto& item : expr.content) {
        switch (item.kind) {
            case expression_item_t::value:
            case expression_item_t::reference: {
                ++n;
                break;
            }
            case expression_item_t::operation: {
                n -= item.op.is_unary ? 0 : 1;
                break;
            }
            default:
                return false;
        }
    }
    return n == 1;
}

static std::optional<expression_item_t> try_get_operation(const lexer_token& token, bool allow_unary) {
    using enum lexer_token::token_type;
    /*if (token.is<symbol>("(")) {
        return marker;
    } else */if (token.is<symbol>("+")) {
        return allow_unary ? identity : addition;
    } else if (token.is<symbol>("-")) {
        return allow_unary ? negation : subtraction;
    } else if (token.is<symbol>("*")) {
        return multiplication;
    } else if (token.is<symbol>("/")) {
        return division;
    }
    return std::nullopt;
}

static bool try_parse_expression(parser_base_t& p, expression_t& expr) {
    using enum lexer_token::token_type;
    p.push_scope();
    expr.content.clear();
    std::vector<expression_item_t> op_stack;
    {
        bool allow_unary = true;
        std::optional<expression_item_t> operation;

        bool keep_parsing = true;
        while (keep_parsing) {
            auto token = p.stage_token();
            if (token.is<symbol>("(")) {
                op_stack.push_back(marker);
                allow_unary = true;
            } else if (token.is<symbol>(")")) {
                while (true) {
                    if (op_stack.empty()) {
                        p.unstage_token();
                        keep_parsing = false;
                        break;
                    }
                    const auto op = op_stack.back();
                    op_stack.pop_back();
                    assert(op.is<expression_item_t::operation>());
                    if (op.op == operations::marker) {
                        break;
                    }
                    expr.content.push_back(op);
                }
                allow_unary = false;
            } else if (operation = try_get_operation(token, allow_unary)) {
                while (!op_stack.empty()) {
                    const auto head = op_stack.back();
                    assert(head.is<expression_item_t::operation>());
                    if (head.op.precedence >= operation->op.precedence) break;
                    op_stack.pop_back();
                    expr.content.push_back(head);
                }
                op_stack.push_back(*operation);
                allow_unary = true;
            } else if (token.is<identifier>()) {
                expression_item_t item;
                item.kind = expression_item_t::reference;
                item.ref = token.content;
                expr.content.push_back(item);
                allow_unary = false;
            } else if (token.is<literal>()) {
                expression_item_t item;
                item.kind = expression_item_t::value;
                item.val = parse_literal(token.content);
                expr.content.push_back(item);
                allow_unary = false;
            } else {
                keep_parsing = false;
                while (!op_stack.empty()) {
                    const auto op = op_stack.back();
                    assert(op.is<expression_item_t::operation>());
                    if (op.op == operations::marker) break;
                    op_stack.pop_back();
                    expr.content.push_back(op);
                }
                p.unstage_token();
                break;
            }
        }
    }
    if (op_stack.empty() && (expr.content.size() > 0)) {
        p.accept_scope();
        return validate(expr);
    } else {
        p.cancel_scope();
        return false;
    }
}

}
