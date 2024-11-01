#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "token.h"

namespace mlp {
class Expression final : public Token {
    std::vector<std::pair<Sign, OwnedToken>> tokens;

  public:
    Expression() = default;

    [[nodiscard]] gsl::owner<Expression *> clone() const override;

    void add_token(Sign sign, OwnedToken &&token);

    [[nodiscard]] std::pair<Sign, OwnedToken> pop_token();

    [[nodiscard]] bool empty() const;

    explicit operator std::string() const override;

    Expression &operator+=(OwnedToken &&token);

    Expression &operator-=(OwnedToken &&token);

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

#endif // EXPRESSION_H
