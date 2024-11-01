#include "../include/variable.h"

#include "../include/constant.h"
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

bool mlp::Variable::is_dependent_on(Variable const &variable) const {
    return *this == variable;
}

bool mlp::Variable::is_linear_of(Variable const &variable) const {
    return this->is_dependent_on(variable);
}

mlp::OwnedToken
mlp::Variable::evaluate(std::map<Variable, SharedToken> const &values) const {
    return OwnedToken(
        values.contains(*this) ? values.at(*this)->clone() : this->clone()
    );
}

mlp::OwnedToken mlp::Variable::simplified() const {
    return std::unique_ptr<Variable>(this->clone());
}

mlp::OwnedToken mlp::Variable::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return Owned<Variable>(this->clone());

    return std::make_unique<Constant>(*this == variable && order == 1 ? 1 : 0);
}

mlp::OwnedToken mlp::Variable::integral(Variable const &variable) {
    if (*this == variable)
        return std::make_unique<Term>((variable ^ 2) / 2);

    auto terms = std::make_unique<Terms>();
    *terms *= Variable(variable);
    *terms *= Variable(*this);

    return terms;
}

namespace mlp {
bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}
} // namespace mlp
