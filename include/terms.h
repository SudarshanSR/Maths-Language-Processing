#ifndef TERMS_H
#define TERMS_H

#include "token.h"

#include <vector>

namespace mlp {
class Terms final {
    std::vector<Token> terms;

  public:
    Constant coefficient{1};

    Terms() = default;

    Terms(Terms const &);

    Terms(Terms &&) = default;

    Terms &operator=(Terms const &);

    Terms &operator=(Terms &&) = default;

    explicit operator std::string() const;

    [[nodiscard]] Terms operator-() const;

    Terms &operator*=(Variable variable);

    Terms &operator*=(Function function);

    Terms &operator*=(Term term);

    Terms &operator*=(Terms terms);

    Terms &operator*=(Expression expression);

    Terms &operator*=(Token token);

    Terms &operator/=(Variable variable);

    Terms &operator/=(Function const &function);

    Terms &operator/=(Term term);

    Terms &operator/=(Terms terms);

    Terms &operator/=(Expression const &expression);

    Terms &operator/=(Token token);

    Terms &operator*=(Constant scalar);

    Terms &operator/=(Constant scalar);

    [[nodiscard]] bool operator==(Terms const &) const;

    friend Token pow(Terms lhs, Constant rhs);

    friend bool is_dependent_on(Terms const &token, Variable const &variable);

    friend bool is_linear_of(Terms const &token, Variable const &variable);

    friend Token
    evaluate(Terms const &token, std::map<Variable, Token> const &values);

    friend Token simplified(Terms const &token);

    friend Token derivative(
        Terms const &token, Variable const &variable, std::uint32_t order
    );

    friend Token integral(Terms const &token, Variable const &variable);

    friend Token pow(Terms lhs, Constant rhs);

    friend Token pow(Terms const &lhs, Token const &rhs);
};

[[nodiscard]] Token operator+(Terms lhs, Constant rhs);
[[nodiscard]] Token operator+(Terms const &lhs, Token const &rhs);

[[nodiscard]] Token operator-(Terms lhs, Constant rhs);
[[nodiscard]] Token operator-(Terms const &lhs, Token const &rhs);

[[nodiscard]] Token operator*(Terms lhs, Constant rhs);
[[nodiscard]] Token operator*(Terms lhs, Token const &rhs);
[[nodiscard]] Token operator*(Terms lhs, Expression rhs);

[[nodiscard]] Token operator/(Terms lhs, Constant rhs);
[[nodiscard]] Token operator/(Terms lhs, Token const &rhs);
} // namespace mlp

#endif // TERMS_H
