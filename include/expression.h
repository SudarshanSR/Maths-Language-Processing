#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "token.h"

#include <vector>

namespace mlp {
class Expression final {
    std::vector<std::pair<Sign, Token>> tokens;

  public:
    Expression() = default;

    Expression(Expression const &);

    Expression(Expression &&) = default;

    Expression &operator=(Expression const &);

    Expression &operator=(Expression &&) = default;

    void add_token(Sign sign, Token const &token);

    explicit operator std::string() const;

    [[nodiscard]] Expression operator-() const;

    Expression &operator+=(Expression const &rhs);

    Expression &operator+=(Constant rhs);

    Expression &operator+=(Variable const &rhs);

    Expression &operator+=(Function const &rhs);

    Expression &operator+=(Term const &rhs);

    Expression &operator+=(Terms const &rhs);

    Expression &operator+=(Token const &token);

    Expression &operator-=(Expression const &rhs);

    Expression &operator-=(Constant rhs);

    Expression &operator-=(Variable const &rhs);

    Expression &operator-=(Function const &rhs);

    Expression &operator-=(Term const &rhs);

    Expression &operator-=(Terms const &rhs);

    Expression &operator-=(Token const &token);

    friend bool
    is_dependent_on(Expression const &token, Variable const &variable);

    friend bool is_linear_of(Expression const &token, Variable const &variable);

    friend Token
    evaluate(Expression const &token, std::map<Variable, Token> const &values);

    friend Token simplified(Expression const &token);

    friend Token derivative(
        Expression const &token, Variable const &variable, std::uint32_t order
    );

    friend Token integral(Expression const &token, Variable const &variable);

    Expression &operator*=(Token const &token);

    Expression &operator*=(Expression const &rhs);

    Expression &operator/=(Token const &rhs);

    friend Token pow(Constant lhs, Expression const &rhs);
    friend Token pow(Token const &lhs, Expression const &rhs);

    [[nodiscard]] bool operator==(Expression const &) const;
};

[[nodiscard]] Expression operator+(Expression lhs, Token const &rhs);

[[nodiscard]] Expression operator-(Expression lhs, Token const &rhs);

[[nodiscard]] Token operator*(Expression lhs, Constant rhs);

[[nodiscard]] Expression operator*(Expression lhs, Expression const &rhs);
[[nodiscard]] Expression operator*(Expression lhs, Token const &rhs);

[[nodiscard]] Token operator/(Expression lhs, Constant rhs);

[[nodiscard]] Token pow(Expression const &lhs, Constant rhs);
[[nodiscard]] Token pow(Expression const &lhs, Token const &rhs);
} // namespace mlp

#endif // EXPRESSION_H
