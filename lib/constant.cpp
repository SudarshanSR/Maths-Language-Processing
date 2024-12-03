#include "../include/constant.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

bool mlp::is_dependent_on(Constant, Variable) { return false; }

bool mlp::is_linear_of(Constant, Variable) { return false; }

mlp::Token mlp::evaluate(Constant token, std::map<Variable, Token> const &) {
    return token;
}

mlp::Token mlp::simplified(Constant token) { return token; }

mlp::Token mlp::derivative(Constant, Variable, std::uint32_t) { return 0.0; }

mlp::Token mlp::integral(Constant const token, Variable const variable) {
    if (token == 0)
        return 0.0;

    return variable / variable.coefficient;
}

mlp::Token mlp::operator+(Constant const lhs, Token const &rhs) {
    return rhs + lhs;
}

mlp::Token mlp::operator-(Constant const lhs, Token const &rhs) {
    return -(rhs - lhs);
}

mlp::Token mlp::operator*(Token const &lhs, Constant const rhs) {
    return std::visit([rhs](auto &&var) -> Token { return var * rhs; }, lhs);
}

mlp::Token mlp::operator*(Constant const lhs, Token const &rhs) {
    return rhs * lhs;
}

mlp::Token mlp::operator*(Constant lhs, Expression rhs) { return rhs *= lhs; }

mlp::Token mlp::operator/(Token const &lhs, Constant const rhs) {
    return std::visit([rhs](auto &&var) -> Token { return var / rhs; }, lhs);
}

mlp::Token mlp::operator/(Constant const lhs, Token const &rhs) {
    return std::visit([lhs](auto &&var) -> Token { return lhs / var; }, rhs);
}

mlp::Token mlp::operator/(Constant const lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs == 0)
        return 0.0;

    return lhs * pow(rhs, -1);
}

mlp::Token mlp::operator/(Constant const lhs, Function const &rhs) {
    if (lhs == 0)
        return 0.0;

    return lhs * pow(rhs, -1);
}

mlp::Token mlp::operator/(Constant const lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs == 0)
        return 0.0;

    return lhs * pow(rhs, -1);
}

mlp::Token mlp::operator/(Constant const lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs == 0)
        return 0.0;

    return lhs * pow(rhs, -1);
}

mlp::Token mlp::operator/(Constant const lhs, Expression const &rhs) {
    if (lhs == 0)
        return 0.0;

    return lhs * pow(rhs, -1);
}

mlp::Token mlp::pow(Constant const lhs, Variable rhs) {
    if (lhs == 0 && rhs.coefficient == 0)
        throw std::domain_error{"Indeterminate!"};

    if (rhs.coefficient == 0)
        return 0.0;

    if (lhs == 1)
        return 1.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Constant const lhs, Function rhs) {
    if (lhs == 0)
        return 0.0;

    if (lhs == 1)
        return 1.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Constant const lhs, Term rhs) {
    if (lhs == 0) {
        if (rhs.coefficient == 0 || rhs.coefficient == HUGE_VAL ||
            rhs.coefficient == -HUGE_VAL)
            throw std::domain_error{"Indeterminate!"};

        return 0.0;
    }

    if (lhs == 1)
        return 1.0;

    Constant const coefficient = std::pow(lhs, rhs.coefficient);
    rhs.coefficient = 1;

    return Term{coefficient, lhs, rhs};
}
