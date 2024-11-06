#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "token.h"

#include <vector>

namespace mlp {
class Expression final : public Token {
    std::vector<std::pair<Sign, OwnedToken>> tokens;

  public:
    using Token::derivative;
    using Token::integral;

    Expression() = default;

    Expression(Expression const &);

    Expression(Expression &&) = default;

    Expression &operator=(Expression const &) = default;

    Expression &operator=(Expression &&) = default;

    [[nodiscard]] gsl::owner<Expression *> clone() const override;

    [[nodiscard]] gsl::owner<Expression *> move() && override;

    void add_token(Sign sign, OwnedToken &&token);

    void add_token(Sign sign, Token const &token);

    void add_token(Sign sign, Token &&token);

    [[nodiscard]] bool empty() const;

    explicit operator std::string() const override;

    [[nodiscard]] Expression operator-() const;

    Expression &operator+=(Token const &token);

    Expression &operator-=(Token const &token);

    Expression &operator+=(OwnedToken &&token);

    Expression &operator-=(OwnedToken &&token);

    Expression &operator+=(Token &&token);

    Expression &operator-=(Token &&token);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

    Expression &operator*=(Token const &token);

    Expression &operator*=(Expression const &rhs);
};

[[nodiscard]] Expression operator+(Expression lhs, Token const &rhs);

[[nodiscard]] Expression operator-(Expression lhs, Token const &rhs);

[[nodiscard]] Expression operator*(Expression lhs, Expression const &rhs);

[[nodiscard]] Expression operator*(Expression lhs, Token const &rhs);
} // namespace mlp

#endif // EXPRESSION_H
