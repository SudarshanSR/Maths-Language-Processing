#ifndef VARIABLE_H
#define VARIABLE_H

#include "token.h"

namespace mlp {
class Variable final {
    char var;

  public:
    explicit Variable(char var);

    Variable(Variable const &) = default;

    Variable(Variable &&) = default;

    Variable &operator=(Variable const &) = default;

    Variable &operator=(Variable &&) = default;

    explicit operator std::string() const;

    bool operator==(Variable const &) const = default;

    friend bool operator<(Variable const &lhs, Variable const &rhs);
};

[[nodiscard]] bool
is_dependent_on(Variable const &token, Variable const &variable);

[[nodiscard]] bool
is_linear_of(Variable const &token, Variable const &variable);

[[nodiscard]] Token
evaluate(Variable const &token, std::map<Variable, Token> const &values);

[[nodiscard]] Token simplified(Variable const &token);

[[nodiscard]] Token derivative(
    Variable const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] Token integral(Variable const &token, Variable const &variable);
} // namespace mlp

#endif // VARIABLE_H
