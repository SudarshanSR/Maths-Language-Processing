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

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};
} // namespace mlp

#endif // VARIABLE_H
