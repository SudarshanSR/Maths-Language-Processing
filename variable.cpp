#include "token.h"

mlp::Variable::Variable(char const var) : var(var) {}

gsl::owner<mlp::Variable *> mlp::Variable::clone() const {
    return new Variable(this->var);
}

mlp::Variable::operator std::string() const { return {this->var}; }

bool mlp::Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

namespace mlp {
bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}
} // namespace mlp
