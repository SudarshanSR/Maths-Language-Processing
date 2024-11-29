#include "../include/variable.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"

mlp::Variable::Variable(char const var) : var(var) {}

gsl::owner<mlp::Variable *> mlp::Variable::clone() const {
    return new Variable(*this);
}

gsl::owner<mlp::Variable *> mlp::Variable::move() && {
    return new Variable(std::move(*this));
}

mlp::Variable::operator std::string() const { return {this->var}; }

bool mlp::Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

bool mlp::is_dependent_on(Variable const &token, Variable const &variable) {
    return token == variable;
}

bool mlp::is_linear_of(Variable const &token, Variable const &variable) {
    return is_dependent_on(token, variable);
}

mlp::token mlp::evaluate(
    Variable const &token, std::map<Variable, SharedToken> const &values
) {
    return values.contains(token) ? to_variant(*values.at(token)) : token;
}

mlp::token mlp::simplified(Variable const &token) { return token; }

mlp::token mlp::derivative(
    Variable const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    return Constant(token == variable && order == 1 ? 1 : 0);
}

mlp::token mlp::integral(Variable const &token, Variable const &variable) {
    if (token == variable)
        return (variable ^ 2) / 2;

    return token * variable;
}

namespace mlp {
bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}
} // namespace mlp
