#include "../include/variable.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"

#include <map>

mlp::Variable::Variable(char const var) : var(var) {}

mlp::Variable::operator std::string() const { return {this->var}; }

bool mlp::is_dependent_on(Variable const &token, Variable const &variable) {
    return token == variable;
}

bool mlp::is_linear_of(Variable const &token, Variable const &variable) {
    return is_dependent_on(token, variable);
}

mlp::Token
mlp::evaluate(Variable const &token, std::map<Variable, Token> const &values) {
    return values.contains(token) ? values.at(token) : token;
}

mlp::Token mlp::simplified(Variable const &token) { return token; }

mlp::Token mlp::derivative(
    Variable const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    return Constant(token == variable && order == 1 ? 1 : 0);
}

mlp::Token mlp::integral(Variable const &token, Variable const &variable) {
    if (token == variable)
        return (variable ^ 2) / 2;

    return token * variable;
}

namespace mlp {
bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}
} // namespace mlp
