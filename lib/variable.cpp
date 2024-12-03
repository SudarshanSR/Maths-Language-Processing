#include "../include/variable.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"

#include <map>
#include <sstream>

mlp::Variable::Variable(Constant const coefficient, char const var)
    : var(var), coefficient(coefficient) {}

mlp::Variable::Variable(char const var) : var(var) {}

mlp::Variable::operator std::string() const {
    std::stringstream stream;

    if (this->coefficient == 0) {
        stream << 0;

        return stream.str();
    }

    if (this->coefficient != 1) {
        if (this->coefficient == -1)
            stream << "-";
        else
            stream << this->coefficient;
    }

    stream << this->var;

    return stream.str();
}

mlp::Variable mlp::Variable::operator-() const {
    return {-this->coefficient, this->var};
}

bool mlp::Variable::operator<(Variable const rhs) const {
    return this->var < rhs.var;
}

bool mlp::Variable::operator==(Variable const rhs) const {
    return this->var == rhs.var;
}

mlp::Variable &mlp::Variable::operator*=(Constant const rhs) {
    this->coefficient *= rhs;

    return *this;
}

mlp::Variable &mlp::Variable::operator/=(Constant const rhs) {
    this->coefficient /= rhs;

    return *this;
}

bool mlp::is_dependent_on(Variable const token, Variable const variable) {
    return token == variable;
}

bool mlp::is_linear_of(Variable const token, Variable const variable) {
    return is_dependent_on(token, variable);
}

mlp::Token
mlp::evaluate(Variable token, std::map<Variable, Token> const &values) {
    if (!values.contains(token))
        return token;

    return token.coefficient * values.at(token);
}

mlp::Token mlp::simplified(Variable token) { return token; }

mlp::Token mlp::derivative(
    Variable token, Variable const variable, std::uint32_t const order
) {
    if (!order)
        return token;

    return token == variable && order == 1 ? token.coefficient : 0;
}

mlp::Token mlp::integral(Variable const token, Variable const variable) {
    if (token == variable)
        return pow(token, 2) / (2 * token.coefficient);

    return token * variable;
}

mlp::Token mlp::operator+(Variable lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Variable lhs, Variable const rhs) {
    if (rhs.coefficient < 0)
        return lhs - -rhs;

    if (lhs == rhs) {
        lhs.coefficient += rhs.coefficient;

        if (lhs.coefficient == 0)
            return 0.0;

        return lhs;
    }

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Variable const lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Variable const lhs, Term const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Variable const lhs, Terms const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Variable const lhs, Expression rhs) {
    return rhs += lhs;
}

mlp::Token mlp::operator-(Variable lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Variable lhs, Variable const rhs) {
    if (rhs.coefficient < 0)
        return lhs + -rhs;

    if (lhs == rhs) {
        lhs.coefficient -= rhs.coefficient;

        if (lhs.coefficient == 0)
            return 0.0;

        return lhs;
    }

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Variable const lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return -rhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Variable const lhs, Term const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Variable const lhs, Terms const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Variable const lhs, Expression rhs) {
    return -(rhs -= lhs);
}

mlp::Token mlp::operator*(Variable lhs, Constant const rhs) {
    if (rhs == 0)
        return 0.0;

    if (rhs == 1)
        return lhs;

    return lhs *= rhs;
}

mlp::Token mlp::operator*(Variable const lhs, Variable const rhs) {
    if (lhs == rhs)
        return lhs.coefficient * rhs.coefficient * pow(lhs, 2);

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Variable const lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Variable const lhs, Term const &rhs) {
    if (lhs.coefficient == 0 || rhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Variable const lhs, Terms rhs) {
    if (lhs.coefficient == 0 || rhs.coefficient == 0)
        return 0.0;

    return rhs *= lhs;
}

mlp::Token mlp::operator*(Variable const lhs, Expression const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator/(Variable lhs, Constant const rhs) {
    if (rhs == 0)
        throw std::domain_error{"Division by 0!"};

    if (rhs == 1)
        return lhs;

    return lhs /= rhs;
}

mlp::Token mlp::operator/(Variable const lhs, Variable const rhs) {
    if (lhs == rhs)
        return lhs.coefficient / rhs.coefficient;

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Variable const lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Variable const lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Variable const lhs, Terms const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Variable const lhs, Expression const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::pow(Variable lhs, Constant const rhs) {
    if (lhs.coefficient == 0 && rhs <= 0)
        throw std::domain_error{"Indeterminate!"};

    if (lhs.coefficient == 0)
        return 0.0;

    if (rhs == 0)
        return 1.0;

    if (rhs == 1)
        return lhs;

    Constant const c = std::pow(lhs.coefficient, rhs);
    lhs.coefficient = 1;

    return Term{c, lhs, rhs};
}

mlp::Token mlp::pow(Variable lhs, Function rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Variable lhs, Term rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient <= 0)
        throw std::domain_error{"Indeterminate!"};

    if (lhs.coefficient == 0)
        return 0.0;

    if (rhs.coefficient == 0)
        return 1.0;

    return Term{1, lhs, rhs};
}

mlp::Token mlp::pow(Variable lhs, Terms rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient <= 0)
        throw std::domain_error{"Indeterminate!"};

    if (lhs.coefficient == 0)
        return 0.0;

    if (rhs.coefficient == 0)
        return 1.0;

    return Term{1, lhs, rhs};
}
