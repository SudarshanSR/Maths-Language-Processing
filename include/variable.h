#ifndef VARIABLE_H
#define VARIABLE_H

#include "token.h"

namespace mlp {
class Variable final : public Token {
    char var;

  public:
    explicit Variable(char var);

    Variable(Variable const &) = default;

    Variable(Variable &&) = default;

    [[nodiscard]] gsl::owner<Variable *> clone() const override;

    [[nodiscard]] gsl::owner<Variable *> move() && override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;

    friend bool operator<(Variable const &lhs, Variable const &rhs);
};

[[nodiscard]] bool
is_dependent_on(Variable const &token, Variable const &variable);

[[nodiscard]] bool
is_linear_of(Variable const &token, Variable const &variable);

[[nodiscard]] token
evaluate(Variable const &token, std::map<Variable, SharedToken> const &values);

[[nodiscard]] token simplified(Variable const &token);

[[nodiscard]] token derivative(
    Variable const &token, Variable const &variable, std::uint32_t order
);

[[nodiscard]] token integral(Variable const &token, Variable const &variable);
} // namespace mlp

#endif // VARIABLE_H
